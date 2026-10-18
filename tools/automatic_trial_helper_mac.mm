#include "../src/automatic_trial_store_mac.h"
#include <iostream>
int main(int argc,char** argv){@autoreleasepool{
 // This is a separate process. Never change the DAW's global Keychain policy.
 // Legacy macOS Keychain can ignore kSecUseAuthenticationUIFail on password reads.
 if(SecKeychainSetUserInteractionAllowed(false)!=errSecSuccess||argc!=6)return 1;
 try{
  auto number=[](const char* value){size_t used=0;auto n=std::stoll(value,&used);if(value[used]||n<0)throw 1;return n;};
  if(std::string(argv[1])!="0"&&std::string(argv[1])!="1")return 1;
  int64_t first=number(argv[4]),last=number(argv[5]);
  aztec::license::Storage storage;
  auto state=storage.update(argv[1][0]=='1',number(argv[2]),number(argv[3]),first,last);
  std::cout<<int(state)<<' '<<first<<' '<<last<<'\n';return 0;
 }catch(...){return 1;}
}}
