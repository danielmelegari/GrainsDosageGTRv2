// Exercise the actual plugin state reader and controller parameter definitions.
#include "src/plugin.cpp"
#include "public.sdk/source/common/memorystream.h"
#include <cassert>
#include <iostream>
#include <limits>
#include "public.sdk/source/vst/hosting/parameterchanges.h"

int main() {
  static_assert(kLfo0==35,"Version 0.2 IDs must remain stable");
  static_assert(kLegacyCount==95,"Legacy IDs stable");
  for(int version=0;version<21;++version) {
    MemoryStream stream; IBStreamer writer(&stream,kLittleEndian);
    writer.writeInt32(0x51473130+version);
    const int count=version==0 ? 26 : (version==1 ? 35 : (version==2 ? 95 : (version==3 ? int(kModuleOrder) : (version==4 ? int(kExtraRoutes0) : (version==5 ? int(kGlitchMove) : (version==6 ? int(kMasterFilter) : (version==7 ? int(kNormalize) : (version==8 ? int(kReverbLength) : (version==9 ? int(kUiWave0) : (version==10 ? int(kLfoSlots0) : (version==11 ? int(kModWaveRnd0) : (version==12 ? int(kReverbSource) : (version==13 ? int(kLimiterCeiling) : (version==14 ? int(kGaterEnabled) : (version==15 ? int(kInputDeclick) : (version==16 ? int(kFilterModel) : (version==17 ? int(kGaterMinLength) : (version==18 ? int(kGlitchTriggerRate) : (version==19 ? int(kResliceRndOn) : kLegacySkinCount)))))))))))))))))));
    auto expected=defaults();if(version<16)expected[kInputDeclick]=0.;if(version<7)expected[kMasterLimiter]=0.;
    for(int i=0;i<count;++i) { expected[i]=double((i*7)%101)/100.; writer.writeDouble(expected[i]); }
    if(version<20&&count>kResliceLength)expected[kResliceLength]=expected[kResliceLength]<1./6.?2./3.:1.;
    if(version>=11&&version<20)migrateModSlots(expected);
    if(version<9){const double rt[]={.45,1.25,2.8,5.5,9.};expected[kReverbLength]=(rt[int(std::round(expected[kReverbType]*4.))]-.2)/19.8;}
    stream.seek(0,IBStream::kIBSeekSet,nullptr);
    auto actual=defaults(); assert(loadState(&stream,actual)); 
    if(version>=4){if(version<11)migrateRoutes(expected);assert(actual==expected);}
    else if(version==3) { expected[kModuleOrder]=1.;migrateRoutes(expected);assert(actual==expected); }
    else {
      assert(std::abs((actual[kPitch]-.5)*96.-(expected[kPitch]-.5)*24.)<1e-10);
      assert(std::abs(actual[kDensity]*31.-expected[kDensity]*7.)<1e-10);
      assert(actual[kDensityFlow]==0. && actual[kGlitchSeq]==0. && actual[kRepeatSeq]==0. && actual[kModuleOrder]==1.);
      for(int id:{int(kGlitchDivision),int(kRepeatDivision)}) if(id<count)
        assert(effectDivisions[int(std::round(actual[id]*15.))]==divisions[int(std::round(expected[id]*8.))]);
      for(int i=0;i<4;++i) {
        assert(actual[kRandomSteps0+i]==0.);
        if(version==2) {
          assert(lfoBeats[int(std::round(actual[lfoID(i,lGrid)]*20.))]==lfoBeats[int(std::round(expected[lfoID(i,lGrid)]*17.))]);
          assert(std::abs(settings(actual,120.,48000.).lfos[i].amount[2]*48.-(expected[lfoID(i,lRoute0+2)]-.5)*24.)<1e-10);
        }
      }
    }
    if(version<2) for(int i=0;i<4;++i) {
      assert(actual[lfoID(i,lEnabled)]==0.);
      for(int t=0;t<6;++t) assert(actual[lfoID(i,lRoute0+t)]==.5);
    }
  }
  // Previous split-format state must not consume newly appended fields.
  {
    auto expected=defaults(); expected[kSize]=.73; expected[kModuleOrder]=1.;
    expected[kFreeze]=1.; expected[kGrainBuffer]=.4;
    MemoryStream legacy; IBStreamer out(&legacy,kLittleEndian);out.writeInt32(0x51473145);
    for(int i=0;i<kParamEnd;++i)out.writeDouble(expected[i]);
    out.writeDouble(expected[kGrainBuffer]);out.writeDouble(expected[kFreeze]);
    for(int i=kFreeze+1;i<kLegacySkinCount;++i)out.writeDouble(expected[i]);
    legacy.seek(0,IBStream::kIBSeekSet,nullptr);auto actual=defaults();
    assert(loadState(&legacy,actual)&&actual==expected);
    assert(settings(actual,120.,48000.).routing==-1);
  }
  {
    auto p=defaults();p[slotTarget(0,0)]=1./qg::modTargetCount;p[slotAmount(0,0)]=.75;
    p[kRoutingOrder]=1.;assert(settings(p,120.,48000.).routing==119);
    for(int mode=0;mode<3;++mode){p[slotPolarity(0,0)]=mode*.5;auto l=settings(p,120.,48000.).lfos[0];
      assert(l.amount[0]==(mode==0?.5:0.));assert(l.positive[0]==(mode==1?.5:0.));assert(l.negative[0]==(mode==2?.5:0.));}
  }
  MemoryStream bad; IBStreamer writer(&bad,kLittleEndian);
  writer.writeInt32(0x51473132); writer.writeDouble(std::numeric_limits<double>::quiet_NaN());
  bad.seek(0,IBStream::kIBSeekSet,nullptr); auto state=defaults(); const auto before=state;
  assert(!loadState(&bad,state) && state==before);
  MemoryStream shortStream; IBStreamer shortWriter(&shortStream,kLittleEndian);
  shortWriter.writeInt32(0x51473132); shortWriter.writeDouble(.5);
  shortStream.seek(0,IBStream::kIBSeekSet,nullptr); assert(!loadState(&shortStream,state) && state==before);
  static_assert(kExtraRoutes0==177,"Existing IDs must not move");
  auto routing=defaults();
  for(int lfo=0;lfo<4;++lfo) for(int target=6;target<8;++target) {
    routing[routeID(lfo,target)]=.75;
    assert(settings(routing,120.,48000.).lfos[lfo].amount[target]==.5);
  }
  static_assert(kGlitchMove==185,"0.6.3 IDs stable");
  Controller controller; assert(controller.initialize(nullptr)==kResultOk);
  // The global BYPASS button was removed: kBypassReserved keeps its legacy enum slot
  // (state/preset alignment) but is no longer a registered VST parameter, so the
  // validator never sees a bypass proxy. The trailing FREEZE/BUFFER/UI-TRIGGER ids
  // are state-only slots and were never registered either; derive the expected
  // count dynamically by subtracting every id that has no parameter object, so
  // this assertion cannot rot when new tail ids are appended to the enum.
  int registeredIds=0; for(int i=0;i<int(kCount);++i) if(controller.getParameterObject(ParamID(i))) ++registeredIds;
  assert(controller.getParameterCount()==registeredIds);
  assert(controller.getParameterObject(ParamID(kBypassReserved))==nullptr);
  // Every registered id must expose its documented default; kBypassReserved is no
  // longer registered (bypass removed) and the tail FREEZE/BUFFER ids are state-only.
  // NOTE: getParamNormalized()/setParamNormalized() take a PARAMETER INDEX (order of
  // registration in Parameters), not a ParamID — the enum value and the registration
  // index only coincide while every id 0..kCount-1 is registered in ascending order,
  // which stopped being true when kBypassReserved (id 34) was un-registered. Reading
  // by index silently queried the wrong parameter for everything after slot 34.
  const auto def=defaults();
  for(int i=0;i<int(kCount);++i) {
    auto* param=controller.getParameterObject(ParamID(i));
    if(!param) continue;
    // setParamNormalized routes through the same container lookup as the host path;
    // verify write-by-ID/read-by-ID round-trips at the documented default.
    assert(controller.setParamNormalized(ParamID(i),def[i])==kResultTrue);
    assert(std::abs(controller.getParamNormalized(ParamID(i))-def[i])<1e-12);
  }
  for(int idx=0;idx<controller.getParameterCount();++idx) { ParameterInfo info{}; assert(controller.getParameterInfo(idx,info)==kResultTrue); assert(info.id!=ParamID(kBypassReserved)); }
  auto xy=defaults(); xy[kXYEnable]=1.; xy[kXYX]=1.; xy[kXYY]=0.;
  auto mapped=settings(xy,120.,48000.); assert(mapped.pitch==-48. && mapped.size>.18);
  xy[kXTarget]=double(kTranspose+1)/kXYX; assert(settings(xy,120.,48000.).transpose==48.);
  auto noGate=defaults();noGate[kStep0]=0.;noGate[kAttack]=1.;assert(settings(noGate,120.,48000.).pattern==0xffff);
  assert(settings(noGate,120.,48000.).attack==.002);
  for(int id=0;id<4;++id)assert(controller.getParameterObject(kUiStep+id)->getInfo().flags&ParameterInfo::kIsReadOnly);
  controller.terminate();
  auto* processor=new Processor;
  MemoryStream saved; assert(processor->getState(&saved)==kResultOk);
  saved.seek(0,IBStream::kIBSeekSet,nullptr); assert(loadState(&saved,state)); assert(state==defaults());
  assert(processor->initialize(nullptr)==kResultOk);
  ProcessSetup setup{};setup.processMode=kRealtime;setup.symbolicSampleSize=kSample32;setup.maxSamplesPerBlock=128;setup.sampleRate=48000.;
  assert(processor->setupProcessing(setup)==kResultOk);assert(processor->setActive(true)==kResultOk);
  float left[128],right[128],outL[128]{},outR[128]{};for(int i=0;i<128;++i){left[i]=2.f;right[i]=.5f;}
  float* ins[]={left,right};float* outs[]={outL,outR};AudioBusBuffers input{},output{};input.numChannels=output.numChannels=2;input.channelBuffers32=ins;output.channelBuffers32=outs;
  ParameterChanges changes(8),meters(kCount);auto put=[&](ParamID id,double v){int32 index=0;auto* q=changes.addParameterData(id,index);q->addPoint(0,v,index);};
  put(kMix,0.);put(kMasterLimiter,1.);ProcessData data{};data.symbolicSampleSize=kSample32;data.numSamples=128;data.numInputs=data.numOutputs=1;data.inputs=&input;data.outputs=&output;data.inputParameterChanges=&changes;data.outputParameterChanges=&meters;
  assert(processor->process(data)==kResultOk);assert(std::abs(outL[127])<=qg::MasterFx::ceiling+1e-6);
  for(double selected:{0.,.5,1.}){changes.clearQueue();put(kLimiterCeiling,selected);assert(processor->process(data)==kResultOk);double db=selected==0?0:selected==.5?-6:-10;for(int i=0;i<128;++i){assert(std::abs(double(outL[i]))<=std::pow(10.,db/20.));assert(std::abs(double(outR[i]))<=std::pow(10.,db/20.));}}
  changes.clearQueue();put(kMasterLimiter,0.);assert(processor->process(data)==kResultOk);assert(outL[127]==2.f&&outR[127]==.5f);
  changes.clearQueue();put(kMasterFilter,1.);put(kMix,1.);put(kGrainMix,0.);put(kFilterType,1.);put(kFilterCutoff,std::log(50.)/std::log(1000.));
  for(int i=0;i<80;++i)assert(processor->process(data)==kResultOk);
  assert(std::abs(outL[127])<1e-4); // HP rejects a DC source through real VST queues.
  bool gotWave=false,gotHead=false;
  for(int i=0;i<meters.getParameterCount();++i){auto* q=meters.getParameterData(i);int32 offset;double value;if(q->getPoint(q->getPointCount()-1,offset,value)==kResultOk){assert(std::isfinite(value)&&value>=0.&&value<=1.);if(q->getParameterId()>=kUiWave0&&q->getParameterId()<kUiGrainStart&&value>0.)gotWave=true;if(q->getParameterId()==kUiGrainHead)gotHead=true;}}
  assert(gotWave&&gotHead);
  // Exercise the final limiter in the actual 64-bit host output path as well.
  processor->setActive(false);setup.symbolicSampleSize=kSample64;assert(processor->setupProcessing(setup)==kResultOk);processor->setActive(true);
  double in64L[128],in64R[128],out64L[128]{},out64R[128]{};for(int i=0;i<128;++i){in64L[i]=3.;in64R[i]=-2.;}double* in64[]={in64L,in64R};double* out64[]={out64L,out64R};input.channelBuffers64=in64;output.channelBuffers64=out64;input.silenceFlags=0;data.symbolicSampleSize=kSample64;
  for(double selected:{0.,.5,1.}){changes.clearQueue();put(kMix,0.);put(kMasterFilter,0.);put(kMasterLimiter,1.);put(kLimiterCeiling,selected);assert(processor->process(data)==kResultOk);double db=selected==0?0:selected==.5?-6:-10;for(int i=0;i<128;++i)assert(std::abs(out64L[i])<=std::pow(10.,db/20.)&&std::abs(out64R[i])<=std::pow(10.,db/20.));}
  assert(processor->getLatencySamples()==32);
  // The global BYPASS button was removed: even a legacy kBypassReserved value of 1
  // must NOT mute processing or passthrough the dry signal anymore.
  {
    changes.clearQueue();put(kMasterLimiter,0.);put(kMix,0.);put(kInputDeclick,1.);put(kBypassReserved,1.);
    for(int i=0;i<128;++i)in64L[i]=in64R[i]=0.;
    for(int block=0;block<8;++block)assert(processor->process(data)==kResultOk);
    in64L[64]=.987654321123;assert(processor->process(data)==kResultOk);
    assert(std::abs(out64L[96])<1e-10); // wet path active (mix=0 => silent), not dry passthrough
    assert(out64R[96]==0.);
  }
  // Roundtrip: dirty the legacy kBypassReserved slot through a real VST queue, then
  // save/reload and verify getState->setState->getState is byte-for-byte stable even
  // though the slot is not a registered parameter (bypass button removed).
  {
    changes.clearQueue();put(kBypassReserved,1.);assert(processor->process(data)==kResultOk);
    MemoryStream rt; assert(processor->getState(&rt)==kResultOk);
    rt.seek(0,IBStream::kIBSeekSet,nullptr); auto rtState=defaults(); assert(loadState(&rt,rtState));
    assert(rtState[kBypassReserved]==1.); // preserved verbatim: the slot is inert but stays aligned
    rt.seek(0,IBStream::kIBSeekSet,nullptr); // rewind: loadState above left the cursor at EOF
    MemoryStream rt2; Processor p2; assert(p2.setState(&rt)==kResultOk);
    MemoryStream rt3; assert(p2.getState(&rt3)==kResultOk);
    rt.seek(0,IBStream::kIBSeekSet,nullptr); rt3.seek(0,IBStream::kIBSeekSet,nullptr);
    int32 m1=0,m2=0;IBStreamer r1(&rt,kLittleEndian),r2(&rt3,kLittleEndian);
    r1.readInt32(m1);r2.readInt32(m2);assert(m1==m2&&m1==0x51473146);
    // getState layout: kParamEnd values, buffer size, freeze flag, then the tail
    // (kBypassReserved lives in this region) up to kCount.
    const int fields=kParamEnd+2+(int(kCount)-int(kFreeze)-1);
    for(int i=0;i<fields;++i){double a=0,b=0;assert(r1.readDouble(a)&&r2.readDouble(b));assert(a==b);}
  }
  processor->setActive(false);processor->terminate();processor->release();
  std::cout<<"PASS: v0.1–v0.12.0 state migration and legacy parallel mode, disabled legacy routes, malformed state rejection, controller defaults, appended Speed/Transpose routes, XY routing and processor state roundtrip, master host automation\n";
}

