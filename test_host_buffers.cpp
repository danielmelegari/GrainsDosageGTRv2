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

int main(){
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
  double windowSquare=0.,windowPeak=0.;int windowSamples=0;
  int batches=0,meterBatches=0;for(int sample=0;sample<int(rate);sample+=block){
   monitor.clearQueue();data.outputs=&outA;data.outputParameterChanges=&monitor;assert(a->process(data)==kResultOk);
   for(int i=0;i<block;++i){windowSquare+=double(aL[i])*aL[i];windowPeak=std::max({windowPeak,std::abs(double(aL[i])),std::abs(double(aR[i]))});}windowSamples+=block;
   bool metered=false;
   for(int i=0;i<monitor.getParameterCount();++i){auto* q=monitor.getParameterData(i);int32 offset=0;double v=0.;
    if(q->getParameterId()==kUiOutputL){assert(q->getPoint(0,offset,v)==kResultOk);assert(std::abs(v-std::clamp(std::sqrt(windowSquare/windowSamples),0.,1.))<1e-12);metered=true;}
    if(q->getParameterId()==kUiStep+3){assert(q->getPoint(0,offset,v)==kResultOk);assert(v==std::clamp(windowPeak,0.,1.));}
    if(q->getParameterId()==kUiWave0)++batches;
   }
   if(metered){windowSquare=windowPeak=0.;windowSamples=0;}
   for(int i=0;i<monitor.getParameterCount();++i)if(monitor.getParameterData(i)->getParameterId()==kUiOutputL)++meterBatches;
   data.outputs=&outB;data.outputParameterChanges=nullptr;assert(b->process(data)==kResultOk);
   for(int i=0;i<block;++i)assert(aL[i]==bL[i]&&aR[i]==bR[i]);
  }
  assert(batches>=7&&batches<=11);assert(meterBatches>=15&&meterBatches<=31);
  for(auto* p:{a.get(),b.get()}){p->setActive(false);p->terminate();}
 }
 std::cout<<"PASS: 32/64-bit null host buffers, automation flush, 10 Hz waveform telemetry, bit-identical audio with/without monitor sink at 44.1/48/96 kHz and 32/256/2048 frames\n";
}
