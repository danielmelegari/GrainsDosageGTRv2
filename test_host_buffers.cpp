// Exercise the real VST3 wrapper with scanner/lifecycle buffer shapes.
#include "src/plugin.cpp"
#include "public.sdk/source/vst/hosting/parameterchanges.h"
#include "public.sdk/source/common/memorystream.h"
#include <cassert>
#include <limits>
#include <iostream>

template<class Sample> void buffers(int symbolic) {
 auto processor=std::make_unique<Processor>();
 assert(processor->initialize(nullptr)==kResultOk);
 ProcessSetup setup{};setup.sampleRate=48000.;setup.maxSamplesPerBlock=128;setup.symbolicSampleSize=symbolic;
 for(double rate:{0.,-1.,std::numeric_limits<double>::infinity(),std::numeric_limits<double>::quiet_NaN()}){auto invalid=setup;invalid.sampleRate=rate;assert(processor->setupProcessing(invalid)==kInvalidArgument);}
 assert(processor->getState(nullptr)==kInvalidArgument);assert(processor->setState(nullptr)==kResultFalse);
 assert(processor->setupProcessing(setup)==kResultOk);assert(processor->setActive(true)==kResultOk);
 SpeakerArrangement stereo=SpeakerArr::kStereo;
 assert(processor->setBusArrangements(nullptr,1,&stereo,1)==kResultFalse);
 assert(processor->setBusArrangements(&stereo,1,nullptr,1)==kResultFalse);
 ParameterChanges changes(1);int32 index=0;
 changes.addParameterData(kMix,index)->addPoint(0,0.,index);
 ProcessData data{};data.symbolicSampleSize=symbolic;data.inputParameterChanges=&changes;
 // Zero-length flush must apply automation without dereferencing buses.
 data.numInputs=data.numOutputs=1;assert(processor->process(data)==kResultOk);
 data.numSamples=128;assert(processor->process(data)==kResultFalse);
 Sample inL[128]{},inR[128]{},outL[128]{},outR[128]{};
 Sample* ins[]={inL,inR};Sample* outs[]={outL,outR};
 AudioBusBuffers input{},output{};input.numChannels=output.numChannels=2;
 if constexpr(std::is_same_v<Sample,float>){input.channelBuffers32=ins;output.channelBuffers32=outs;}
 else {input.channelBuffers64=ins;output.channelBuffers64=outs;}
 data.inputs=&input;data.outputs=&output;
 auto silent=[&](){assert(processor->process(data)==kResultOk);for(int i=0;i<128;++i)assert(outL[i]==0.&&outR[i]==0.);assert(output.silenceFlags==3);};
 silent();
 // Restoring an unfrozen preset must release an already frozen live engine.
 MemoryStream unfrozen;assert(processor->getState(&unfrozen)==kResultOk);
 changes.clearQueue();changes.addParameterData(kFreeze,index)->addPoint(0,1.,index);
 assert(processor->process(data)==kResultOk);
 auto frozen=[&](){MemoryStream saved;assert(processor->getState(&saved)==kResultOk);saved.seek(0,IBStream::kIBSeekSet,nullptr);auto state=defaults();assert(loadState(&saved,state));return state[kFreeze]>=.5;};
 assert(frozen());unfrozen.seek(0,IBStream::kIBSeekSet,nullptr);
 assert(processor->setState(&unfrozen)==kResultOk);data.inputParameterChanges=nullptr;
 assert(processor->process(data)==kResultOk);assert(!frozen());
 // Two automation points in the same block must both reach the engine.
 changes.clearQueue();auto* freezeQueue=changes.addParameterData(kFreeze,index);
 freezeQueue->addPoint(0,1.,index);freezeQueue->addPoint(64,0.,index);data.inputParameterChanges=&changes;
 assert(processor->process(data)==kResultOk);assert(!frozen());data.inputParameterChanges=nullptr;
 ins[0]=ins[1]=nullptr;silent();
 // A completely missing input channel array is silence, not a crash.
 if constexpr(std::is_same_v<Sample,float>)input.channelBuffers32=nullptr;else input.channelBuffers64=nullptr;
 silent();outs[1]=nullptr;assert(processor->process(data)==kResultFalse);
 output.numChannels=1;assert(processor->process(data)==kResultOk);assert(output.silenceFlags==1);
 outs[0]=nullptr;assert(processor->process(data)==kResultFalse);
 processor->setActive(false);processor->terminate();
}

void randomizeBlock(){
 auto a=std::make_unique<Processor>(),b=std::make_unique<Processor>();
 ProcessSetup setup{};setup.sampleRate=48000;setup.maxSamplesPerBlock=128;setup.symbolicSampleSize=kSample32;
 for(auto* p:{a.get(),b.get()}){assert(p->initialize(nullptr)==kResultOk);assert(p->setupProcessing(setup)==kResultOk);assert(p->setActive(true)==kResultOk);}
 auto expected=defaults();uint32_t seed=0x9E3779B9u;
 aztec::randomizeSound(seed,[&](ParamID id){return expected[id];},[&](ParamID id,double value){expected[id]=value;});
 ParameterChanges trigger(1),direct(kCount);int32 index=0;
 trigger.addParameterData(kRandomAll,index)->addPoint(0,1.,index);
 for(int id=0;id<kCount;++id)if(!isMonitor(id)&&expected[id]!=defaults()[id])direct.addParameterData(id,index)->addPoint(0,expected[id],index);
 float left[128],right[128],aL[128]{},aR[128]{},bL[128]{},bR[128]{};
 for(int i=0;i<128;++i){left[i]=float(std::sin(i*.12)*.2);right[i]=float(std::cos(i*.17)*.3);}
 float* ins[]={left,right};float* ao[]={aL,aR};float* bo[]={bL,bR};AudioBusBuffers input{},outA{},outB{};
 input.numChannels=outA.numChannels=outB.numChannels=2;input.channelBuffers32=ins;outA.channelBuffers32=ao;outB.channelBuffers32=bo;
 ProcessData data{};data.numSamples=128;data.numInputs=data.numOutputs=1;data.symbolicSampleSize=kSample32;data.inputs=&input;
 data.outputs=&outA;data.inputParameterChanges=&trigger;assert(a->process(data)==kResultOk);
 data.outputs=&outB;data.inputParameterChanges=&direct;assert(b->process(data)==kResultOk);
 double energy=0.;for(int i=0;i<128;++i){assert(aL[i]==bL[i]&&aR[i]==bR[i]);energy+=double(aL[i])*aL[i]+double(aR[i])*aR[i];}assert(energy>0.);
 for(auto* p:{a.get(),b.get()}){p->setActive(false);p->terminate();}
}

class CountedQueue final:public ParameterValueQueue {
public:
 int checks=0;
 CountedQueue():ParameterValueQueue(kMix){}
 int32 PLUGIN_API getPointCount()override{++checks;return ParameterValueQueue::getPointCount();}
};
class CountedChanges final:public ParameterChanges {
public:
 CountedQueue queue;
 int32 PLUGIN_API getParameterCount()override{return 1;}
 IParamValueQueue* PLUGIN_API getParameterData(int32 i)override{return i==0?&queue:nullptr;}
};
void consumedAutomation(){
 auto p=std::make_unique<Processor>();assert(p->initialize(nullptr)==kResultOk);
 ProcessSetup setup{};setup.sampleRate=48000;setup.maxSamplesPerBlock=2048;setup.symbolicSampleSize=kSample32;
 assert(p->setupProcessing(setup)==kResultOk);assert(p->setActive(true)==kResultOk);
 float l[2048]{},r[2048]{};float* channels[]={l,r};AudioBusBuffers bus{};bus.numChannels=2;bus.channelBuffers32=channels;
 CountedChanges changes;int32 index=0;changes.queue.addPoint(0,.3,index);
 ProcessData data{};data.symbolicSampleSize=kSample32;data.numSamples=2048;data.numInputs=data.numOutputs=1;
 data.inputs=data.outputs=&bus;data.inputParameterChanges=&changes;
 assert(p->process(data)==kResultOk);
 std::cerr<<"Point-count calls for one consumed automation event: "<<changes.queue.checks<<"\n";
 assert(changes.queue.checks<=3);
 p->setActive(false);p->terminate();
}
// Compare mixed queues in one block with the same events split across blocks.
// An early-exhausted queue must not discard or move another queue's later points.
std::vector<float> automatedRender(int block){
 auto p=std::make_unique<Processor>();assert(p->initialize(nullptr)==kResultOk);
 ProcessSetup setup{};setup.sampleRate=48000;setup.maxSamplesPerBlock=128;setup.symbolicSampleSize=kSample32;
 assert(p->setupProcessing(setup)==kResultOk);assert(p->setActive(true)==kResultOk);
 struct Event{int sample;ParamID id;double value;};
 const Event events[]={{0,kMix,.8},{0,kPitch,.5},{17,kMix,.2},{32,kFreeze,1.},{64,kPitch,.6},{65,kFreeze,0.},{127,kMix,.9}};
 std::vector<float> result(1024),left(128),right(128),outR(128);
 float* ins[]={left.data(),right.data()};float* outs[]={nullptr,outR.data()};
 AudioBusBuffers in{},out{};in.numChannels=out.numChannels=2;in.channelBuffers32=ins;out.channelBuffers32=outs;
 ProcessData data{};data.symbolicSampleSize=kSample32;data.numSamples=block;data.numInputs=data.numOutputs=1;data.inputs=&in;data.outputs=&out;
 ParameterChanges changes(3);data.inputParameterChanges=&changes;
 for(int start=0;start<int(result.size());start+=block){
  changes.clearQueue();int32 index=0;
  for(auto e:events)if(e.sample>=start&&e.sample<start+block)changes.addParameterData(e.id,index)->addPoint(e.sample-start,e.value,index);
  for(int i=0;i<block;++i){left[i]=float(.2*std::sin((start+i)*.037));right[i]=float(.15*std::cos((start+i)*.053));}
  outs[0]=result.data()+start;assert(p->process(data)==kResultOk);
 }
 p->setActive(false);p->terminate();return result;
}
void automationTiming(){
 const auto whole=automatedRender(128),split=automatedRender(16);
 double maximum=0.;for(size_t i=0;i<whole.size();++i)maximum=std::max(maximum,std::abs(double(whole[i])-split[i]));
 std::cerr<<"Mixed-queue automation block-size difference: "<<maximum<<"\n";assert(maximum<1e-6);
}

void transportTransitions(){
 aztec::HostTransport clock;double beat=0.;constexpr double inc=1./24000.;
 assert(!clock.update(beat,0.,true,true,128,inc));beat+=128*inc;
 assert(!clock.update(beat,128*inc,true,true,128,inc));beat+=128*inc;
 assert(clock.update(beat,8.,true,true,128,inc)); // seek
 beat+=128*inc;assert(clock.update(beat,0.,true,true,128,inc)); // loop wrap
 beat+=128*inc;assert(clock.update(beat,beat,true,false,128,inc)); // stop once
 double stopped=beat;beat+=128*inc;
 assert(!clock.update(beat,stopped,true,false,128,inc));assert(beat>stopped);
 assert(!clock.update(beat,99.,true,true,0,inc)); // flush is not a seek
 assert(clock.update(beat,stopped,true,true,128,inc));assert(beat==stopped); // resume
 beat+=128*inc;assert(!clock.update(beat,0.,false,true,128,inc)); // missing context
 clock.reset();beat=0.;assert(!clock.update(beat,0.,true,false,128,inc));
}

// A stopped host's fixed playhead must not reset active effects each block.
// Rendering the same live input in different block sizes must agree.
std::vector<float> stoppedRender(int block){
 auto p=std::make_unique<Processor>();assert(p->initialize(nullptr)==kResultOk);
 ProcessSetup setup{};setup.sampleRate=48000;setup.maxSamplesPerBlock=512;setup.symbolicSampleSize=kSample32;
 assert(p->setupProcessing(setup)==kResultOk);assert(p->setActive(true)==kResultOk);
 ProcessContext context{};context.state=ProcessContext::kProjectTimeMusicValid|ProcessContext::kTempoValid;
 context.projectTimeMusic=8.;context.tempo=120.;
 std::vector<float> result(16384),right(512),left(512),outR(512);
 float* ins[]={left.data(),right.data()};float* outs[]={nullptr,outR.data()};
 AudioBusBuffers in{},out{};in.numChannels=out.numChannels=2;in.channelBuffers32=ins;out.channelBuffers32=outs;
 ProcessData data{};data.symbolicSampleSize=kSample32;data.numSamples=block;data.numInputs=data.numOutputs=1;
 data.inputs=&in;data.outputs=&out;data.processContext=&context;
 for(int start=0;start<int(result.size());start+=block){
  for(int i=0;i<block;++i){left[i]=float(.2*std::sin((start+i)*.037));right[i]=float(.15*std::cos((start+i)*.053));}
  outs[0]=result.data()+start;assert(p->process(data)==kResultOk);
 }
 p->setActive(false);p->terminate();return result;
}
void stoppedTransport(){
 auto small=stoppedRender(128),large=stoppedRender(512);
 double maximum=0.;for(size_t i=0;i<small.size();++i)maximum=std::max(maximum,std::abs(double(small[i])-large[i]));
 std::cerr<<"Stopped transport block-size audio difference: "<<maximum<<"\n";
 assert(maximum<1e-6);
}

int main(){
 consumedAutomation();automationTiming();transportTransitions();stoppedTransport();
 randomizeBlock();
 buffers<float>(kSample32);buffers<double>(kSample64);
 // At each host block size/rate, monitoring must leave audio bit-identical
 // and publish at most about ten waveform batches per second.
 for(double rate:{44100.,48000.,96000.})for(int block:{32,256,2048}){
  auto a=std::make_unique<Processor>(),b=std::make_unique<Processor>();
  ProcessSetup setup{};setup.sampleRate=rate;setup.maxSamplesPerBlock=block;setup.symbolicSampleSize=kSample32;
  for(auto* p:{a.get(),b.get()}){assert(p->initialize(nullptr)==kResultOk);assert(p->setupProcessing(setup)==kResultOk);assert(p->setActive(true)==kResultOk);}
  float inL[2048]{},inR[2048]{},aL[2048]{},aR[2048]{},bL[2048]{},bR[2048]{};
  for(int i=0;i<block;++i){inL[i]=float(std::sin(i*.1)*.2);inR[i]=float(std::cos(i*.13)*.1);}
  float* ins[]={inL,inR};float* ao[]={aL,aR};float* bo[]={bL,bR};
  AudioBusBuffers input{},outA{},outB{};input.numChannels=outA.numChannels=outB.numChannels=2;
  input.channelBuffers32=ins;outA.channelBuffers32=ao;outB.channelBuffers32=bo;
  ParameterChanges monitor(kCount);ProcessData data{};data.symbolicSampleSize=kSample32;data.numSamples=block;data.numInputs=data.numOutputs=1;data.inputs=&input;
  double windowPeak=0.;
  std::array<double,kCount> lastTelemetry;lastTelemetry.fill(-1.);
  int batches=0,meterBatches=0;for(int sample=0;sample<int(rate);sample+=block){
   monitor.clearQueue();data.outputs=&outA;data.outputParameterChanges=&monitor;assert(a->process(data)==kResultOk);
   for(int i=0;i<block;++i){windowPeak=std::max({windowPeak,std::abs(double(aL[i])),std::abs(double(aR[i]))});}
   bool metered=false;
   for(int i=0;i<monitor.getParameterCount();++i){auto* q=monitor.getParameterData(i);int32 offset=0;double v=0.;
    assert(q->getParameterId()!=kUiOutputL&&q->getParameterId()!=kUiOutputR); // removed display bars must produce no RMS traffic
    if(q->getParameterId()==kUiLevel)metered=true;
    if(q->getParameterId()==kUiStep+3){assert(q->getPoint(0,offset,v)==kResultOk);assert(v==std::clamp(windowPeak,0.,1.));}
    if(q->getParameterId()==kUiWave0)++batches;
    auto id=q->getParameterId();
    bool meter=id==kUiOutputL||id==kUiOutputR||id==kUiReverb||(id>=kUiStep&&id<kUiStep+4);
    if(!meter&&id!=kUiWave0){assert(q->getPoint(0,offset,v)==kResultOk);assert(v!=lastTelemetry[id]);lastTelemetry[id]=v;}

   }
   if(metered)windowPeak=0.;
   for(int i=0;i<monitor.getParameterCount();++i)if(monitor.getParameterData(i)->getParameterId()==kUiLevel)++meterBatches;
   data.outputs=&outB;data.outputParameterChanges=nullptr;assert(b->process(data)==kResultOk);
   for(int i=0;i<block;++i)assert(aL[i]==bL[i]&&aR[i]==bR[i]);
  }
  assert(batches>=7&&batches<=11);assert(meterBatches>=15&&meterBatches<=31);
  for(auto* p:{a.get(),b.get()}){p->setActive(false);p->terminate();}
 }
 std::cout<<"PASS: 32/64-bit null host buffers, automation flush, 10 Hz waveform telemetry, bit-identical audio with/without monitor sink at 44.1/48/96 kHz and 32/256/2048 frames\n";
}
