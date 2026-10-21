#include "src/plugin.cpp"
#include "public.sdk/source/vst/hosting/hostclasses.h"
#include "public.sdk/source/vst/hosting/parameterchanges.h"
#include <cassert>
#include <iostream>
#include <cstdlib>
#include <new>

static bool audioThread=false;
static size_t audioAllocations=0;
void* operator new(std::size_t n){if(audioThread)++audioAllocations;if(void* p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
void* operator new[](std::size_t n){return ::operator new(n);}
void operator delete(void* p)noexcept{std::free(p);}
void operator delete[](void* p)noexcept{::operator delete(p);}
void operator delete(void* p,std::size_t)noexcept{::operator delete(p);}
void operator delete[](void* p,std::size_t)noexcept{::operator delete(p);}
class CountingHost final:public HostApplication {
public:
 int messages=0;
 tresult PLUGIN_API createInstance(TUID cid,TUID iid,void** obj)override{
  assert(!audioThread);++messages;return HostApplication::createInstance(cid,iid,obj);
 }
};

class EditCounter final:public IComponentHandler {
 uint32 refs=1;
public:
 int calls=0;
 tresult PLUGIN_API queryInterface(const TUID iid,void** obj)override{
  if(!obj)return kInvalidArgument;*obj=nullptr;
  if(FUnknownPrivate::iidEqual(iid,FUnknown::iid)||FUnknownPrivate::iidEqual(iid,IComponentHandler::iid)){*obj=static_cast<IComponentHandler*>(this);addRef();return kResultOk;}return kNoInterface;
 }
 uint32 PLUGIN_API addRef()override{return ++refs;}uint32 PLUGIN_API release()override{return --refs;}
 tresult PLUGIN_API beginEdit(ParamID)override{++calls;return kResultOk;}
 tresult PLUGIN_API performEdit(ParamID,ParamValue)override{++calls;return kResultOk;}
 tresult PLUGIN_API endEdit(ParamID)override{++calls;return kResultOk;}
 tresult PLUGIN_API restartComponent(int32)override{++calls;return kResultOk;}
};

template<class Sample> void check(int format){
 CountingHost host;EditCounter counter;
 auto processor=owned(new Processor),reference=owned(new Processor);
 auto controller=owned(new Controller),other=owned(new Controller);
 for(auto* p:{processor.get(),reference.get()})assert(p->initialize(&host)==kResultOk);
 for(auto* c:{controller.get(),other.get()})assert(c->initialize(&host)==kResultOk);
 controller->setComponentHandler(&counter);
 // Retaining editor state simulates an open native view, including on Linux.
 auto view=controller->editorState();
 assert(processor->connect(controller)==kResultOk);assert(controller->connect(processor)==kResultOk);
 ProcessSetup setup{};setup.sampleRate=48000;setup.maxSamplesPerBlock=128;setup.symbolicSampleSize=format;
 for(auto* p:{processor.get(),reference.get()}){assert(p->setupProcessing(setup)==kResultOk);assert(p->setActive(true)==kResultOk);}
 controller->onTimer(nullptr); // negotiate the message transport before processing
 Sample left[128],right[128],outL[128]{},outR[128]{},refL[128]{},refR[128]{};
 for(int i=0;i<128;++i){left[i]=Sample(.2*std::sin(i*.1));right[i]=Sample(.15*std::cos(i*.13));}
 Sample* in[]={left,right};Sample* out[]={outL,outR};Sample* ref[]={refL,refR};
 AudioBusBuffers input{},output{},refOutput{};input.numChannels=output.numChannels=refOutput.numChannels=2;
 if constexpr(std::is_same_v<Sample,float>){input.channelBuffers32=in;output.channelBuffers32=out;refOutput.channelBuffers32=ref;}
 else{input.channelBuffers64=in;output.channelBuffers64=out;refOutput.channelBuffers64=ref;}
 ParameterChanges telemetry(kCount),automation(1);int32 index=0;
 automation.addParameterData(kMix,index)->addPoint(0,0.,index);
 ProcessData data{};data.symbolicSampleSize=format;data.numSamples=128;data.numInputs=data.numOutputs=1;
 data.inputs=&input;data.inputParameterChanges=&automation;
 for(int block=0;block<400;++block){
  telemetry.clearQueue();data.outputs=&output;data.outputParameterChanges=&telemetry;
  audioThread=true;assert(processor->process(data)==kResultOk);audioThread=false;assert(audioAllocations==0);
  assert(telemetry.getParameterCount()==0); // no hidden display data in host automation
  data.outputs=&refOutput;data.outputParameterChanges=nullptr;assert(reference->process(data)==kResultOk);
  for(int i=0;i<128;++i)assert(outL[i]==refL[i]&&outR[i]==refR[i]);
  if(block%25==0)controller->onTimer(nullptr);
 }
 controller->onTimer(nullptr);
 assert(controller->getParamNormalized(kUiOutputL)>0);
 assert(other->getParamNormalized(kUiOutputL)==0); // no cross-instance display globals
 assert(counter.calls==0); // display delivery never announces a control edit/restart
 auto tab=controller->getParamNormalized(kUiTab),mix=controller->getParamNormalized(kMix);
 // Reject malformed packets and never overwrite editable controls/navigation.
 auto message=owned(new HostMessage);message->setMessageID("GrainsDosage.Display.Values.v1");
 double invalid=1.;message->getAttributes()->setBinary("values",&invalid,sizeof(invalid));
 assert(controller->notify(message)==kInvalidArgument);
 std::array<double,kCount> packet{};packet.fill(.123);
 message->getAttributes()->setBinary("values",packet.data(),sizeof(packet));assert(controller->notify(message)==kResultOk);
 assert(controller->getParamNormalized(kUiTab)==tab&&controller->getParamNormalized(kMix)==mix);
 assert(counter.calls==0);
 // A disconnected host retains the legacy monitor route rather than losing all UI feedback.
 assert(processor->disconnect(controller)==kResultOk);assert(controller->disconnect(processor)==kResultOk);
 int fallback=0;data.outputs=&output;data.outputParameterChanges=&telemetry;
 for(int i=0;i<50;++i){telemetry.clearQueue();assert(processor->process(data)==kResultOk);fallback+=telemetry.getParameterCount();}
 assert(fallback>0);
 // With no remaining view, the polling callback sends no messages.
 view.reset();int before=host.messages;controller->onTimer(nullptr);assert(host.messages==before);
 controller->setComponentHandler(nullptr);
 for(auto* p:{processor.get(),reference.get()}){p->setActive(false);p->terminate();}
 controller->terminate();other->terminate();
}
int main(){check<float>(kSample32);check<double>(kSample64);std::cout<<"PASS: message display transport, zero host telemetry/edits, isolated instances, unchanged 32/64-bit audio, malformed-packet guard and fallback\n";}
