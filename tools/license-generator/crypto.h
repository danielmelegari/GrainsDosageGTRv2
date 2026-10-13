#pragma once
#import <Foundation/Foundation.h>
#import <Security/Security.h>
#import <CommonCrypto/CommonDigest.h>
#ifdef GRAINS_GENERATOR_TEST
#include "license_test_key.h"
#else
#include "../../src/license_public_key.h"
#endif
#include <stdexcept>
#include <string>
namespace grains::issuer {
inline NSData* expectedPublic(){return [NSData dataWithBytes:aztec::license::publicKey length:sizeof(aztec::license::publicKey)];}
inline std::string hexData(NSData* data){const auto* bytes=(const unsigned char*)data.bytes;std::string s;const char* h="0123456789abcdef";for(NSUInteger i=0;i<data.length;++i){s+=h[bytes[i]>>4];s+=h[bytes[i]&15];}return s;}
inline std::string fingerprint(){unsigned char digest[CC_SHA256_DIGEST_LENGTH];NSData* data=expectedPublic();CC_SHA256(data.bytes,CC_LONG(data.length),digest);return hexData([NSData dataWithBytes:digest length:sizeof(digest)]);}
inline NSData* publicData(SecKeyRef key){
  SecKeyRef pub=key?SecKeyCopyPublicKey(key):nullptr;
  if(!pub)throw std::runtime_error("Chiave pubblica non disponibile.");
  CFDataRef der=SecKeyCopyExternalRepresentation(pub,nullptr);CFRelease(pub);
  if(!der)throw std::runtime_error("Esportazione della chiave pubblica fallita.");
  return CFBridgingRelease(der);
}
inline NSString* keyFingerprint(SecKeyRef key){NSData* data=publicData(key);unsigned char digest[CC_SHA256_DIGEST_LENGTH];CC_SHA256(data.bytes,CC_LONG(data.length),digest);return @(hexData([NSData dataWithBytes:digest length:sizeof(digest)]).c_str());}
inline bool matches(SecKeyRef key){
  if(!key)return false;SecKeyRef pub=SecKeyCopyPublicKey(key);if(!pub)return false;
  CFDataRef der=SecKeyCopyExternalRepresentation(pub,nullptr);CFRelease(pub);
  bool equal=der&&[(__bridge NSData*)der isEqualToData:expectedPublic()];if(der)CFRelease(der);return equal;
}
inline NSData* tag(){return [@"com.grainsdosage.publisher.rsa.v1" dataUsingEncoding:NSUTF8StringEncoding];}
inline NSDictionary* keyQuery(){return @{(__bridge id)kSecClass:(__bridge id)kSecClassKey,(__bridge id)kSecAttrKeyClass:(__bridge id)kSecAttrKeyClassPrivate,(__bridge id)kSecAttrKeyType:(__bridge id)kSecAttrKeyTypeRSA,(__bridge id)kSecAttrApplicationTag:tag()};}
inline SecKeyRef loadKey(){NSMutableDictionary* q=[keyQuery() mutableCopy];q[(__bridge id)kSecReturnRef]=@YES;CFTypeRef ref=nullptr;OSStatus r=SecItemCopyMatching((__bridge CFDictionaryRef)q,&ref);if(r==errSecItemNotFound)return nullptr;if(r!=errSecSuccess)throw std::runtime_error("Impossibile accedere alla chiave nel Portachiavi. Sbloccare il Portachiavi login.");return (SecKeyRef)ref;}
inline SecKeyRef importKey(NSData* pem,bool securePrompt=true){
  SecExternalFormat format=kSecFormatUnknown;SecExternalItemType type=kSecItemTypePrivateKey;
  SecItemImportExportKeyParameters params={};params.version=SEC_KEY_IMPORT_EXPORT_PARAMS_VERSION;
  if(securePrompt){params.flags=kSecKeySecurePassphrase;params.alertTitle=CFSTR("GrainsDosage — chiave produttore");params.alertPrompt=CFSTR("Password della chiave privata esistente");}
  CFArrayRef result=nullptr;OSStatus status=SecItemImport((__bridge CFDataRef)pem,nullptr,&format,&type,0,&params,nullptr,&result);
  if(status!=errSecSuccess||!result){if(result)CFRelease(result);throw std::runtime_error("Importazione RSA fallita. Usare la chiave privata PEM originale (RSA 2048), con la sua password se cifrata.");}
  SecKeyRef key=nullptr;
  if(CFArrayGetCount(result)==1&&CFGetTypeID(CFArrayGetValueAtIndex(result,0))==SecKeyGetTypeID()){key=(SecKeyRef)CFArrayGetValueAtIndex(result,0);CFRetain(key);}CFRelease(result);
  if(!matches(key)){if(key)CFRelease(key);throw std::runtime_error("Questa chiave NON corrisponde alla chiave pubblica del plugin. Nessuna chiave è stata sostituita.");}return key;
}
inline void storeKey(SecKeyRef key){
  if(!matches(key))throw std::runtime_error("Chiave non corrispondente.");
  NSMutableDictionary* q=[keyQuery() mutableCopy];q[(__bridge id)kSecValueRef]=(__bridge id)key;q[(__bridge id)kSecAttrIsPermanent]=@YES;q[(__bridge id)kSecAttrLabel]=@"GrainsDosage Publisher — RSA signing";
  // Default login Keychain ACL requires the user's authorization for access by
  // another application. Never create an allow-all trusted-application list.
  OSStatus r=SecItemAdd((__bridge CFDictionaryRef)q,nullptr);
  if(r!=errSecSuccess&&r!=errSecDuplicateItem)throw std::runtime_error("Salvataggio della chiave nel Portachiavi fallito.");
  SecKeyRef saved=loadKey();bool valid=matches(saved);if(saved)CFRelease(saved);if(!valid)throw std::runtime_error("Il Portachiavi contiene un'altra chiave: configurazione interrotta.");
}
inline NSDictionary* configQuery(){return @{(__bridge id)kSecClass:(__bridge id)kSecClassGenericPassword,(__bridge id)kSecAttrService:@"com.grainsdosage.publisher.config.v1",(__bridge id)kSecAttrAccount:@"ledger"};}
inline NSString* ledgerPath(){NSMutableDictionary* q=[configQuery() mutableCopy];q[(__bridge id)kSecReturnData]=@YES;CFTypeRef out=nullptr;OSStatus r=SecItemCopyMatching((__bridge CFDictionaryRef)q,&out);if(r==errSecItemNotFound)return nil;if(r!=errSecSuccess)throw std::runtime_error("Impossibile leggere la configurazione nel Portachiavi.");NSData* d=CFBridgingRelease(out);NSString* path=[[NSString alloc] initWithData:d encoding:NSUTF8StringEncoding];if(!path.length)throw std::runtime_error("Configurazione del registro danneggiata.");return path;}
inline void storeLedgerPath(NSString* path){NSMutableDictionary* q=[configQuery() mutableCopy];q[(__bridge id)kSecValueData]=[path dataUsingEncoding:NSUTF8StringEncoding];if(SecItemAdd((__bridge CFDictionaryRef)q,nullptr)!=errSecSuccess)throw std::runtime_error("Configurazione registro già presente o non salvabile. Non è stata sostituita.");}
inline NSDictionary* enrollmentQuery(){NSMutableDictionary* q=[configQuery() mutableCopy];q[(__bridge id)kSecAttrAccount]=@"new-key-enrollment";return q;}
inline bool freshKey(SecKeyRef key){
  NSMutableDictionary* q=[enrollmentQuery() mutableCopy];q[(__bridge id)kSecReturnData]=@YES;
  CFTypeRef out=nullptr;OSStatus r=SecItemCopyMatching((__bridge CFDictionaryRef)q,&out);
  if(r==errSecItemNotFound)return false;
  if(r!=errSecSuccess)throw std::runtime_error("Impossibile leggere lo stato della configurazione iniziale.");
  NSData* data=CFBridgingRelease(out);return [data isEqualToData:[keyFingerprint(key) dataUsingEncoding:NSUTF8StringEncoding]];
}
inline void finishEnrollment(){SecItemDelete((__bridge CFDictionaryRef)enrollmentQuery());}
// Creation happens ONLY on the publisher's Mac. No private material is exported
// to the enrollment request, source tree, CI, application bundle or logs.
inline SecKeyRef createPublisherKey(){
  SecKeyRef existing=loadKey();if(existing){CFRelease(existing);throw std::runtime_error("Una chiave esiste già. Non verrà sostituita: usare Esporta chiave pubblica.");}
  if(ledgerPath())throw std::runtime_error("Un registro è già configurato. Ripristinare la chiave originale; non crearne un'altra.");
  NSDictionary* attrs=@{(__bridge id)kSecAttrKeyType:(__bridge id)kSecAttrKeyTypeRSA,(__bridge id)kSecAttrKeySizeInBits:@2048};
  CFErrorRef error=nullptr;SecKeyRef key=SecKeyCreateRandomKey((__bridge CFDictionaryRef)attrs,&error);
  if(!key){if(error)CFRelease(error);throw std::runtime_error("Creazione RSA nel Mac fallita.");}
  NSMutableDictionary* q=[keyQuery() mutableCopy];q[(__bridge id)kSecValueRef]=(__bridge id)key;q[(__bridge id)kSecAttrIsPermanent]=@YES;q[(__bridge id)kSecAttrLabel]=@"GrainsDosage Publisher — RSA signing";
  OSStatus r=SecItemAdd((__bridge CFDictionaryRef)q,nullptr);
  if(r!=errSecSuccess){CFRelease(key);throw std::runtime_error("La chiave non è stata salvata. Nessuna chiave esistente è stata sostituita.");}
  NSMutableDictionary* enrollment=[enrollmentQuery() mutableCopy];enrollment[(__bridge id)kSecValueData]=[keyFingerprint(key) dataUsingEncoding:NSUTF8StringEncoding];
  // There must be no stale enrollment record before starting a fresh key.
  r=SecItemAdd((__bridge CFDictionaryRef)enrollment,nullptr);
  if(r!=errSecSuccess){CFRelease(key);throw std::runtime_error("Chiave salvata nel Portachiavi, ma configurazione incompleta. Non rigenerarla: esportare la chiave pubblica per assistenza.");}
  CFRelease(key);SecKeyRef saved=loadKey();
  if(!saved)throw std::runtime_error("La chiave non è accessibile nel Portachiavi. Non crearne un’altra; controllare il Portachiavi login.");
  return saved;
}
inline NSData* publicEnrollment(SecKeyRef key){
  NSDictionary* data=@{@"format":@"GrainsDosage-Public-Key-v1",@"algorithm":@"RSA-2048-SHA256-PKCS1-v1_5",@"public_key_pkcs1_der_base64":[publicData(key) base64EncodedStringWithOptions:0],@"sha256":keyFingerprint(key)};
  NSData* json=[NSJSONSerialization dataWithJSONObject:data options:NSJSONWritingPrettyPrinted error:nil];
  if(!json)throw std::runtime_error("Impossibile creare la richiesta pubblica.");return json;
}
inline NSData* encryptedBackup(SecKeyRef key,CFStringRef testPassphrase=nullptr){
  SecItemImportExportKeyParameters params={};params.version=SEC_KEY_IMPORT_EXPORT_PARAMS_VERSION;
  if(testPassphrase)params.passphrase=testPassphrase;
  else{params.flags=kSecKeySecurePassphrase;params.alertTitle=CFSTR("Backup privato GrainsDosage");params.alertPrompt=CFSTR("Scegli una password per proteggere il backup. Conservala separatamente.");}
  CFDataRef out=nullptr;OSStatus r=SecItemExport(key,kSecFormatWrappedPKCS8,kSecItemPemArmour,&params,&out);
  if(r!=errSecSuccess||!out){if(out)CFRelease(out);throw std::runtime_error("Backup cifrato annullato o fallito. La chiave resta nel Portachiavi.");}
  return CFBridgingRelease(out);
}
inline std::string signPayload(SecKeyRef key,const std::string& payload,bool mac){
  if(!matches(key))throw std::runtime_error("La chiave non corrisponde al plugin.");
  NSData* data=[NSData dataWithBytes:payload.data() length:payload.size()];CFErrorRef error=nullptr;
  CFDataRef signature=SecKeyCreateSignature(key,kSecKeyAlgorithmRSASignatureMessagePKCS1v15SHA256,(__bridge CFDataRef)data,&error);
  if(!signature){if(error)CFRelease(error);throw std::runtime_error("Firma RSA negata o fallita. Controllare il Portachiavi.");}
  NSData* sig=CFBridgingRelease(signature);SecKeyRef pub=SecKeyCopyPublicKey(key);
  bool verified=pub&&SecKeyVerifySignature(pub,kSecKeyAlgorithmRSASignatureMessagePKCS1v15SHA256,(__bridge CFDataRef)data,(__bridge CFDataRef)sig,nullptr);if(pub)CFRelease(pub);
  if(!verified||sig.length!=256)throw std::runtime_error("Verifica della firma fallita: nessun codice esportato.");
  if(mac)return "GDS2."+std::string([data base64EncodedStringWithOptions:0].UTF8String)+"."+std::string([sig base64EncodedStringWithOptions:0].UTF8String)+"\n";
  return payload+hexData(sig)+"\n";
}
}
