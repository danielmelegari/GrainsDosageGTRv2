#include "src/parameter_settings.h"
#include "src/factory_presets.h"
#include <cassert>
#include <iostream>
int main(){using namespace aztec;auto p=initialParameters();
 for(int old=0;old<5;++old){p[kModWaveRnd0]=old/4.;p[kModWaveRate0]=0;assert(settings(p,120,48000).lfos[0].waveRandom==old);}
 for(int selected=1;selected<=7;++selected){p[kModWaveRate0]=selected/7.;assert(settings(p,120,48000).lfos[0].waveRandom==selected-1);}
 for(int choice=5;choice<=6;++choice){qg::Lfo l;l.prepare(12345);qg::LfoSettings s;s.enabled=true;s.waveRandom=choice;double interval=choice==5?8.:16.;int changes=0,previous=-1;
  for(int step=-10;step<100;++step){l.process(s,step*interval+.001,48000,false,0);int wave=l.wave();assert(wave>=0&&wave<128);if(wave!=previous)++changes;previous=wave;l.process(s,(step+1)*interval-.001,48000,false,0);assert(l.wave()==wave);l.process(s,step*interval+.25*interval,48000,false,0);assert(l.wave()==wave);}
  assert(changes>80);
 }
 std::cout<<"PASS: saved MOD W rates, new selections and 8/16-beat quantization across negative positions and seeks\n";
}
