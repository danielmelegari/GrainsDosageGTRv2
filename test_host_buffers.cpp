// Exercise the real VST3 wrapper with scanner/lifecycle buffer shapes.
#include "src/plugin.cpp"
#include "public.sdk/source/vst/hosting/parameterchanges.h"
#include <cassert>
#include <iostream>

template<class Sample> void buffers(int symbolic) {
 auto processor=std::make_unique<Processor>();
 assert(processor->initialize(nullptr)==kResultOk);
 ProcessSetup setup{};setup.sampleRate=48000.;setup.maxSamplesPerBlock=128;setup.symbolicSampleSize=symbolic;
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
 silent();ins[0]=ins[1]=nullptr;silent();
 // A completely missing input channel array is silence, not a crash.
 if constexpr(std::is_same_v<Sample,float>)input.channelBuffers32=nullptr;else input.channelBuffers64=nullptr;
 silent();outs[1]=nullptr;assert(processor->process(data)==kResultFalse);
 output.numChannels=1;assert(processor->process(data)==kResultOk);assert(output.silenceFlags==1);
 outs[0]=nullptr;assert(processor->process(data)==kResultFalse);
 processor->setActive(false);processor->terminate();
}

int main(){
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
  int batches=0;for(int sample=0;sample<int(rate);sample+=block){
   monitor.clearQueue();data.outputs=&outA;data.outputParameterChanges=&monitor;assert(a->process(data)==kResultOk);
   for(int i=0;i<monitor.getParameterCount();++i)if(monitor.getParameterData(i)->getParameterId()==kUiWave0)++batches;
   data.outputs=&outB;data.outputParameterChanges=nullptr;assert(b->process(data)==kResultOk);
   for(int i=0;i<block;++i)assert(aL[i]==bL[i]&&aR[i]==bR[i]);
  }
  assert(batches>=7&&batches<=11);
  for(auto* p:{a.get(),b.get()}){p->setActive(false);p->terminate();}
 }
 std::cout<<"PASS: 32/64-bit null host buffers, automation flush, 10 Hz waveform telemetry, bit-identical audio with/without monitor sink at 44.1/48/96 kHz and 32/256/2048 frames\n";
}
