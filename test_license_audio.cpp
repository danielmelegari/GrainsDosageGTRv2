// Exercise the real VST3 process() with a test authorization provider. Signature,
// machine and protected-store behavior are covered separately by native tests.
#include "src/plugin.cpp"
#include "public.sdk/source/vst/hosting/parameterchanges.h"
#include <cassert>
#include <iostream>
namespace aztec::license {
void initialize(){} void firstOpen(){} std::string deviceCode(){return {};}
std::string status(){return {};}
bool activate(const std::string&,std::string&){return false;}
}
int main(){
 constexpr int size=128;constexpr double rate=48000.;
 for(int format:{kSample32,kSample64}){
  aztec::license::allowed.store(true);auto* p=new Processor;assert(p->initialize(nullptr)==kResultOk);
  ProcessSetup setup{};setup.processMode=kRealtime;setup.symbolicSampleSize=format;setup.maxSamplesPerBlock=size;setup.sampleRate=rate;
  assert(p->setupProcessing(setup)==kResultOk);assert(p->setActive(true)==kResultOk);
  float fL[size],fR[size],foL[size]{},foR[size]{};double dL[size],dR[size],doL[size]{},doR[size]{};
  for(int i=0;i<size;++i){fL[i]=dL[i]=2.;fR[i]=dR[i]=-.5;}
  float* fi[]={fL,fR};float* fo[]={foL,foR};double* di[]={dL,dR};double* dout[]={doL,doR};
  AudioBusBuffers input{},output{};input.numChannels=output.numChannels=2;
  if(format==kSample32){input.channelBuffers32=fi;output.channelBuffers32=fo;}else{input.channelBuffers64=di;output.channelBuffers64=dout;}
  ParameterChanges changes(6);auto put=[&](ParamID id,double value){int32 idx=0;changes.addParameterData(id,idx)->addPoint(0,value,idx);};
  put(kMix,0.);put(kMasterLimiter,1.);put(kLimiterCeiling,.5);put(kInputDeclick,0.);
  ProcessData data{};data.symbolicSampleSize=format;data.numSamples=size;data.numInputs=data.numOutputs=1;data.inputs=&input;data.outputs=&output;data.inputParameterChanges=&changes;
  auto left=[&](int i){return format==kSample32?double(foL[i]):doL[i];};
  assert(p->process(data)==kResultOk);changes.clearQueue();
  // Valid trial / permanent authorization: real limiter/DSP is active, nonzero.
  for(int b=0;b<16;++b)assert(p->process(data)==kResultOk);
  assert(left(size-1)>.1&&left(size-1)<.51);assert(!(output.silenceFlags&1));
  double previous=left(size-1);aztec::license::allowed.store(false);
  for(int b=0;b<10;++b){assert(p->process(data)==kResultOk);for(int i=0;i<size;++i){assert(std::isfinite(left(i)));assert(std::abs(left(i)-previous)<.01);previous=left(i);}}
  // Expired/invalid license disables processing, not the DAW signal.
  for(int i=0;i<size;++i)assert(left(i)==2.);assert(!(output.silenceFlags&1));
  aztec::license::allowed.store(true);previous=left(size-1);
  for(int b=0;b<12;++b){assert(p->process(data)==kResultOk);for(int i=0;i<size;++i){assert(std::abs(left(i)-previous)<.02);previous=left(i);}}
  assert(left(size-1)>.1&&left(size-1)<.51);
  aztec::license::allowed.store(false);for(int b=0;b<10;++b)assert(p->process(data)==kResultOk);
  // Disabled effect retains the advertised lookahead/PDC, including exact doubles.
  for(int i=0;i<size;++i){fL[i]=float(.001*i);dL[i]=.001*i+1e-12;}
  assert(p->process(data)==kResultOk);
  for(int i=32;i<size;++i)assert(left(i)==(format==kSample32?double(fL[i-32]):dL[i-32]));
  p->setActive(false);p->terminate();p->release();
 }
 std::cout<<"PASS: real 32/64-bit VST3 audio stays active under authorization, fades to dry on expiry and recovers on activation without a DAW restart\n";
}
