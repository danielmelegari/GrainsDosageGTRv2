#pragma once
// macOS verification of the same publisher-signed GRAINS-TRIAL-1 licenses
// used on Windows. No signing/private key is shipped with the plugin.
#include "trial_license.h"
#ifndef GRAINS_TRIAL_PUBLIC_KEY_HEADER
#error "Set GRAINS_TRIAL_PUBLIC_KEY_HEADER"
#endif
#include GRAINS_TRIAL_PUBLIC_KEY_HEADER
#include <CommonCrypto/CommonDigest.h>
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <Security/Security.h>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <sys/file.h>
#include <fcntl.h>
#include <unistd.h>

namespace grains::trial {
inline Tick wallTime() {
  constexpr Tick offset=116444736000000000ULL;
  const auto now=std::chrono::system_clock::now().time_since_epoch();
  return offset+Tick(std::chrono::duration_cast<std::chrono::nanoseconds>(now).count()/100);
}
inline std::string machineId() {
  io_service_t service=IOServiceGetMatchingService(kIOMasterPortDefault, IOServiceMatching("IOPlatformExpertDevice"));
  if(!service) return {};
  CFTypeRef uuid=IORegistryEntryCreateCFProperty(service, CFSTR("IOPlatformUUID"), kCFAllocatorDefault, 0);
  IOObjectRelease(service);
  if(!uuid) return {};
  char text[128]{};
  bool ok=CFGetTypeID(uuid)==CFStringGetTypeID() &&
          CFStringGetCString(static_cast<CFStringRef>(uuid),text,sizeof(text),kCFStringEncodingUTF8);
  CFRelease(uuid);
  if(!ok) return {};
  std::string identity="GrainsDosage-mac-machine-v1:";
  for(char* p=text;*p;++p) identity+=char((*p>='A' && *p<='Z')?*p+('a'-'A'):*p);
  unsigned char hash[CC_SHA256_DIGEST_LENGTH]{};
  CC_SHA256(identity.data(),CC_LONG(identity.size()),hash);
  const char* hex="0123456789abcdef";
  std::string result;
  for(auto b:hash){result+=hex[b>>4];result+=hex[b&15];}
  return result;
}
inline std::filesystem::path licenseDirectory() {
  const char* home=getenv("HOME");
  if(!home || !*home) return {};
  return std::filesystem::path(home)/"Library/Application Support/GrainsDosage-Trial";
}
inline bool signatureValid(const License& l) {
  // Public key is a Windows BCRYPT_RSAPUBLIC_BLOB, convert to DER SubjectPublicKeyInfo
  // via ASN.1 INTEGER SEQUENCE (SecKey accepts PKCS#1 RSA public keys).
  if(sizeof(trialPublicKey)!=283 || trialPublicKey[0]!=0x52 || trialPublicKey[1]!=0x53 ||
     trialPublicKey[2]!=0x41 || trialPublicKey[3]!=0x31) return false;
  // Current keygen always issues exponent 65537 and a 256-byte modulus.
  const unsigned char* mod=trialPublicKey+27;
  const unsigned char derPrefix[]={0x30,0x82,0x01,0x0a,0x02,0x82,0x01,0x01,0x00};
  const unsigned char derSuffix[]={0x02,0x03,0x01,0x00,0x01};
  std::vector<unsigned char> der;
  der.insert(der.end(),std::begin(derPrefix),std::end(derPrefix));
  der.insert(der.end(),mod,mod+256);
  der.insert(der.end(),std::begin(derSuffix),std::end(derSuffix));
  CFDataRef keyData=CFDataCreate(kCFAllocatorDefault,der.data(),CFIndex(der.size()));
  if(!keyData) return false;
  const void* keys[]={kSecAttrKeyType,kSecAttrKeyClass,kSecAttrKeySizeInBits};
  int bits=2048; CFNumberRef bitNum=CFNumberCreate(nullptr,kCFNumberIntType,&bits);
  const void* values[]={kSecAttrKeyTypeRSA,kSecAttrKeyClassPublic,bitNum};
  CFDictionaryRef attrs=CFDictionaryCreate(nullptr,keys,values,3,&kCFTypeDictionaryKeyCallBacks,&kCFTypeDictionaryValueCallBacks);
  CFErrorRef error=nullptr;
  SecKeyRef key=SecKeyCreateWithData(keyData,attrs,&error);
  if(error) CFRelease(error);
  CFRelease(attrs);CFRelease(bitNum);CFRelease(keyData);
  if(!key)return false;
  CFDataRef payload=CFDataCreate(nullptr,reinterpret_cast<const UInt8*>(l.payload.data()),CFIndex(l.payload.size()));
  CFDataRef signature=CFDataCreate(nullptr,l.signature.data(),CFIndex(l.signature.size()));
  bool valid=payload&&signature&&SecKeyVerifySignature(key,kSecKeyAlgorithmRSASignatureMessagePKCS1v15SHA256,payload,signature,nullptr);
  if(payload)CFRelease(payload);
  if(signature)CFRelease(signature);
  CFRelease(key);
  return valid;
}
inline bool loadLicense(License& license) {
  auto dir=licenseDirectory();
  if(dir.empty())return false;
  std::ifstream in(dir/"trial.license",std::ios::binary);
  if(!in)return false;
  std::string text(2049, char(0));
  in.read(text.data(),std::streamsize(text.size()));
  text.resize(size_t(in.gcount()));
  if(!parseLicense(text,license)||!signatureValid(license)||license.machine!=machineId())return false;
  const Tick now=wallTime();
  return now>=license.start&&now<license.end;
}
// A locked monotonic high-water file detects ordinary rollback between launches.
// This is less tamper-resistant than Windows DPAPI; a determined local admin can
// remove it or modify the plugin. A signed expiry always remains authoritative.
class Store {
  std::filesystem::path dir_;
public:
  static Store production(){return Store{licenseDirectory()};}
  bool refresh(Tick now,State& state,bool allowCreate,Tick minimumTime=0) const {
    if(dir_.empty()||!wellFormed(state))return false;
    std::error_code ec;
    std::filesystem::create_directories(dir_,ec);
    if(ec)return false;
    auto file=dir_/"licensed-state.dat";
    int fd=open(file.c_str(),O_CREAT|O_RDWR,0600);
    if(fd<0)return false;
    if(flock(fd,LOCK_EX)!=0){close(fd);return false;}
    Tick saved[3]{};
    const auto length=lseek(fd,0,SEEK_END);
    if(length==sizeof(saved)){
      lseek(fd,0,SEEK_SET);
      if(read(fd,saved,sizeof(saved))!=sizeof(saved) || saved[0]!=0x4752545249414c32ULL ||
         saved[1]!=state.start){flock(fd,LOCK_UN);close(fd);return false;}
      state.last=saved[2];
    }else if(length!=0 || !allowCreate){flock(fd,LOCK_UN);close(fd);return false;}
    if(now<state.start){flock(fd,LOCK_UN);close(fd);return false;}
    const bool active=advance(state,now) && (minimumTime<=state.last||advance(state,minimumTime));
    const Tick next[]={0x4752545249414c32ULL,state.start,state.last};
    lseek(fd,0,SEEK_SET);
    bool written=write(fd,next,sizeof(next))==sizeof(next) && ftruncate(fd,sizeof(next))==0 && fsync(fd)==0;
    flock(fd,LOCK_UN);close(fd);
    return written&&active;
  }
};
} // namespace grains::trial
