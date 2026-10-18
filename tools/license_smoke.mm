#include "license.h"
#include <cassert>
#include <chrono>
#include <fstream>
#include <iostream>
namespace aztec::license {void testRecord(int64_t,int64_t,bool=false);void testCleanup();void testTime(int64_t);void testRestart(int);}
int main(int argc,char** argv){
 using namespace aztec::license;assert(argc==10);assert(deviceCode()=="GDDEV2-"+std::string(64,'a'));
 std::ifstream file(argv[1]);std::string serial;std::getline(file,serial);assert(!serial.empty());
 const int64_t t=std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
 assert(evaluate(t,t,t+trialSeconds-1)==State::Trial);
 assert(evaluate(t,t,t+trialSeconds)==State::Expired);
 assert(evaluate(t,t+900,t)==State::ClockError);
 assert(evaluate(0,0,t)==State::StorageError);
 testCleanup();initialize();assert(!allowed.load()); // DAW scan cannot start a trial.
 firstOpen();assert(!allowed.load());assert(status()=="ACTIVATE LICENSE");
 testRecord(t,t);firstOpen();assert(!allowed.load()); // unsigned legacy state cannot authorize a device.

 std::string error,bad=serial;auto i=bad.size()-20;bad[i]=bad[i]=='A'?'B':'A';
 assert(!activate(bad,error));assert(!allowed.load());
 assert(!activate("GDS1.invalid.invalid",error));
 for(int i=2;i<5;++i){std::ifstream rejected(argv[i]);std::string code;std::getline(rejected,code);assert(!activate(code,error));assert(!allowed.load());}
 // Trial signatures come from the actual native GUI backend, with test-only RSA.
 for(int i=6;i<10;++i){std::ifstream rejected(argv[i]);std::string code;std::getline(rejected,code);assert(!activate(code,error));assert(!allowed.load());}
 std::ifstream trialFile(argv[5]);std::string trialCode;std::getline(trialFile,trialCode);
 assert(activate(trialCode,error));assert(allowed.load());assert(status()=="14 DAYS LICENSE ACTIVE - 14 DAYS LEFT");
 // Reconstruct the manager and worker with no cached license/authorization.
 testRestart(false);assert(allowed.load());assert(status()=="14 DAYS LICENSE ACTIVE - 14 DAYS LEFT");
 testRestart(2);assert(!allowed.load());assert(status()=="LICENSE STORAGE ERROR (-25308)");
 assert(!activate(trialCode,error));assert(!allowed.load());assert(error.find("Keychain code -25308")!=std::string::npos);
 testRestart(1);
 assert(!activate(bad,error));assert(!allowed.load()); // prompting cannot validate an altered signature
 assert(activate(trialCode,error));assert(allowed.load());
 testRestart(false);assert(allowed.load()); // persisted state is usable after authorization
 testRecord(t-10,t+1000,true);testRestart(true);assert(!allowed.load());
 assert(!activate(trialCode,error));assert(error.find("system clock")!=std::string::npos);assert(!allowed.load());
 testRestart(false); // authorization must not clear the saved rollback evidence
 assert(status()=="CHECK SYSTEM CLOCK");
 testRecord(t-10,t+1000,true);firstOpen();assert(!allowed.load());assert(status()=="CHECK SYSTEM CLOCK");
 testRecord(t-10,t,true);firstOpen();assert(allowed.load());
 testTime(t+7200);firstOpen();assert(allowed.load());assert(status().find("14 DAYS LICENSE ACTIVE")!=std::string::npos);
 testTime(t+trialSeconds);firstOpen();assert(!allowed.load());assert(status()=="TRIAL EXPIRED");
 testTime(0);
 assert(activate(serial,error));assert(allowed.load());assert(status()=="LICENSE ACTIVE");
 firstOpen();assert(allowed.load()); // license overrides expired/clock-invalid trial.
 assert(!activate(trialCode,error));assert(status()=="LICENSE ACTIVE"); // do not downgrade a paid activation
 testCleanup();std::cout<<"PASS: trial boundary, manager restart, explicit Keychain authorization, preserved rollback evidence, tampering, wrong-device/legacy rejection and offline device activation\n";
}
