// DSGMachines: perpetual 1.x entitlement with six-calendar-month offline certificates.
export const PRODUCT_ID='tl1D89RY6g0H5blD-izZ3Q==';
const ECDSA={name:'ECDSA',namedCurve:'P-256'}, SIGN={name:'ECDSA',hash:'SHA-256'};
const enc=new TextEncoder();
const response=(body,status=200)=>new Response(JSON.stringify(body),{status,headers:{'Content-Type':'application/json','Cache-Control':'no-store','X-Content-Type-Options':'nosniff'}});
export const b64url=b=>btoa(String.fromCharCode(...new Uint8Array(b))).replace(/\+/g,'-').replace(/\//g,'_').replace(/=+$/,'');
export const decode=s=>Uint8Array.from(atob(s.replace(/-/g,'+').replace(/_/g,'/')),c=>c.charCodeAt(0));
const pemBytes=pem=>decode(pem.replace(/-----[^-]+-----|\s/g,''));
export async function digest(s){return [...new Uint8Array(await crypto.subtle.digest('SHA-256',enc.encode(s)))].map(b=>b.toString(16).padStart(2,'0')).join('');}
async function keyedHash(secret,s){const k=await crypto.subtle.importKey('raw',enc.encode(secret),{name:'HMAC',hash:'SHA-256'},false,['sign']);return b64url(await crypto.subtle.sign('HMAC',k,enc.encode(s)));}
export function expiresAt(now){const d=new Date(now),day=d.getUTCDate();d.setUTCDate(1);d.setUTCMonth(d.getUTCMonth()+6);const end=new Date(Date.UTC(d.getUTCFullYear(),d.getUTCMonth()+1,0)).getUTCDate();d.setUTCDate(Math.min(day,end));return d.toISOString();}
class Failure extends Error{constructor(status,message){super(message);this.status=status;}}
async function boundedJson(stream,limit){if(!stream)throw new Failure(400,'invalid_json');const reader=stream.getReader(),parts=[];let size=0;for(;;){const {done,value}=await reader.read();if(done)break;size+=value.length;if(size>limit){await reader.cancel();throw new Failure(413,'too_large');}parts.push(value);}const bytes=new Uint8Array(size);let pos=0;for(const p of parts){bytes.set(p,pos);pos+=p.length;}try{return JSON.parse(new TextDecoder().decode(bytes));}catch{throw new Failure(400,'invalid_json');}}
// Canonical proof is unambiguous: exact UTF-8 JSON array, not object property order.
export function proofMessage(path,b){return JSON.stringify(['DSGMachines-request-v1',path,b.license_key,b.machine_id,b.plugin_version,b.device_public_key,b.timestamp,b.nonce,b.old_machine_id||'']);}
async function verifyProof(spki,signature,message){try{if(!/^[A-Za-z0-9_-]+$/.test(spki)||!/^[A-Za-z0-9_-]{86}$/.test(signature))return false;const k=await crypto.subtle.importKey('spki',decode(spki),ECDSA,false,['verify']);return await crypto.subtle.verify(SIGN,k,decode(signature),enc.encode(message));}catch{return false;}}
async function signer(env){const key=await crypto.subtle.importKey('pkcs8',pemBytes(env.SIGNING_PRIVATE_KEY),ECDSA,false,['sign']);const pub=await crypto.subtle.importKey('spki',pemBytes(env.SIGNING_PUBLIC_KEY),ECDSA,false,['verify']);return async payload=>{const text=b64url(enc.encode(JSON.stringify({alg:'ES256',typ:'JWT',kid:env.SIGNING_KEY_ID})))+'.'+b64url(enc.encode(JSON.stringify(payload)));const sig=await crypto.subtle.sign(SIGN,key,enc.encode(text));if(!await crypto.subtle.verify(SIGN,pub,sig,enc.encode(text)))throw Error('key_mismatch');return text+'.'+b64url(sig);};}
export async function purchase(fetcher,key){let r;try{r=await fetcher('https://api.gumroad.com/v2/licenses/verify',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:new URLSearchParams({product_id:PRODUCT_ID,license_key:key,increment_uses_count:'false'}),signal:AbortSignal.timeout(8000),redirect:'manual'});}catch{throw new Failure(503,'purchase_verification_unavailable');}
 if((r.status>=300&&r.status<400)||r.status===429||r.status>=500)throw new Failure(503,'purchase_verification_unavailable');
 if(!r.ok)throw new Failure(403,'invalid_or_refunded_key');let d;try{d=await boundedJson(r.body,131072);}catch{throw new Failure(503,'purchase_verification_unavailable');}const p=d?.purchase;
 if(d.success!==true||!p||typeof p.id!=='string'||!p.id||p.id.length>256||typeof p.email!=='string'||!p.email||p.email.length>320||p.product_id!==PRODUCT_ID||p.license_key!==key||p.refunded!==false||p.disputed===true||p.chargebacked===true||p.chargeback===true||p.subscription_id||p.subscription_cancelled_at||p.subscription_failed_at||p.subscription_ended_at)throw new Failure(403,'invalid_or_refunded_key');
 return {id:p.id,email:p.email.trim().toLowerCase()};}
export function createWorker({fetcher=fetch,now=()=>Date.now()}={}){return {async fetch(request,env){try{
 const path=new URL(request.url).pathname;
 if(path==='/health'&&request.method==='GET'){const db=env.DB.withSession('first-primary');await db.prepare('SELECT device_public_key FROM activations LIMIT 1').first();const ready=!!(env.SIGNING_PRIVATE_KEY&&env.MACHINE_PEPPER&&env.SIGNING_PUBLIC_KEY&&env.SIGNING_KEY_ID&&env.IP_LIMITER&&env.KEY_LIMITER);if(ready){if(!(await env.IP_LIMITER.limit({key:request.headers.get('CF-Connecting-IP')||'unknown'})).success)throw new Failure(429,'rate_limited');const sign=await signer(env);await sign({purpose:'signing-readiness-check'});}return response({service:'dsgmachines-license-api',database:'connected',ready,protocol:2,key_id:env.SIGNING_KEY_ID},ready?200:503);}
 if(!['/v1/activate','/v1/verify','/v1/replace'].includes(path))throw new Failure(404,'not_found');if(request.method!=='POST')throw new Failure(405,'method_not_allowed');
 if(!env.MACHINE_PEPPER||!env.SIGNING_PRIVATE_KEY||!env.SIGNING_PUBLIC_KEY||!env.SIGNING_KEY_ID||!env.IP_LIMITER||!env.KEY_LIMITER)throw new Failure(503,'service_not_ready');
 if(!(await env.IP_LIMITER.limit({key:request.headers.get('CF-Connecting-IP')||'unknown'})).success)throw new Failure(429,'rate_limited');
 if(!request.headers.get('Content-Type')?.toLowerCase().startsWith('application/json'))throw new Failure(415,'json_required');
 const b=await boundedJson(request.body,8192),at=now();
 if(!b||typeof b.license_key!=='string'||! /^[A-Za-z0-9-]{8,200}$/.test(b.license_key)||typeof b.machine_id!=='string'||! /^(GDDEV2-)?[0-9a-f]{64}$/.test(b.machine_id)||typeof b.plugin_version!=='string'||! /^1\.(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)(?:[-+][A-Za-z0-9.-]+)?$/.test(b.plugin_version)||typeof b.device_public_key!=='string'||b.device_public_key.length>256||typeof b.signature!=='string'||!Number.isSafeInteger(b.timestamp)||Math.abs(at/1000-b.timestamp)>300||typeof b.nonce!=='string'||! /^[0-9a-f]{32}$/.test(b.nonce))throw new Failure(400,'invalid_input_or_version');
 const keyHash=await keyedHash(env.MACHINE_PEPPER,'license:'+b.license_key);
 if(!(await env.KEY_LIMITER.limit({key:keyHash})).success)throw new Failure(429,'rate_limited');
 const message=proofMessage(path,b);if(!await verifyProof(b.device_public_key,b.signature,message))throw new Failure(403,'invalid_device_proof');
 if(path==='/v1/replace'&&(typeof b.old_machine_id!=='string'||! /^(GDDEV2-)?[0-9a-f]{64}$/.test(b.old_machine_id)||b.old_machine_id===b.machine_id||typeof b.old_signature!=='string'))throw new Failure(400,'old_device_proof_required');
 const sale=await purchase(fetcher,b.license_key);
 const sign=await signer(env); // Fail before changing any device records if config is invalid.
 await sign({probe:true});
 const db=env.DB.withSession('first-primary'),customer=await keyedHash(env.MACHINE_PEPPER,'customer:'+sale.email);
 await db.batch([db.prepare('INSERT OR IGNORE INTO customers(id,email) VALUES (?,?)').bind(customer,sale.email),db.prepare("INSERT OR IGNORE INTO licenses(id,customer_id,product,license_hash,gumroad_sale_id) VALUES (?,?,'GrainsDosage',?,?)").bind(crypto.randomUUID(),customer,keyHash,sale.id)]);
 const license=await db.prepare('SELECT * FROM licenses WHERE license_hash=? AND gumroad_sale_id=?').bind(keyHash,sale.id).first();if(!license||license.status!=='active')throw new Failure(403,'license_inactive');
 const mh=await keyedHash(env.MACHINE_PEPPER,'machine:'+b.machine_id);
 let a=await db.prepare('SELECT * FROM activations WHERE license_id=? AND machine_id_hash=?').bind(license.id,mh).first();
 if(a&&(a.status!=='active'||a.device_public_key!==b.device_public_key))throw new Failure(403,'device_revoked_or_key_mismatch');
 if(path==='/v1/replace'){
  const oldHash=await keyedHash(env.MACHINE_PEPPER,'machine:'+b.old_machine_id);
  const old=await db.prepare('SELECT * FROM activations WHERE license_id=? AND machine_id_hash=?').bind(license.id,oldHash).first();
  if(!old||!await verifyProof(old.device_public_key,b.old_signature,message))throw new Failure(403,'old_device_proof_required');
  const prior=await db.prepare('SELECT * FROM device_replacements WHERE license_id=?').bind(license.id).first();
  if(prior){if(!a||prior.old_activation_id!==old.id||prior.new_activation_id!==a.id)throw new Failure(409,'replacement_limit');}
  else{if(a)throw new Failure(409,'new_device_already_active');try{await db.prepare('INSERT INTO device_replacements(license_id,old_activation_id,new_activation_id,new_machine_hash,new_public_key) VALUES(?,?,?,?,?)').bind(license.id,old.id,crypto.randomUUID(),mh,b.device_public_key).run();}catch{throw new Failure(409,'replacement_conflict_or_limit');}}
 }else if(!a){
  if(path==='/v1/verify')throw new Failure(403,'device_not_activated');
  try{await db.prepare("INSERT INTO activations(id,license_id,machine_id_hash,device_public_key,status) VALUES(?,?,?,?,'active') ON CONFLICT(license_id,machine_id_hash) DO NOTHING").bind(crypto.randomUUID(),license.id,mh,b.device_public_key).run();}catch{throw new Failure(409,'device_limit_or_conflict');}
 }
 a=await db.prepare("SELECT a.* FROM activations a JOIN licenses l ON l.id=a.license_id WHERE a.license_id=? AND a.machine_id_hash=? AND a.status='active' AND l.status='active'").bind(license.id,mh).first();
 if(!a||a.device_public_key!==b.device_public_key)throw new Failure(403,'device_revoked_or_key_mismatch');
 const expiry=expiresAt(at),payload={iss:'DSGMachines',aud:'GrainsDosage',product:'GrainsDosage',major:1,license_id:license.id,activation_id:a.id,machine_hash:await digest(b.machine_id),device_key_hash:await digest(b.device_public_key),issued_at:new Date(at).toISOString(),expires_at:expiry,iat:Math.floor(at/1000),exp:Math.floor(Date.parse(expiry)/1000),perpetual:true};
 const certificate=await sign(payload);
 await db.prepare('UPDATE activations SET last_seen_at=CURRENT_TIMESTAMP WHERE id=?').bind(a.id).run();
 return response({ok:true,protocol:2,certificate,expires_at:expiry,license_type:'perpetual',major_version:1,max_devices:license.max_devices,replacements_allowed:license.replacements_allowed});
 }catch(e){return response({error:e instanceof Failure?e.message:'service_unavailable'},e instanceof Failure?e.status:503);}}};}
export default createWorker();
