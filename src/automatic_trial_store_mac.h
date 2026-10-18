#pragma once
#import <Foundation/Foundation.h>
#import <Security/Security.h>
#include "automatic_trial.h"
#include <sys/file.h>
#include <fcntl.h>
#include <unistd.h>
#include <sstream>
namespace aztec::license {
namespace {
#ifdef GRAINS_AUTO_TRIAL_TEST
NSString* trialTestId(){const char* id=getenv("GRAINS_TRIAL_TEST_ID");return id?[NSString stringWithUTF8String:id]:[NSString stringWithFormat:@"%d",getpid()];}
#endif
struct Storage {
 int64_t now()const{return automatic::wallSeconds();}
 int64_t steady()const{return automatic::steadySeconds();}
 State update(bool create,int64_t current,int64_t minimum,int64_t& first,int64_t& last){@autoreleasepool{
  NSString* root=[NSHomeDirectory() stringByAppendingPathComponent:@"Library/Application Support/GrainsDosage/AutomaticTrial"];
#ifdef GRAINS_AUTO_TRIAL_TEST
  root=[NSTemporaryDirectory() stringByAppendingPathComponent:[NSString stringWithFormat:@"grains-auto-trial-test-%@",trialTestId()]];
#endif
  NSError* error=nil;if(![[NSFileManager defaultManager] createDirectoryAtPath:root withIntermediateDirectories:YES attributes:nil error:&error])return State::StorageError;
  const int fd=::open([[root stringByAppendingPathComponent:@"state.lock"] fileSystemRepresentation],O_CREAT|O_RDWR,0600);if(fd<0)return State::StorageError;
  struct Lock{int fd;~Lock(){flock(fd,LOCK_UN);close(fd);}} lock{fd};if(flock(fd,LOCK_EX|LOCK_NB)!=0)return State::StorageError;
  NSString* marker=[root stringByAppendingPathComponent:@"started"];
  bool marked=[[NSFileManager defaultManager] fileExistsAtPath:marker];
  NSMutableDictionary* q=[@{(__bridge id)kSecClass:(__bridge id)kSecClassGenericPassword,(__bridge id)kSecAttrService:@"com.danielmelegari.grainsdosage.automatic-trial.v1",(__bridge id)kSecAttrAccount:@"first-use",(__bridge id)kSecUseAuthenticationUI:(__bridge id)kSecUseAuthenticationUIFail} mutableCopy];
#ifdef GRAINS_AUTO_TRIAL_TEST
  q[(__bridge id)kSecAttrService]=[NSString stringWithFormat:@"com.grainsdosage.auto-test.%@",trialTestId()];
#endif
  auto read=[q mutableCopy];read[(__bridge id)kSecReturnData]=@YES;CFTypeRef found=nullptr;OSStatus result=SecItemCopyMatching((__bridge CFDictionaryRef)read,&found);
  bool fresh=result==errSecItemNotFound;
  if(fresh){if(marked||first)return State::StorageError;if(!create)return State::Ready;first=last=current;}
  else if(result!=errSecSuccess){if(found)CFRelease(found);return State::StorageError;}
  else{NSData* data=CFBridgingRelease(found);NSString* text=[[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];if(!text)return State::StorageError;std::istringstream input(text.UTF8String);int64_t a=0,b=0;std::string extra;if(!(input>>a>>b)||(input>>extra)||a<=0||b<a||(first&&a!=first))return State::StorageError;first=a;last=b;}
  bool rollback=current<first||current+120<last;
  last=std::max({last,current,minimum});
  NSData* record=[[NSString stringWithFormat:@"%lld %lld",(long long)first,(long long)last] dataUsingEncoding:NSUTF8StringEncoding];
  if(fresh){q[(__bridge id)kSecValueData]=record;result=SecItemAdd((__bridge CFDictionaryRef)q,nullptr);}
  else result=SecItemUpdate((__bridge CFDictionaryRef)q,(__bridge CFDictionaryRef)@{(__bridge id)kSecValueData:record});
  if(result!=errSecSuccess)return State::StorageError;
  if(!marked&&![@"Started; authoritative dates are stored in Keychain." writeToFile:marker atomically:YES encoding:NSUTF8StringEncoding error:&error])return State::StorageError;
  if(last-first>=trialSeconds)return State::Expired;
  return rollback?State::ClockError:State::Trial;
 }}
};
}
}
