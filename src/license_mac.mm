#include "license.h"
#ifdef GRAINS_LICENSE_TEST
#include "license_test_key.h"
#else
#include "license_public_key.h"
#endif
#import <Foundation/Foundation.h>
#import <Security/Security.h>
#import <IOKit/IOKitLib.h>
#import <CommonCrypto/CommonDigest.h>
#include <algorithm>
#include <chrono>
#include <cerrno>
#include <cstdlib>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <unistd.h>

namespace aztec::license {
std::string deviceCode(){
#ifdef GRAINS_LICENSE_TEST
 return "GDDEV2-"+std::string(64,'a');
#else
 static const std::string code=[]()->std::string{
  io_service_t platform=IOServiceGetMatchingService(kIOMasterPortDefault,IOServiceMatching("IOPlatformExpertDevice"));
  if(!platform)return {};
  CFTypeRef value=IORegistryEntryCreateCFProperty(platform,CFSTR("IOPlatformUUID"),kCFAllocatorDefault,0);IOObjectRelease(platform);
  if(!value)return {};if(CFGetTypeID(value)!=CFStringGetTypeID()){CFRelease(value);return {};}
  NSString* identifier=CFBridgingRelease(value);if(identifier.length<16)return {};
  NSData* bytes=[[@"GrainsDosage-device-v2:" stringByAppendingString:identifier.lowercaseString] dataUsingEncoding:NSUTF8StringEncoding];
  unsigned char digest[CC_SHA256_DIGEST_LENGTH];CC_SHA256(bytes.bytes,CC_LONG(bytes.length),digest);
  std::string result="GDDEV2-";const char* hex="0123456789abcdef";for(unsigned char b:digest){result+=hex[b>>4];result+=hex[b&15];}return result;
 }();return code;
#endif
}
namespace {
#ifdef GRAINS_LICENSE_TEST
std::atomic<int64_t> testClock{0};
#endif
int64_t now(){
#ifdef GRAINS_LICENSE_TEST
 if(auto t=testClock.load())return t;
#endif
 return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();}
NSString* service(){
#ifdef GRAINS_LICENSE_TEST
 return [NSString stringWithFormat:@"com.danielmelegari.grainsdosage.test.%d",getpid()];
#else
 return @"com.danielmelegari.grainsdosage.trial.v1";
#endif
}
NSMutableDictionary* query(bool signedTrial=false){return [@{(__bridge id)kSecClass:(__bridge id)kSecClassGenericPassword,
 (__bridge id)kSecAttrService:service(),
 (__bridge id)kSecAttrAccount:signedTrial?@"signed-trial":@"first-open",(__bridge id)kSecUseAuthenticationUI:(__bridge id)kSecUseAuthenticationUIFail} mutableCopy];}
NSString* serialPath(){NSString* root=[NSHomeDirectory() stringByAppendingPathComponent:@"Library/Application Support/GrainsDosage/Licensing"];

#ifdef GRAINS_LICENSE_TEST
 root=[NSTemporaryDirectory() stringByAppendingPathComponent:[NSString stringWithFormat:@"grains-license-test-%d",getpid()]];
#endif
 [[NSFileManager defaultManager] createDirectoryAtPath:root withIntermediateDirectories:YES attributes:nil error:nil];
 return [root stringByAppendingPathComponent:@"license.txt"];}
bool verify(NSString* serial,int64_t* trialStart=nullptr,int64_t* trialEnd=nullptr){
 if(trialStart)*trialStart=0;if(trialEnd)*trialEnd=0;
 if(serial.length>4096)return false;
 NSArray* fields=[serial componentsSeparatedByString:@"."];if(fields.count!=3||![fields[0] isEqual:@"GDS2"])return false;
 NSData* payload=[[NSData alloc] initWithBase64EncodedString:fields[1] options:0];
 NSData* signature=[[NSData alloc] initWithBase64EncodedString:fields[2] options:0];
 if(!payload||!signature||signature.length!=256)return false;
 NSString* message=[[NSString alloc] initWithData:payload encoding:NSUTF8StringEncoding];
 // Existing permanent payloads remain byte-for-byte compatible. Signed
 // trials use the same RSA key and bind a fixed 14-day UTC interval to this Mac.
 NSArray* parts=[message componentsSeparatedByString:@":"];
 if(parts.count!=4)return false;
 NSCharacterSet* invalid=[[NSCharacterSet characterSetWithCharactersInString:@"0123456789abcdef"] invertedSet];
 NSString* device=nil;int64_t start=0,end=0;
 if([parts[0] isEqual:@"GRAINS-DOSAGE-V2"]){
  NSString* identifier=parts[1];device=parts[2];
  if(![parts[3] isEqual:@"3"]||identifier.length!=32||[identifier rangeOfCharacterFromSet:invalid].location!=NSNotFound)return false;
 }else if([parts[0] isEqual:@"GRAINS-DOSAGE-TRIAL-V2"]){
  device=parts[1];
  auto number=[](NSString* text,int64_t& result){
   if(!text.length||text.length>12||[text rangeOfCharacterFromSet:[[NSCharacterSet characterSetWithCharactersInString:@"0123456789"] invertedSet]].location!=NSNotFound)return false;
   errno=0;char* tail=nullptr;result=strtoll(text.UTF8String,&tail,10);return errno==0&&tail&&!*tail;
  };
  if(!number(parts[2],start)||!number(parts[3],end)||start<=0||start>4102444800LL||end-start!=trialSeconds)return false;
 }else return false;
 if(device.length!=64||[device rangeOfCharacterFromSet:invalid].location!=NSNotFound)return false;
 const auto local=deviceCode();if(local.empty()||local!="GDDEV2-"+std::string(device.UTF8String))return false;
 NSData* keyData=[NSData dataWithBytes:publicKey length:sizeof(publicKey)];
 NSDictionary* attrs=@{(__bridge id)kSecAttrKeyType:(__bridge id)kSecAttrKeyTypeRSA,(__bridge id)kSecAttrKeyClass:(__bridge id)kSecAttrKeyClassPublic,(__bridge id)kSecAttrKeySizeInBits:@2048};
 SecKeyRef key=SecKeyCreateWithData((__bridge CFDataRef)keyData,(__bridge CFDictionaryRef)attrs,nullptr);if(!key)return false;
 bool ok=SecKeyVerifySignature(key,kSecKeyAlgorithmRSASignatureMessagePKCS1v15SHA256,(__bridge CFDataRef)payload,(__bridge CFDataRef)signature,nullptr);CFRelease(key);if(ok){if(trialStart)*trialStart=start;if(trialEnd)*trialEnd=end;}return ok;
}
class Manager {
 std::mutex mutex;std::condition_variable wake;bool stopping=false;std::thread worker;
 State state=State::Ready;int64_t first=0,last=0;bool licensed=false;
 int64_t signedStart=0,cachedStart=0,cachedEnd=0;bool cachedValid=false,protectedOK=false;
 std::chrono::steady_clock::time_point signedDeadline{},lastStorage{};
 std::atomic<State> viewState{State::Ready};std::atomic<int64_t> starts{0};
 void publish(State s){state=s;starts.store(first);viewState.store(s);allowed.store(s==State::Licensed||s==State::Trial,std::memory_order_release);}
 bool loadTrial(bool signedTrial=false){auto q=query(signedTrial);q[(__bridge id)kSecReturnData]=@YES;CFTypeRef out=nullptr;
  OSStatus result=SecItemCopyMatching((__bridge CFDictionaryRef)q,&out);
  if(result==errSecItemNotFound){first=last=0;return true;}
  if(result!=errSecSuccess){publish(State::StorageError);return false;}
  NSData* data=CFBridgingRelease(out);NSString* text=[[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
  NSArray* fields=[text componentsSeparatedByString:@":"];
  if(fields.count!=2){publish(State::StorageError);return false;}
  first=[fields[0] longLongValue];last=[fields[1] longLongValue];
  if(first<=0||last<first){publish(State::StorageError);return false;}return true;
 }
 NSData* record(int64_t start,int64_t seen){return [[NSString stringWithFormat:@"%lld:%lld",(long long)start,(long long)seen] dataUsingEncoding:NSUTF8StringEncoding];}
 void loadSerial(){
  NSString* serial=[NSString stringWithContentsOfFile:serialPath() encoding:NSUTF8StringEncoding error:nil];
  cachedValid=serial&&verify([serial stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceAndNewlineCharacterSet]],&cachedStart,&cachedEnd);
  protectedOK=false;lastStorage={};
 }
 void refresh(){
  const int64_t start=cachedStart,end=cachedEnd;const bool valid=cachedValid;
  licensed=valid&&!start;
  if(licensed){publish(State::Licensed);return;}
  if(valid&&start){
   const auto current=now();
   if(current<start){publish(State::ClockError);return;}
   if(current>=end){publish(State::Expired);return;}
   if(signedStart!=start){signedStart=start;signedDeadline=std::chrono::steady_clock::now()+std::chrono::seconds(end-current);}
   if(std::chrono::steady_clock::now()>=signedDeadline){publish(State::Expired);return;}
   if(std::chrono::steady_clock::now()-lastStorage<std::chrono::seconds(30)){if(protectedOK)publish(State::Trial);return;}
   lastStorage=std::chrono::steady_clock::now();protectedOK=false;
   if(!loadTrial(true))return;
   first=start;last=std::max(last,start);
   State next=evaluate(first,last,current);
   if(next==State::Trial){auto q=query(true);NSData* data=record(first,std::max(last,current));
    OSStatus result=SecItemUpdate((__bridge CFDictionaryRef)q,(__bridge CFDictionaryRef)@{(__bridge id)kSecValueData:data});
    if(result==errSecItemNotFound){q[(__bridge id)kSecValueData]=data;result=SecItemAdd((__bridge CFDictionaryRef)q,nullptr);if(result==errSecDuplicateItem)result=SecItemUpdate((__bridge CFDictionaryRef)query(true),(__bridge CFDictionaryRef)@{(__bridge id)kSecValueData:data});}
    if(result!=errSecSuccess){publish(State::StorageError);return;}last=std::max(last,current);
   }
   protectedOK=next==State::Trial;publish(next);return;
  }
  publish(State::Required);
 }
public:
 Manager(){@autoreleasepool{loadSerial();refresh();}worker=std::thread([this]{std::unique_lock<std::mutex> lock(mutex);while(!wake.wait_for(lock,std::chrono::seconds(1),[this]{return stopping;})){@autoreleasepool{refresh();}}});}
 ~Manager(){{std::lock_guard<std::mutex> lock(mutex);stopping=true;}wake.notify_all();worker.join();}
 void open(){std::lock_guard<std::mutex> lock(mutex);@autoreleasepool{loadSerial();refresh();

 }}
 std::string label(){switch(viewState.load()){
  case State::Licensed:return "LICENSED";
  case State::Ready:return "START TRIAL";
  case State::Trial:return "TRIAL ACTIVE - "+std::to_string(std::max<int64_t>(1,(starts.load()+trialSeconds-now()+86399)/86400))+" DAYS";
  case State::Expired:return "TRIAL EXPIRED";
  case State::ClockError:return "CHECK SYSTEM CLOCK";
  case State::StorageError:return "LICENSE STORAGE ERROR";
  default:return "ACTIVATE LICENSE";}}
 bool activateSerial(const std::string& input,std::string& error){std::lock_guard<std::mutex> lock(mutex);@autoreleasepool{
  NSString* serial=[[NSString stringWithUTF8String:input.c_str()] stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceAndNewlineCharacterSet]];
  if([serial hasPrefix:@"GDS1."]){error="This portable license must be exchanged for a device activation. Send your Device Code to the publisher.";return false;}
  if(deviceCode().empty()){error="Cannot read this Mac device code. Activation has not been saved.";return false;}
  int64_t start=0,end=0;
  if(!verify(serial,&start,&end)){error="Invalid activation code, or the code belongs to another Mac.";return false;}
  if(start&&(now()<start||now()>=end)){error="This signed trial has not started or has expired.";return false;}
  if(start&&licensed){error="This Mac already has a permanent license.";return false;}
  if(![serial writeToFile:serialPath() atomically:YES encoding:NSUTF8StringEncoding error:nil]){error="Cannot save the license in Application Support.";return false;}
  loadSerial();refresh();if(state!=State::Licensed&&state!=State::Trial){error="License saved, but protected trial state is unavailable or the system clock is invalid.";return false;}return true;}}
};
Manager& manager(){static Manager instance;return instance;}
}
#ifdef GRAINS_LICENSE_TEST
void testTime(int64_t t){testClock.store(t);}
void testRecord(int64_t start,int64_t seen,bool signedTrial=false){auto q=query(signedTrial);SecItemDelete((__bridge CFDictionaryRef)q);q[(__bridge id)kSecValueData]=[[NSString stringWithFormat:@"%lld:%lld",(long long)start,(long long)seen] dataUsingEncoding:NSUTF8StringEncoding];SecItemAdd((__bridge CFDictionaryRef)q,nullptr);}
void testCleanup(){SecItemDelete((__bridge CFDictionaryRef)query());SecItemDelete((__bridge CFDictionaryRef)query(true));[[NSFileManager defaultManager] removeItemAtPath:serialPath() error:nil];}
#endif
void initialize(){(void)manager();}
void firstOpen(){manager().open();}
std::string status(){return manager().label();}
bool activate(const std::string& serial,std::string& error){return manager().activateSerial(serial,error);}
}
