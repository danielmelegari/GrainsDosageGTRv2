#include "src/trial_helper_process.h"
#include <cassert>
#include <iostream>
#include <cstdlib>
int main(int argc,char** argv){
 using namespace aztec::license;
 // Child modes exercise the actual spawn/timeout/parser path without Keychain.
 if(argc==6){
  switch(std::atoi(argv[2])){
   case 1:std::cout<<int(State::Trial)<<" 100 200\n";return 0;
   case 2:std::cout<<int(State::Expired)<<" 100 "<<100+trialSeconds<<"\n";return 0;
   case 3:std::this_thread::sleep_for(std::chrono::seconds(30));return 0;
   case 4:std::cout<<"invalid reply\n";return 0;
   case 5:return 7;
   default:return 1;
  }
 }
 char resolved[4096]{};assert(realpath(argv[0],resolved));int64_t first=0,last=0;
 assert(automatic::runHelper(resolved,true,1,0,first,last)==State::Trial);assert(first==100&&last==200);
 assert(automatic::runHelper(resolved,false,2,0,first,last)==State::Expired);
 const auto start=std::chrono::steady_clock::now();
 assert(automatic::runHelper(resolved,false,3,0,first,last,std::chrono::milliseconds(100))==State::StorageError);
 assert(std::chrono::steady_clock::now()-start<std::chrono::seconds(2));
 assert(automatic::runHelper(resolved,false,4,0,first,last)==State::StorageError);
 assert(automatic::runHelper(resolved,false,5,0,first,last)==State::StorageError);
 assert(automatic::runHelper("/nonexistent/grains-helper",false,1,0,first,last)==State::StorageError);
 first=101;assert(automatic::runHelper(resolved,false,1,0,first,last)==State::StorageError);
 int savedIn=dup(STDIN_FILENO),savedOut=dup(STDOUT_FILENO);assert(savedOut>=0);
 close(STDIN_FILENO);close(STDOUT_FILENO);first=last=0;
 auto withoutStdio=automatic::runHelper(resolved,true,1,0,first,last);
 if(savedIn>=0){dup2(savedIn,STDIN_FILENO);close(savedIn);}
 dup2(savedOut,STDOUT_FILENO);close(savedOut);assert(withoutStdio==State::Trial);
 int status=0;assert(waitpid(-1,&status,WNOHANG)==-1&&errno==ECHILD);
 std::cout<<"PASS: helper results, immutable start, timeout, malformed output, missing executable and child reaping\n";
}
