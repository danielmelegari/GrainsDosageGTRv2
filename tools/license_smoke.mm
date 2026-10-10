#include "license.h"
#include <cassert>
#include <chrono>
#include <fstream>
#include <iostream>
namespace aztec::license {void testRecord(int64_t,int64_t);void testCleanup();}
int main(int argc,char** argv){
 using namespace aztec::license;assert(argc==2);
 std::ifstream file(argv[1]);std::string serial;std::getline(file,serial);assert(!serial.empty());
 const int64_t t=std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
 assert(evaluate(t,t,t+trialSeconds-1)==State::Trial);
 assert(evaluate(t,t,t+trialSeconds)==State::Expired);
 assert(evaluate(t,t+900,t)==State::ClockError);
 assert(evaluate(0,0,t)==State::StorageError);
 testCleanup();initialize();assert(!allowed.load()); // DAW scan cannot start a trial.
#if GRAINS_TRIAL
 firstOpen();assert(allowed.load());assert(status().find("14 DAYS")!=std::string::npos);
 testRecord(t-trialSeconds,t);firstOpen();assert(!allowed.load());assert(status()=="TRIAL EXPIRED");
 firstOpen();assert(!allowed.load()); // reopening cannot restart an expired trial.
 testRecord(t-100,t+1000);firstOpen();assert(!allowed.load());assert(status()=="CHECK SYSTEM CLOCK");
#else
 firstOpen();assert(!allowed.load());assert(status()=="ACTIVATE LICENSE");
 testRecord(t,t);firstOpen();assert(!allowed.load()); // paid edition never grants a trial.
#endif
 std::string error,bad=serial;auto i=bad.size()-20;bad[i]=bad[i]=='A'?'B':'A';
 assert(!activate(bad,error));assert(!allowed.load());
 assert(!activate("GDS1.invalid.invalid",error));
 assert(activate(serial,error));assert(allowed.load());assert(status()=="LICENSED");
 firstOpen();assert(allowed.load()); // license overrides expired/clock-invalid trial.
 testCleanup();std::cout<<"PASS: trial boundary, persistence, clock rollback, tampering and offline activation\n";
}
