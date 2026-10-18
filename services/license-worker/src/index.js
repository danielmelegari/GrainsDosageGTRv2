import { publicKeySpki } from './public-key.js';
import { page, clientScript } from './page.js';
const encoder = new TextEncoder();
const algorithm = {name:'RSASSA-PKCS1-v1_5', hash:'SHA-256'};
const headers = {'Cache-Control':'no-store', 'X-Content-Type-Options':'nosniff', 'Referrer-Policy':'no-referrer', 'Content-Security-Policy':"default-src 'none'; script-src 'self'; style-src 'unsafe-inline'; connect-src 'self'; form-action 'self'; frame-ancestors 'none'; base-uri 'none'"};
class Failure extends Error { constructor(status, message) {super(message); this.status=status;} }
const json = (value, status=200) => new Response(JSON.stringify(value), {status,headers:{...headers,'Content-Type':'application/json'}});
const bytes = s => Uint8Array.from(atob(s), c=>c.charCodeAt(0));
const b64 = b => btoa(String.fromCharCode(...new Uint8Array(b)));
const hex = b => [...new Uint8Array(b)].map(x=>x.toString(16).padStart(2,'0')).join('');
export async function hash(s) {return hex(await crypto.subtle.digest('SHA-256',encoder.encode(s)));}
export async function signedCode(pem, license, device, publicSpki=publicKeySpki) {
  const key = await crypto.subtle.importKey('pkcs8',bytes(pem.replace(/-----[^-]+-----|\s/g,'')),algorithm,false,['sign']);
  if(key.algorithm.modulusLength!==2048) throw new Error('wrong_signing_key');
  const mac=device.startsWith('GDDEV2-');
  const payload=mac?`GRAINS-DOSAGE-V2:${license.slice(5)}:${device.slice(7)}:3`:`GRAINS-PERMANENT-1\n${device}\n${license.slice(5)}\n3\n`;
  const signature=await crypto.subtle.sign(algorithm,key,encoder.encode(payload));
  const pub=await crypto.subtle.importKey('spki',bytes(publicSpki),algorithm,false,['verify']);
  if(!await crypto.subtle.verify(algorithm,pub,signature,encoder.encode(payload))) throw new Error('wrong_signing_key');
  return mac?`GDS2.${b64(encoder.encode(payload))}.${b64(signature)}`:payload+hex(signature)+'\n';
}
async function readBody(request) {
  if(!request.headers.get('Content-Type')?.toLowerCase().startsWith('application/json')) throw new Failure(415,'Send JSON.');
  if(!request.body) throw new Failure(400,'Missing request.');
  const reader=request.body.getReader(); const chunks=[];let size=0;
  for(;;){const {done,value}=await reader.read();if(done)break;size+=value.length;if(size>4096){await reader.cancel();throw new Failure(413,'Request too large.');}chunks.push(value);}
  const data=new Uint8Array(size);let offset=0;for(const part of chunks){data.set(part,offset);offset+=part.length;}
  try{return JSON.parse(new TextDecoder().decode(data));}catch{throw new Failure(400,'Invalid JSON.');}
}
export async function verifyPurchase(fetcher, product, key) {
  let response;
  try {response=await fetcher('https://api.gumroad.com/v2/licenses/verify',{
    method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},
    body:new URLSearchParams({product_id:product,license_key:key,increment_uses_count:'false'}),
    signal:AbortSignal.timeout(8000),redirect:'error'
  });}catch{throw new Failure(503,'Purchase verification unavailable. Please retry later.');}
  if(response.status===429||response.status>=500) throw new Failure(503,'Purchase verification unavailable. Please retry later.');
  let data;try{data=await response.json();}catch{throw new Failure(503,'Invalid purchase-service response. Please retry later.');}
  const p=data?.purchase;
  if(!response.ok||data.success!==true||!p||p.product_id!==product||typeof p.id!=='string'||!p.id||p.id.length>256||
    p.refunded!==false||p.disputed===true||p.chargebacked===true||p.license_key!==key||p.subscription_id||p.subscription_cancelled_at||p.subscription_failed_at||p.subscription_ended_at)
    throw new Failure(403,'This key is not an eligible permanent GrainsDosage purchase. Contact support.');
  return p;
}
export function createWorker({fetcher=fetch, sign=signedCode}={}) {
 return {async fetch(request, env) {
  try {
   const url=new URL(request.url);
   if(request.method==='GET'&&(url.pathname==='/'||url.pathname==='/activate.js')) return new Response(url.pathname==='/'?page:clientScript,{headers:{...headers,'Content-Type':url.pathname==='/'?'text/html; charset=utf-8':'text/javascript; charset=utf-8'}});
   if(url.pathname!=='/v1/activate') return json({error:'Not found.'},404);
   if(request.method!=='POST') return json({error:'POST required.'},405);
   const origin=request.headers.get('Origin');if(origin&&origin!==url.origin) throw new Failure(403,'Use the activation page directly.');
   if(!env.GUMROAD_PRODUCT_ID||env.GUMROAD_PRODUCT_ID.startsWith('REPLACE_')||!env.SIGNING_PRIVATE_KEY||!env.DB||!env.ACTIVATION_LIMITER) throw new Failure(503,'Activation is not configured. Contact support.');
   if(!(await env.ACTIVATION_LIMITER.limit({key:request.headers.get('CF-Connecting-IP')||'unknown'})).success) throw new Failure(429,'Too many attempts. Please wait a minute.');
   const input=await readBody(request);
   if(!input||typeof input.license_key!=='string'||typeof input.device!=='string') throw new Failure(400,'Enter your purchase key and Device Code.');
   const key=input.license_key.trim(),device=input.device.trim();
   if(!key||key.length>128||! /^[A-Za-z0-9-]+$/.test(key)||! /^(GDDEV2-)?[0-9a-f]{64}$/.test(device)) throw new Failure(400,'Invalid purchase key or Device Code format.');
   const purchase=await verifyPurchase(fetcher,env.GUMROAD_PRODUCT_ID,key);
   // Use primary reads even if D1 read replication is enabled later.
   const db=env.DB.withSession('first-primary');
   const digest=await hash(key),newId='GDL2-'+crypto.randomUUID().replaceAll('-','');
   await db.prepare('INSERT INTO licenses(id,sale_id,product_id,key_hash) VALUES(?,?,?,?) ON CONFLICT(sale_id) DO NOTHING').bind(newId,purchase.id,env.GUMROAD_PRODUCT_ID,digest).run();
   const license=await db.prepare('SELECT id,status,product_id,key_hash FROM licenses WHERE sale_id=?').bind(purchase.id).first();
   if(!license||license.status!=='active'||license.product_id!==env.GUMROAD_PRODUCT_ID||license.key_hash!==digest) throw new Failure(403,'Activation blocked. Contact support.');
   let activation=await db.prepare('SELECT code FROM activations WHERE license_id=? AND device=?').bind(license.id,device).first();
   if(!activation){
    const code=await sign(env.SIGNING_PRIVATE_KEY,license.id,device);
    try {await db.prepare('INSERT INTO activations(license_id,device,code) VALUES(?,?,?) ON CONFLICT(license_id,device) DO NOTHING').bind(license.id,device,code).run();}
    catch(e){const message=String(e.message)+' '+String(e.cause?.message);if(message.includes('device_limit')) throw new Failure(409,'All three device slots are used. Contact support.');if(message.includes('license_blocked')) throw new Failure(403,'Activation blocked. Contact support.');throw e;}
   }
   // Read after commit; never send an activation that failed to reserve a slot.
   activation=await db.prepare("SELECT a.code FROM activations a JOIN licenses l ON l.id=a.license_id WHERE a.license_id=? AND a.device=? AND l.status='active'").bind(license.id,device).first();
   if(!activation) throw new Failure(403,'Activation blocked. Contact support.');
   return json({activation_code:activation.code,filename:'GrainsDosage-Permanent-Key.txt',license_id:license.id});
  }catch(e){return json({error:e instanceof Failure?e.message:'Activation service unavailable. Contact support or retry later.'},e instanceof Failure?e.status:503);}
 }};
}
export default createWorker();
