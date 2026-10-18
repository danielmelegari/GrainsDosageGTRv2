#include "ledger.h"
#include <cassert>
#include <atomic>
#include <iostream>
#include <thread>
#include <unistd.h>
#include <cstdio>
using namespace grains::issuer;
template<class F> bool fails(F f){try{f();return false;}catch(const std::exception&){return true;}}
int main(){
 char dir[]="/tmp/grains-issuer-test-XXXXXX";assert(mkdtemp(dir));const std::string path=std::string(dir)+"/licenses.sqlite",fp(64,'a');
 const auto id="GDL2-"+std::string(32,'1'),id2="GDL2-"+std::string(32,'2');
 const auto device=[](int n){std::string s(64,'a');s.back()=char('0'+n);return "GDDEV2-"+s;};
 auto sign=[](const Grant& g){return "SIGNED:"+g.payload;};Grant grant;
 assert(fails([&]{Ledger missing(path,fp);}));
 {Ledger db(path,fp,true);
  auto a=db.issue(false,device(0),"  Alice  ","",id,1700000000,sign,grant);assert(grant.license==id&&grant.devices==1);
  auto b=db.issue(false,device(0),"alice","",id2,1700001000,sign,grant);assert(a==b&&grant.license==id&&grant.devices==1);
  assert(fails([&]{db.issue(false,device(1),"Bob",id,id2,1700000000,sign,grant);}));
  db.issue(false,device(1),"ALICE","",id2,1700000000,sign,grant);assert(grant.devices==2);
  db.issue(false,std::string(64,'b'),"Alice","",id2,1700000000,sign,grant);assert(grant.devices==3);assert(grant.payload=="GRAINS-PERMANENT-1\n"+std::string(64,'b')+"\n"+std::string(32,'1')+"\n3\n");
  assert(fails([&]{db.issue(false,device(2),"Alice","",id2,1700000000,sign,grant);}));
  assert(fails([&]{db.issue(true,"GDDEV2-short","","",id,1700000000,sign,grant);}));
  auto trial=db.issue(true,device(7),"","","",1700000000,sign,grant);assert(grant.end-grant.start==1209600);
  assert(trial==db.issue(true,device(7),"","","",1900000000,sign,grant)); // never restart even after expiry
  assert(grant.start==1700000000);
  db.issue(true,std::string(64,'c'),"","","",1700000000,sign,grant);assert(grant.payload.find("GRAINS-TRIAL-1\n")==0);assert(grant.payload.find("133444736000000000\n")!=std::string::npos);
  assert(fails([&]{db.issue(false,device(8),"Signing failure","",id2,1700000000,[](const Grant&)->std::string{throw std::runtime_error("Keychain locked");},grant);}));
  db.issue(false,device(8),"Bob","",id2,1700000000,sign,grant);assert(grant.devices==1); // failed signing transaction rolled back
 }
 assert(fails([&]{Ledger wrong(path,std::string(64,'f'));}));
 {Ledger reopen(path,fp);reopen.issue(false,device(0),"Alice","",id2,1700000000,sign,grant);assert(grant.devices==3);}
 const auto id3="GDL2-"+std::string(32,'3');std::atomic<int> successes{0};std::vector<std::thread> workers;
 for(int i=0;i<8;++i)workers.emplace_back([&,i]{try{Ledger db(path,fp);Grant g;db.issue(false,device(i),"Concurrent","",id3,1700000000,sign,g);++successes;}catch(const std::exception&) {}});
 for(auto& t:workers)t.join();assert(successes==3);
 // Existing Python issuer schemas/rows are accepted, without resetting slots.
 sqlite3* raw=nullptr;assert(sqlite3_open(path.c_str(),&raw)==SQLITE_OK);
 assert(sqlite3_exec(raw,"INSERT INTO licenses VALUES('GDL2-44444444444444444444444444444444','Alice')",nullptr,nullptr,nullptr)==SQLITE_OK);sqlite3_close(raw);
 {Ledger db(path,fp);assert(fails([&]{db.issue(false,device(0),"Alice","",id2,1700000000,sign,grant);}));db.issue(false,device(0),"Alice",id,id2,1700000000,sign,grant);assert(grant.devices==3);}
 std::remove(path.c_str());rmdir(dir);std::cout<<"PASS: persisted customer/license reuse, mixed-platform 3-device cap, concurrent issuance, fixed trial dates, signing rollback, key mismatch and missing-ledger rejection\n";
}
