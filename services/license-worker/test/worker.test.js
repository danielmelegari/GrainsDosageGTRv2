import {test} from 'node:test';
import assert from 'node:assert/strict';
import {DatabaseSync} from 'node:sqlite';
import {readFileSync} from 'node:fs';
import {generateKeyPairSync,createVerify} from 'node:crypto';
import {createWorker,signedCode,hash} from '../src/index.js';
const {privateKey,publicKey}=generateKeyPairSync('rsa',{modulusLength:2048});
const pem=privateKey.export({type:'pkcs8',format:'pem'}),spki=publicKey.export({type:'spki',format:'der'}).toString('base64');
const key='ABCD-1234-EFGH-5678', product='product==';
const purchase={id:'sale-id',product_id:product,license_key:key,refunded:false,disputed:false,chargebacked:false};
const device=i=>i.toString(16).padStart(64,'0');
function fixture(overrides={}){
 const sqlite=new DatabaseSync(':memory:');sqlite.exec(readFileSync(new URL('../migrations/0001_licenses.sql',import.meta.url),'utf8'));
 const db={
  withSession(){return this;},
  prepare(sql){
   const q=sqlite.prepare(sql);
   return {bind(...args){return {
    async run(){return q.run(...args);},
    async first(){return q.get(...args)||null;}
   };}};
  }
 };
 let current=purchase;
 const fetcher=async(url,options)=>{assert.equal(url,'https://api.gumroad.com/v2/licenses/verify');assert.equal(options.body.get('increment_uses_count'),'false');return Response.json({success:true,purchase:current});};
 const worker=createWorker({fetcher,sign:(p,l,d)=>signedCode(p,l,d,spki),...overrides});
 const env={DB:db,GUMROAD_PRODUCT_ID:product,SIGNING_PRIVATE_KEY:pem,ACTIVATION_LIMITER:{async limit(){return{success:true};}}};
 async function request(d=device(1),k=key,extra={}){const r=await worker.fetch(new Request('https://licenses.example/v1/activate',{method:'POST',headers:{'Content-Type':'application/json',...extra},body:JSON.stringify({license_key:k,device:d})}),env);return{status:r.status,body:await r.json()};}
 return{sqlite,env,request,worker,setPurchase:p=>current=p};
}
function verify(message,signature){const v=createVerify('RSA-SHA256');v.update(message);return v.verify(publicKey,signature);}
test('Mac and Windows signatures match existing payload contracts',async()=>{
 const f=fixture();const mac=await f.request('GDDEV2-'+device(1));assert.equal(mac.status,200);
 const parts=mac.body.activation_code.split('.'),payload=Buffer.from(parts[1],'base64').toString();
 assert.equal(payload,`GRAINS-DOSAGE-V2:${mac.body.license_id.slice(5)}:${device(1)}:3`);assert.ok(verify(payload,Buffer.from(parts[2],'base64')));
 const win=await f.request(device(2));assert.equal(win.status,200);const lines=win.body.activation_code.split('\n');assert.deepEqual(lines.slice(0,4),['GRAINS-PERMANENT-1',device(2),win.body.license_id.slice(5),'3']);assert.ok(verify(lines.slice(0,4).join('\n')+'\n',Buffer.from(lines[4],'hex')));
 assert.equal(f.sqlite.prepare('SELECT key_hash FROM licenses').get().key_hash,await hash(key));
});
test('concurrent requests enforce three devices across platforms and repeat uses same slot',async()=>{
 const f=fixture();const ids=[device(1),'GDDEV2-'+device(2),device(3),device(4)];const results=await Promise.all(ids.map(d=>f.request(d)));
 assert.deepEqual(results.map(r=>r.status).sort(),[200,200,200,409]);const allowed=ids[results.findIndex(r=>r.status===200)];
 const retries=await Promise.all(Array.from({length:8},()=>f.request(allowed)));assert.ok(retries.every(r=>r.status===200));assert.equal(new Set(retries.map(r=>r.body.activation_code)).size,1);assert.equal(f.sqlite.prepare('SELECT count(*) n FROM activations').get().n,3);
 assert.throws(()=>f.sqlite.exec('DELETE FROM activations'),/cannot_be_deleted/);
});
test('ineligible purchases fail closed',async()=>{
 for(const update of [{refunded:true},{disputed:true},{chargebacked:true},{product_id:'other'},{license_key:'other'},{subscription_id:'sub'},{refunded:undefined}]){const f=fixture();f.setPurchase({...purchase,...update});assert.equal((await f.request()).status,403);assert.equal(f.sqlite.prepare('SELECT count(*) n FROM activations').get().n,0);}
});
test('refund and local block prevent redownload and new activations',async()=>{
 const f=fixture();assert.equal((await f.request()).status,200);f.setPurchase({...purchase,refunded:true});assert.equal((await f.request()).status,403);
 f.setPurchase(purchase);f.sqlite.exec("UPDATE licenses SET status='blocked'");assert.equal((await f.request()).status,403);assert.equal((await f.request(device(2))).status,403);
});
test('wrong signing key cannot reserve a device or expose a code',async()=>{const f=fixture({sign:signedCode});const r=await f.request();assert.equal(r.status,503);assert.ok(!r.body.activation_code);assert.equal(f.sqlite.prepare('SELECT count(*) n FROM activations').get().n,0);});
test('upstream outages and rate limits do not issue codes',async()=>{
 for(const fetcher of [async()=>{throw new Error('network secret');},async()=>new Response('',{status:429}),async()=>new Response('bad-json'),async()=>new Response('',{status:500})]){const f=fixture({fetcher});const r=await f.request();assert.equal(r.status,503);assert.ok(!JSON.stringify(r).includes('secret'));}
 const f=fixture();f.env.ACTIVATION_LIMITER.limit=async()=>({success:false});assert.equal((await f.request()).status,429);delete f.env.ACTIVATION_LIMITER;assert.equal((await f.request()).status,503);
});
test('input, origin and configuration validation',async()=>{const f=fixture();assert.equal((await f.request('invalid')).status,400);assert.equal((await f.request(device(1),key,{Origin:'https://evil.example'})).status,403);f.env.GUMROAD_PRODUCT_ID='REPLACE_WITH_EXACT_PRODUCT_ID';assert.equal((await f.request()).status,503);});
test('same-device concurrent first requests persist one code',async()=>{const f=fixture();const results=await Promise.all(Array.from({length:12},()=>f.request()));assert.ok(results.every(r=>r.status===200));assert.equal(f.sqlite.prepare('SELECT count(*) n FROM activations').get().n,1);assert.equal(new Set(results.map(r=>r.body.activation_code)).size,1);});
test('database commit failure never exposes generated code',async()=>{const f=fixture();f.sqlite.exec("CREATE TRIGGER fail BEFORE INSERT ON activations BEGIN SELECT RAISE(ABORT,'disk failure'); END;");const r=await f.request();assert.equal(r.status,503);assert.ok(!r.body.activation_code);});
