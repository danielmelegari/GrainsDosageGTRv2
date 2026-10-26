#pragma once
#include <array>
#include <cstdint>
namespace aztec {
// UI-thread-only, allocation-free queue for one dragged control or an XY pair.
// Local parameter values are updated before enqueueing; only host notifications
// are rate-limited. Discrete button/menu edits never enter this queue.
class HostEditQueue {
 struct Slot {std::uint32_t id=0;double value=0.;bool used=false,pending=false;};
 std::array<Slot,2> slots_{};
 double next_=0.;
public:
 void queue(std::uint32_t id,double value){
  for(auto& slot:slots_)if(slot.used&&slot.id==id){slot.value=value;slot.pending=true;return;}
  for(auto& slot:slots_)if(!slot.used){slot={id,value,true,true};return;}
 }
 template<class Send> void flush(double now,Send send,bool force=false){
  if(!force&&now<next_)return;
  bool sent=false;
  for(auto& slot:slots_)if(slot.pending){auto id=slot.id;auto v=slot.value;slot.pending=false;send(id,v);sent=true;}
  if(sent)next_=now+1./60.;
 }
 void clear(){slots_={};next_=0.;}
};
}
