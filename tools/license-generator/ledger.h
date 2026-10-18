#pragma once
// Publisher ledger; no signing keys or signatures are stored here.
#include <sqlite3.h>
#include <algorithm>
#include <cstdint>
#include <functional>
#include <stdexcept>
#include <string>
#include <vector>

namespace grains::issuer {
inline bool hex(const std::string& s, size_t length) {
  return s.size()==length && s.find_first_not_of("0123456789abcdef")==std::string::npos;
}
inline bool macDevice(const std::string& s) { return s.compare(0,7,"GDDEV2-")==0 && hex(s.substr(7),64); }
inline bool deviceValid(const std::string& s) { return macDevice(s)||hex(s,64); }
inline bool licenseValid(const std::string& s) { return s.compare(0,5,"GDL2-")==0 && hex(s.substr(5),32); }
inline std::string customerKey(std::string s) {
  auto ws=[](char c){return c==' '||c=='\t'||c=='\r'||c=='\n';};
  while(!s.empty()&&ws(s.back()))s.pop_back();
  auto i=std::find_if_not(s.begin(),s.end(),ws);s.erase(s.begin(),i);
  for(auto& c:s)if(c>='A'&&c<='Z')c+=32;
  return s;
}
struct Grant { std::string payload, license; int64_t start=0,end=0; int devices=0; };
class Ledger {
  sqlite3* db=nullptr;
  void run(const char* sql) { if(sqlite3_exec(db,sql,nullptr,nullptr,nullptr)!=SQLITE_OK)throw std::runtime_error(sqlite3_errmsg(db)); }
  struct Statement {
    sqlite3_stmt* p=nullptr;
    Statement(sqlite3* db,const char* sql,const std::vector<std::string>& values={}) {
      if(sqlite3_prepare_v2(db,sql,-1,&p,nullptr)!=SQLITE_OK)throw std::runtime_error(sqlite3_errmsg(db));
      for(size_t i=0;i<values.size();++i)if(sqlite3_bind_text(p,int(i+1),values[i].c_str(),-1,SQLITE_TRANSIENT)!=SQLITE_OK){sqlite3_finalize(p);p=nullptr;throw std::runtime_error("SQLite bind failed");}
    }
    ~Statement(){sqlite3_finalize(p);}
    bool next(){int r=sqlite3_step(p);if(r==SQLITE_ROW)return true;if(r==SQLITE_DONE)return false;throw std::runtime_error(sqlite3_errmsg(sqlite3_db_handle(p)));}
    std::string text(int i){auto s=sqlite3_column_text(p,i);if(!s)throw std::runtime_error("Registro SQLite non valido");return reinterpret_cast<const char*>(s);}
  };
  std::vector<std::vector<std::string>> rows(const char* sql,const std::vector<std::string>& args={}){
    Statement q(db,sql,args);std::vector<std::vector<std::string>> result;
    while(q.next()){std::vector<std::string> row;for(int i=0;i<sqlite3_column_count(q.p);++i)row.push_back(q.text(i));result.push_back(row);}return result;
  }
  void exec(const char* sql,const std::vector<std::string>& args){Statement q(db,sql,args);q.next();}
public:
  Ledger(const std::string& path,const std::string& fingerprint,bool create=false){
    if(!hex(fingerprint,64))throw std::runtime_error("Fingerprint non valido");
    if(sqlite3_open_v2(path.c_str(),&db,SQLITE_OPEN_READWRITE|(create?SQLITE_OPEN_CREATE:0),nullptr)!=SQLITE_OK){if(db)sqlite3_close(db);db=nullptr;throw std::runtime_error("Registro assente o inaccessibile. Ripristinare il registro originale.");}
    try {
      sqlite3_busy_timeout(db,30000);run("PRAGMA synchronous=FULL");run("BEGIN IMMEDIATE");
      if(create){run("CREATE TABLE IF NOT EXISTS publisher(fingerprint TEXT PRIMARY KEY)");run("CREATE TABLE IF NOT EXISTS licenses(id TEXT PRIMARY KEY,customer TEXT NOT NULL)");run("CREATE TABLE IF NOT EXISTS activations(license TEXT NOT NULL,device TEXT NOT NULL,PRIMARY KEY(license,device))");}
      auto saved=rows("SELECT fingerprint FROM publisher");
      if(saved.empty()&&create)exec("INSERT INTO publisher VALUES(?)",{fingerprint});
      else if(saved.size()!=1||saved[0][0]!=fingerprint)throw std::runtime_error("Il registro non appartiene alla chiave del plugin.");
      // Also validate the original issuer tables before accepting an imported ledger.
      rows("SELECT id,customer FROM licenses LIMIT 1");rows("SELECT license,device FROM activations LIMIT 1");
      run("CREATE TABLE IF NOT EXISTS trials(device TEXT PRIMARY KEY,start INTEGER NOT NULL,end INTEGER NOT NULL)");
      run("COMMIT");
    }catch(...){sqlite3_exec(db,"ROLLBACK",nullptr,nullptr,nullptr);sqlite3_close(db);db=nullptr;throw;}
  }
  ~Ledger(){if(db)sqlite3_close(db);}
  Ledger(const Ledger&)=delete;Ledger& operator=(const Ledger&)=delete;
  // The caller signs while the transaction is locked. Nothing is exposed until
  // COMMIT succeeds. Re-exporting a device never uses an additional slot.
  std::string issue(bool trial,const std::string& device,const std::string& customer,
                    const std::string& requestedId,const std::string& newId,int64_t now,
                    const std::function<std::string(const Grant&)>& sign,Grant& grant){
    if(!deviceValid(device))throw std::runtime_error("Machine ID non valida: incollare GDDEV2- completo oppure i 64 caratteri Windows.");
    run("BEGIN IMMEDIATE");
    try {
      Grant g;
      if(trial){
        auto prior=rows("SELECT start,end FROM trials WHERE device=?",{device});
        if(prior.empty()){
          if(now<=0||now>4102444800LL)throw std::runtime_error("Controllare la data del Mac.");
          g.start=now;g.end=now+1209600;
          exec("INSERT INTO trials VALUES(?,?,?)",{device,std::to_string(g.start),std::to_string(g.end)});
        }else{g.start=std::stoll(prior[0][0]);g.end=std::stoll(prior[0][1]);}
        if(g.start<=0||g.start>4102444800LL||g.end<=g.start||g.end-g.start!=1209600)throw std::runtime_error("Periodo trial nel registro non valido.");
        if(macDevice(device))g.payload="GRAINS-DOSAGE-TRIAL-V2:"+device.substr(7)+":"+std::to_string(g.start)+":"+std::to_string(g.end);
        else g.payload="GRAINS-TRIAL-1\n"+device+"\n"+std::to_string((g.start+11644473600LL)*10000000LL)+"\n"+std::to_string((g.end+11644473600LL)*10000000LL)+"\n";
      }else{
        const auto name=customerKey(customer);if(name.empty())throw std::runtime_error("Inserire il nome cliente.");
        auto customers=rows("SELECT id,customer FROM licenses");
        if(!requestedId.empty()){
          if(!licenseValid(requestedId))throw std::runtime_error("License ID non valido.");
          for(const auto& c:customers)if(c[0]==requestedId&&customerKey(c[1])==name)g.license=requestedId;
          if(g.license.empty())throw std::runtime_error("License ID non trovato per questo cliente nel registro originale.");
        }else{
          for(const auto& c:customers)if(customerKey(c[1])==name){if(!g.license.empty())throw std::runtime_error("Più licenze per questo cliente: inserire il License ID originale.");g.license=c[0];}
          if(g.license.empty()){if(!licenseValid(newId))throw std::runtime_error("Nuovo License ID non valido.");g.license=newId;exec("INSERT INTO licenses VALUES(?,?)",{g.license,customer});}
        }
        if(!licenseValid(g.license))throw std::runtime_error("License ID nel registro non valido.");
        auto devices=rows("SELECT device FROM activations WHERE license=?",{g.license});bool exists=false;
        for(const auto& d:devices)if(d[0]==device)exists=true;
        if(!exists){if(devices.size()>=3)throw std::runtime_error("Limite raggiunto: questa licenza ha già 3 dispositivi. Le attivazioni offline non si revocano.");exec("INSERT INTO activations VALUES(?,?)",{g.license,device});}
        g.devices=int(devices.size())+(exists?0:1);
        g.payload=macDevice(device)?"GRAINS-DOSAGE-V2:"+g.license.substr(5)+":"+device.substr(7)+":3":"GRAINS-PERMANENT-1\n"+device+"\n"+g.license.substr(5)+"\n3\n";
      }
      auto code=sign(g);if(code.empty())throw std::runtime_error("Firma non generata.");run("COMMIT");grant=g;return code;
    }catch(...){sqlite3_exec(db,"ROLLBACK",nullptr,nullptr,nullptr);throw;}
  }
};
}
