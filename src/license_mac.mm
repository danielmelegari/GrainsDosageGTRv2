#include "license.h"
#ifdef GRAINS_LICENSE_TEST
#include "license_test_key.h"
#else
#include "license_public_key.h"
#endif
#import <Foundation/Foundation.h>
#import <Security/Security.h>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <unistd.h>

namespace aztec::license {
namespace {
int64_t now(){return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();}
NSString* service(){
#ifdef GRAINS_LICENSE_TEST
 return [NSString stringWithFormat:@"com.danielmelegari.grainsdosage.test.%d",getpid()];
#else
 return @"com.danielmelegari.grainsdosage.trial.v1";
#endif
}
NSMutableDictionary* query(){return [@{(__bridge id)kSecClass:(__bridge id)kSecClassGenericPassword,
 (__bridge id)kSecAttrService:service(),
 (__bridge id)kSecAttrAccount:@"first-open",(__bridge id)kSecUseAuthenticationUI:(__bridge id)kSecUseAuthenticationUIFail} mutableCopy];}
NSString* serialPath(){NSString* root=[NSHomeDirectory() stringByAppendingPathComponent:@"Library/Application Support/GrainsDosage/Licensing"];

#ifdef GRAINS_LICENSE_TEST
 root=[NSTemporaryDirectory() stringByAppendingPathComponent:[NSString stringWithFormat:@"grains-license-test-%d",getpid()]];
#endif
 [[NSFileManager defaultManager] createDirectoryAtPath:root withIntermediateDirectories:YES attributes:nil error:nil];
 return [root stringByAppendingPathComponent:@"license.txt"];}
bool verify(NSString* serial){
 if(serial.length>4096)return false;
 NSArray* fields=[serial componentsSeparatedByString:@"."];if(fields.count!=3||![fields[0] isEqual:@"GDS1"])return false;
 NSData* payload=[[NSData alloc] initWithBase64EncodedString:fields[1] options:0];
 NSData* signature=[[NSData alloc] initWithBase64EncodedString:fields[2] options:0];
 if(!payload||!signature||signature.length!=256)return false;
 NSString* message=[[NSString alloc] initWithData:payload encoding:NSUTF8StringEncoding];
 // Fixed, product-specific payload: GRAINS-DOSAGE-V1:<32 lowercase hex license ID>.
 if(message.length!=49||![message hasPrefix:@"GRAINS-DOSAGE-V1:"])return false;
 NSString* identifier=[message substringFromIndex:17];
 if(identifier.length!=32||[identifier rangeOfCharacterFromSet:[[NSCharacterSet characterSetWithCharactersInString:@"0123456789abcdef"] invertedSet]].location!=NSNotFound)return false;
 NSData* keyData=[NSData dataWithBytes:publicKey length:sizeof(publicKey)];
 NSDictionary* attrs=@{(__bridge id)kSecAttrKeyType:(__bridge id)kSecAttrKeyTypeRSA,(__bridge id)kSecAttrKeyClass:(__bridge id)kSecAttrKeyClassPublic,(__bridge id)kSecAttrKeySizeInBits:@2048};
 SecKeyRef key=SecKeyCreateWithData((__bridge CFDataRef)keyData,(__bridge CFDictionaryRef)attrs,nullptr);if(!key)return false;
 bool ok=SecKeyVerifySignature(key,kSecKeyAlgorithmRSASignatureMessagePKCS1v15SHA256,(__bridge CFDataRef)payload,(__bridge CFDataRef)signature,nullptr);CFRelease(key);return ok;
}
class Manager {
 std::mutex mutex;std::condition_variable wake;bool stopping=false;std::thread worker;
 State state=State::Ready;int64_t first=0,last=0;bool licensed=false;
 std::atomic<State> viewState{State::Ready};std::atomic<int64_t> starts{0};
 void publish(State s){state=s;starts.store(first);viewState.store(s);allowed.store(s==State::Licensed||s==State::Trial,std::memory_order_release);}
 bool loadTrial(){auto q=query();q[(__bridge id)kSecReturnData]=@YES;CFTypeRef out=nullptr;
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
 void refresh(){
  NSString* serial=[NSString stringWithContentsOfFile:serialPath() encoding:NSUTF8StringEncoding error:nil];
  licensed=serial&&verify([serial stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceAndNewlineCharacterSet]]);
  if(licensed){publish(State::Licensed);return;}
#if GRAINS_TRIAL
  if(!loadTrial())return;
  if(!first){publish(State::Ready);return;}
  const auto current=now();State next=evaluate(first,last,current);
  if(next==State::Trial&&current>last+60){auto q=query();NSDictionary* changes=@{(__bridge id)kSecValueData:record(first,current)};
   if(SecItemUpdate((__bridge CFDictionaryRef)q,(__bridge CFDictionaryRef)changes)!=errSecSuccess){publish(State::StorageError);return;}last=current;}
  publish(next);
#else
  publish(State::Required);
#endif
 }
public:
 Manager(){@autoreleasepool{refresh();}worker=std::thread([this]{std::unique_lock<std::mutex> lock(mutex);while(!wake.wait_for(lock,std::chrono::seconds(15),[this]{return stopping;})){@autoreleasepool{refresh();}}});}
 ~Manager(){{std::lock_guard<std::mutex> lock(mutex);stopping=true;}wake.notify_all();worker.join();}
 void open(){std::lock_guard<std::mutex> lock(mutex);@autoreleasepool{refresh();
#if GRAINS_TRIAL
  if(state==State::Ready){auto q=query();const auto t=now();q[(__bridge id)kSecValueData]=record(t,t);
   OSStatus result=SecItemAdd((__bridge CFDictionaryRef)q,nullptr);
   if(result!=errSecSuccess&&result!=errSecDuplicateItem){publish(State::StorageError);return;}refresh();}
#endif
 }}
 std::string label(){switch(viewState.load()){
  case State::Licensed:return "LICENSED";
  case State::Ready:return "START TRIAL";
  case State::Trial:return "TRIAL - "+std::to_string(std::max<int64_t>(1,(starts.load()+trialSeconds-now()+86399)/86400))+" DAYS";
  case State::Expired:return "TRIAL EXPIRED";
  case State::ClockError:return "CHECK SYSTEM CLOCK";
  case State::StorageError:return "LICENSE STORAGE ERROR";
  default:return "ACTIVATE LICENSE";}}
 bool activateSerial(const std::string& input,std::string& error){std::lock_guard<std::mutex> lock(mutex);@autoreleasepool{
  NSString* serial=[[NSString stringWithUTF8String:input.c_str()] stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceAndNewlineCharacterSet]];
  if(!verify(serial)){error="Invalid serial number for GrainsDosage.";return false;}
  if(![serial writeToFile:serialPath() atomically:YES encoding:NSUTF8StringEncoding error:nil]){error="Cannot save the license in Application Support.";return false;}
  refresh();return licensed;}}
};
Manager& manager(){static Manager instance;return instance;}
}
#ifdef GRAINS_LICENSE_TEST
void testRecord(int64_t start,int64_t seen){auto q=query();SecItemDelete((__bridge CFDictionaryRef)q);q[(__bridge id)kSecValueData]=[[NSString stringWithFormat:@"%lld:%lld",(long long)start,(long long)seen] dataUsingEncoding:NSUTF8StringEncoding];SecItemAdd((__bridge CFDictionaryRef)q,nullptr);}
void testCleanup(){SecItemDelete((__bridge CFDictionaryRef)query());[[NSFileManager defaultManager] removeItemAtPath:serialPath() error:nil];}
#endif
void initialize(){(void)manager();}
void firstOpen(){manager().open();}
std::string status(){return manager().label();}
bool activate(const std::string& serial,std::string& error){return manager().activateSerial(serial,error);}
}
