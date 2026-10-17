#include "../src/automatic_trial_mac.mm"
#include <cassert>
#include <iostream>
int main(){@autoreleasepool{
 using namespace aztec::license;
 NSString* service=[NSString stringWithFormat:@"com.grainsdosage.auto-test.%d",getpid()];
 NSDictionary* q=@{(__bridge id)kSecClass:(__bridge id)kSecClassGenericPassword,(__bridge id)kSecAttrService:service,(__bridge id)kSecAttrAccount:@"first-use"};
 NSString* root=[NSTemporaryDirectory() stringByAppendingPathComponent:[NSString stringWithFormat:@"grains-auto-trial-test-%d",getpid()]];
 auto cleanup=[&](){SecItemDelete((__bridge CFDictionaryRef)q);[[NSFileManager defaultManager] removeItemAtPath:root error:nil];};cleanup();
 Storage store;int64_t first=0,last=0;const int64_t now=automatic::wallSeconds();
 assert(store.update(false,now,0,first,last)==State::Ready);
 assert(store.update(true,now,0,first,last)==State::Trial);assert(first==now);
 first=last=0;assert(store.update(true,now+86400,0,first,last)==State::Trial);assert(first==now);
 assert(store.update(false,now,0,first,last)==State::ClockError);
 assert(store.update(false,now+trialSeconds-1,0,first,last)==State::Trial);
 assert(store.update(false,now+trialSeconds,0,first,last)==State::Expired);
 first=last=0;assert(store.update(true,now,0,first,last)==State::Expired);
 assert(SecItemDelete((__bridge CFDictionaryRef)q)==errSecSuccess);first=last=0;
 assert(store.update(true,now,0,first,last)==State::StorageError); // Marker prevents a deleted Keychain item resetting the trial.
 cleanup();std::cout<<"PASS: real Keychain automatic trial, restart, rollback, exact expiry and partial deletion protection\n";
}}
