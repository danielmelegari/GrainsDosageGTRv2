#include "crypto.h"
#include "ledger.h"
#include "../../src/trial_license.h"
#include <cassert>
#include <fstream>
#include <iostream>
using namespace grains::issuer;
static void write(const std::string& path,const std::string& value){std::ofstream file(path);file<<value;assert(file.good());}
int main(int argc,char** argv){@autoreleasepool{
 assert(argc==2);const std::string root=argv[1];NSData* pem=[NSData dataWithContentsOfFile:@((root+"/private.pem").c_str())];SecKeyRef key=importKey(pem,false);assert(matches(key));
 // Isolated Keychain exercises one-time persistence and normal signing lookup.
 SecKeychainRef previous=nullptr,temporary=nullptr;CFArrayRef previousSearch=nullptr;
 assert(SecKeychainCopyDefault(&previous)==errSecSuccess);assert(SecKeychainCopySearchList(&previousSearch)==errSecSuccess);
 const std::string keychainPath=root+"/issuer-test.keychain";const char password[]="temporary-test-only";
 assert(SecKeychainCreate(keychainPath.c_str(),sizeof(password)-1,password,false,nullptr,&temporary)==errSecSuccess);
 assert(SecKeychainSetDefault(temporary)==errSecSuccess);
 const void* entry=temporary;CFArrayRef list=CFArrayCreate(nullptr,&entry,1,&kCFTypeArrayCallBacks);assert(SecKeychainSetSearchList(list)==errSecSuccess);CFRelease(list);
 // First-time publishers have no PEM. A fresh key persists without replacing an
 // existing key; only public material is enrolled and old builds cannot sign it.
 SecKeyRef created=createPublisherKey();assert(created);assert(!matches(created));assert(freshKey(created));
 NSData* enrollment=publicEnrollment(created);NSDictionary* request=[NSJSONSerialization JSONObjectWithData:enrollment options:0 error:nil];
 assert(request.count==4);assert([request[@"format"] isEqualToString:@"GrainsDosage-Public-Key-v1"]);
 assert([request[@"sha256"] isEqualToString:keyFingerprint(created)]);
 assert([[[NSData alloc] initWithBase64EncodedString:request[@"public_key_pkcs1_der_base64"] options:0] isEqualToData:publicData(created)]);
 assert([enrollment writeToFile:@((root+"/enrollment.json").c_str()) atomically:YES]);
 NSData* backup=encryptedBackup(created,CFSTR("temporary-backup-test-only"));
 assert([backup writeToFile:@((root+"/enrollment-backup.pem").c_str()) atomically:YES]);
 SecKeyRef reloaded=loadKey();assert(reloaded);assert([publicData(reloaded) isEqualToData:publicData(created)]);CFRelease(reloaded);
 bool creationRejected=false;try{SecKeyRef unexpected=createPublisherKey();CFRelease(unexpected);}catch(...){creationRejected=true;}assert(creationRejected);
 bool signingRejected=false;try{signPayload(created,"must not sign for unmatched build",true);}catch(...){signingRejected=true;}assert(signingRejected);
 testNoPublicCopy=true;
 for(bool legacySigning:{false,true}){
  testLegacySigning=legacySigning;
  assert(!matches(created));
  bool exportRejected=false;try{publicEnrollment(created);}catch(...){exportRejected=true;}assert(exportRejected);
  bool signatureRejected=false;try{signPayload(created,"wrong persisted key",true);}catch(...){signatureRejected=true;}assert(signatureRejected);
 }
 testNoPublicCopy=false;testLegacySigning=false;
 // Simulate completing the one-time setup: the first-issuance permission ends.
 finishEnrollment();assert(!freshKey(created));CFRelease(created);
 assert(SecItemDelete((__bridge CFDictionaryRef)keyQuery())==errSecSuccess);
 std::cout<<"PASS: first-time key creation, persistent reload, public-only enrollment, encrypted backup, no key replacement and unmatched-build rejection\n";
 storeKey(key);SecKeyRef restored=loadKey();assert(matches(restored));assert(!signPayload(restored,"test persistence",true).empty());
 testNoPublicCopy=true;
 for(bool legacySigning:{false,true}){
  testLegacySigning=legacySigning;
  assert(matches(restored));assert([publicData(restored) isEqualToData:expectedPublic()]);
  assert([keyFingerprint(restored) isEqualToString:@(fingerprint().c_str())]);
  NSDictionary* exported=[NSJSONSerialization JSONObjectWithData:publicEnrollment(restored) options:0 error:nil];
  assert([exported[@"sha256"] isEqualToString:@(fingerprint().c_str())]);
  assert(!signPayload(restored,"test persisted key without public copy",true).empty());
  NSData* message=[@"signed message" dataUsingEncoding:NSUTF8StringEncoding];NSData* signature=signMessage(restored,message);
  assert(verifiesExpected(message,signature));
  assert(!verifiesExpected([@"altered message" dataUsingEncoding:NSUTF8StringEncoding],signature));
 }
 CFRelease(restored);
 // Keep the forced legacy/no-public-copy path enabled below: real license
 // fixtures are independently checked by OpenSSL and both plugin editions.
 std::cout<<"PASS: missing public-key copy uses a verified challenge; current/legacy signing works and wrong keys/altered messages stay rejected\n";
 storeLedgerPath(@((root+"/generator.sqlite").c_str()));assert([ledgerPath() isEqualToString:@((root+"/generator.sqlite").c_str())]);
 // Losing the private key must never silently create a replacement for an
 // already configured publisher ledger.
 assert(SecItemDelete((__bridge CFDictionaryRef)keyQuery())==errSecSuccess);
 bool configuredRejected=false;try{SecKeyRef unexpected=createPublisherKey();CFRelease(unexpected);}catch(...){configuredRejected=true;}assert(configuredRejected);assert(!loadKey());storeKey(key);
 Ledger db(root+"/generator.sqlite",fingerprint(),true);Grant g;const std::string device="GDDEV2-"+std::string(64,'a'),id="GDL2-0123456789abcdef0123456789abcdef";
 const auto signMac=[&](const Grant& grant){return signPayload(key,grant.payload,true);};
 write(root+"/serial",db.issue(false,device,"Test customer","",id,1700000000,signMac,g));
 const int64_t now=(int64_t)[NSDate.date timeIntervalSince1970];
 write(root+"/signed-trial",db.issue(true,device,"","","",now-10,signMac,g));
 write(root+"/expired-trial",signPayload(key,"GRAINS-DOSAGE-TRIAL-V2:"+std::string(64,'a')+":"+std::to_string(now-1209700)+":"+std::to_string(now-100),true));
 write(root+"/future-trial",signPayload(key,"GRAINS-DOSAGE-TRIAL-V2:"+std::string(64,'a')+":"+std::to_string(now+100)+":"+std::to_string(now+1209700),true));
 write(root+"/wrong-trial-period",signPayload(key,"GRAINS-DOSAGE-TRIAL-V2:"+std::string(64,'a')+":"+std::to_string(now-10)+":"+std::to_string(now+3600),true));
 write(root+"/wrong-trial-device",signPayload(key,"GRAINS-DOSAGE-TRIAL-V2:"+std::string(64,'b')+":"+std::to_string(now-10)+":"+std::to_string(now-10+1209600),true));
 for(bool trial:{false,true}){
  auto code=db.issue(trial,std::string(64,'c'),"Test customer","",id,now,[&](const Grant& grant){return signPayload(key,grant.payload,false);},g);
  grains::trial::License license;assert(grains::trial::parseLicense(code,license));assert(license.permanent==!trial);
  write(root+(trial?"/windows-trial":"/windows-permanent"),code);
 }
 NSData* wrong=[NSData dataWithContentsOfFile:@((root+"/wrong-private.pem").c_str())];bool rejected=false;try{SecKeyRef bad=importKey(wrong,false);CFRelease(bad);}catch(...){rejected=true;}assert(rejected);
 CFRelease(key);
 assert(SecKeychainSetDefault(previous)==errSecSuccess);assert(SecKeychainSetSearchList(previousSearch)==errSecSuccess);
 assert(SecKeychainDelete(temporary)==errSecSuccess);CFRelease(temporary);CFRelease(previous);CFRelease(previousSearch);
 std::cout<<"PASS: native generator imports matching RSA, rejects another key, signs both platforms and verifies its signatures\n";
}return 0;}
