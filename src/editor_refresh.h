#pragma once
#include <chrono>
namespace aztec {
// Only drawing is coalesced: host parameter edits remain synchronous.
// Explicit times make burst handling deterministic in the regression test.
class EditorRefresh {
  bool pending_=false;
  double nextInteraction_=0.,nextBackground_=0.;
public:
  static double now(){return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();}
  void request(){pending_=true;}
  bool interactionDue(double time){
    if(!pending_||time<nextInteraction_)return false;
    pending_=false;nextInteraction_=time+1./60.;return true;
  }
  bool backgroundDue(double time,double interval=1./30.){
    if(time<nextBackground_)return false;
    nextBackground_=time+interval;return true;
  }
};
}
