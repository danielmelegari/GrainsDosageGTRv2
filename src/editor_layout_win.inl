// Header and navigation never use the page scroll offset.
ADD(kInputDeclick,835,65,55,23,Toggle,"ON");
ADD(kDeclickSensitivity,895,62,108,27,Slider,"");
auto addBody=[&](ParamID id,double x,double y,double w,double h,mockup::Kind kind,const char* label){auto r=state.position({x,y,w,h});result.push_back({id,r,kind,label});};
#define BODY(ID,X,Y,W,H,K,L) addBody(ParamID(ID),X,Y,W,H,mockup::K,L)
if(state.page==0){
 BODY(kTranspose,350,260,140,122,Knob,"TRANSPOSE");
 BODY(kStretchSpeed,570,260,140,122,Knob,"STRETCH");
 BODY(kStretchOn,756,309,192,32,Toggle,"FREE STRETCH");
 for(int stage:mockup::routeChain(value)){
  auto r=mockup::moduleRect(stage,value,state);double y=r.y+state.offset(),x=r.x;
  BODY(mockup::stageEnabled[stage],x+712,y+13,82,30,Toggle,"ON");
  if(stage==0){const ParamID ids[]={kSize,kDensity,kPosition,kChaos,kPitch,kGrainMix};const char* labels[]={"SIZE","DENSITY","LOOKBACK","CHAOS","PITCH","MIX"};for(int i=0;i<6;++i)BODY(ids[i],x+25+i*155,y+65,130,120,Knob,labels[i]);
   BODY(kMixLock0,x+805,y+188,110,23,Toggle,"LOCK");
   BODY(kDensityFlow,x+25,y+188,161,30,Toggle,"DENSITY FLOW");BODY(kFreeze,x+197,y+188,105,30,Toggle,"FREEZE");BODY(kGrainBuffer,x+313,y+188,148,30,Select,"BUFFER");BODY(kPanMode,x+472,y+188,172,30,Select,"PAN MODE");if(value(kPanMode)<.25)BODY(kGrainPan,x+657,y+186,135,35,Slider,"PAN");
  }else if(stage==1){
   BODY(kGlitchDivision,x+27,y+65,225,32,Select,"SLICE");BODY(kGlitchTriggerRate,x+275,y+65,292,32,Select,"TRIGGER EVERY");BODY(kGlitchReverse,x+598,y+65,160,32,Toggle,"REVERSE");
   BODY(kGlitchChance,x+60,y+117,135,112,Knob,"CHANCE");BODY(kGlitchMix,x+305,y+117,135,112,Knob,"MIX");BODY(kMixLock0+1,x+449,y+158,91,24,Toggle,"LOCK");BODY(kGlitchVariation,x+585,y+145,340,63,Slider,"VARIATION");
  }else if(stage==2){
   BODY(kRepeatDivision,x+25,y+67,165,32,Select,"LENGTH");BODY(kRepeatOn,x+204,y+67,105,32,Toggle,"HOLD");BODY(kRepeatSeq,x+323,y+67,118,32,Toggle,"GRID ON");BODY(kRepeatAuto,x+456,y+67,97,32,Toggle,"AUTO");BODY(kRepeatInterval,x+570,y+67,370,32,Select,"INTERVAL");
   BODY(kRepeatMix,x+25,y+112,128,105,Knob,"MIX");BODY(kMixLock0+2,x+164,y+158,90,24,Toggle,"LOCK");BODY(kRepeatDuration,x+285,y+137,300,64,Slider,"DURATION");BODY(kRepeatChance,x+623,y+137,300,64,Slider,"CHANCE");
   BODY(kRepeatStep0+selectedRepeat,x+26,y+304,139,31,Toggle,"STEP ON");BODY(kRepeatRate0+selectedRepeat,x+187,y+304,233,31,Select,"DIVISION");BODY(kRepeatPitch0+selectedRepeat,x+456,y+299,240,37,Slider,"STEP PITCH");
  }else if(stage==3){
   BODY(kResliceLength,x+27,y+69,214,51,Slider,"WINDOW");BODY(kResliceMix,x+267,y+69,170,51,Slider,"MIX");BODY(kMixLock0+3,x+451,y+87,60,24,Toggle,"LOCK");BODY(kResliceRndOn,x+519,y+81,166,32,Toggle,"STEP RND");BODY(kResliceRndRate,x+714,y+81,225,32,Select,"RND RATE");BODY(kResliceIndex0+selectedReslice,x+27,y+235,344,31,Select,"SOURCE SLICE");
  }else{
   BODY(kGaterGrid,x+27,y+69,175,32,Select,"RATE");BODY(kGaterLengthRnd,x+227,y+69,183,32,Toggle,"LENGTH RND");BODY(kGaterStepRnd,x+436,y+69,180,32,Toggle,"STEP RND");BODY(kGaterLatch,x+643,y+69,150,32,Toggle,"LATCH");
   BODY(kGaterChance,x+27,y+125,211,62,Slider,"CHANCE");BODY(kGaterMinLength,x+260,y+125,211,62,Slider,"MIN LENGTH");BODY(kGaterLength0+selectedGate,x+493,y+125,211,62,Slider,"STEP LENGTH");BODY(kGaterSustain0+selectedGate,x+726,y+125,211,62,Slider,"SUSTAIN");
  }
 }
}else if(state.page==1){
 BODY(lfoID(selectedLfo,lEnabled),400,223,92,34,Toggle,"ON");
 BODY(lfoID(selectedLfo,lWave),78,262,285,29,Select,"");BODY(lfoID(selectedLfo,lSync),391,270,105,30,Toggle,"SYNC");BODY(kRandomSteps0+selectedLfo,514,270,222,30,Select,"POINTS");BODY(lfoID(selectedLfo,lGrid),752,270,249,30,Select,"LENGTH");
 BODY(lfoID(selectedLfo,lReset),78,563,285,29,Toggle,"RETRIGGER");BODY(kLfoSpeed0+selectedLfo,78,613,127,29,Select,"SPEED");BODY(kModWaveRate0+selectedLfo,217,613,146,29,Select,"MOD W.");
 BODY(lfoID(selectedLfo,lHz),391,322,279,61,Slider,"RATE");BODY(lfoID(selectedLfo,lDepth),718,322,283,61,Slider,"DEPTH");BODY(lfoID(selectedLfo,lPhase),391,391,279,61,Slider,"PHASE");BODY(lfoID(selectedLfo,lGlide),718,391,283,61,Slider,"GLIDE");
 for(int i=0;i<6;++i){int col=i%2,row=i/2;BODY(slotTarget(selectedLfo,i),391+col*327,472+row*64,239,27,Select,"DEST");BODY(slotPolarity(selectedLfo,i),641+col*327,472+row*64,34,27,Select,"");BODY(slotAmount(selectedLfo,i),391+col*327,504+row*64,283,21,Slider,"");}
 BODY(kMasterFilter,422,715,85,30,Toggle,"ON");BODY(kFilterCutoff,79,758,129,127,Knob,"CUTOFF");BODY(kFilterResonance,227,758,129,127,Knob,"RESONANCE");BODY(kFilterDrive,375,758,129,127,Knob,"DRIVE");BODY(kFilterType,79,916,129,29,Select,"TYPE");BODY(kFilterSlope,227,916,129,29,Select,"SLOPE");BODY(kFilterModel,375,916,129,29,Select,"ALG");
 BODY(kFilterSeqOn,918,715,85,30,Toggle,"ON");BODY(kFilterSeqMode,574,761,125,29,Select,"MODE");if(value(kFilterSeqMode)<.5)BODY(kFilterSeqPattern,711,761,144,29,Select,"PATTERN");BODY(kFilterSeqRate,866,761,137,29,Select,"RATE");BODY(kFilterSeqDepth,574,800,199,29,Slider,"DEPTH");BODY(kFilterSeqGlide,798,800,203,29,Slider,"GLIDE");int comb=int(std::round(value(kFilterModel)*18));if(comb==7||comb==8){BODY(kCombRoot,574,960,112,23,Select,"ROOT");BODY(kCombOctave,699,960,127,23,Select,"OCTAVE");BODY(kCombScale,838,960,165,23,Select,"NOTES");}
 BODY(kReverbOn,916,1024,86,31,Toggle,"ON");BODY(kReverbKill,763,1024,139,31,Toggle,"KILL DRY");BODY(kReverbModel,78,1069,210,30,Select,"TYPE");if(value(kReverbModel)<.125)BODY(kReverbType,304,1069,202,30,Select,"MODE");BODY(kReverbSource,521,1069,287,30,Select,"SOURCE");BODY(kReverbRateV2,824,1069,178,30,Select,"RATE");BODY(kReverbMix,86,1139,80,151,VSlider,"AMOUNT");BODY(kReverbLength,195,1139,80,151,VSlider,"LENGTH");BODY(kMixLock0+4,87,1111,80,21,Toggle,"LOCK");if(value(kReverbSource)<.5)for(int i=0;i<16;++i)BODY(kReverbStep0+i,300+i*43,1118,26,138,Pad,"");
 BODY(kMix,490,1378,125,123,Knob,"MIX");BODY(kMixLock0+5,501,1504,95,21,Toggle,"LOCK");BODY(kNormalize,650,1390,174,30,Toggle,"NORMALIZE");BODY(kMasterLimiter,650,1440,174,30,Toggle,"LIMITER");BODY(kLimiterCeiling,844,1390,157,30,Select,"LIMIT");
}else{
 BODY(kXYEnable,916,223,86,34,Toggle,"ON");BODY(kXTarget,79,955,445,30,Select,"X DEST");BODY(kYTarget,558,955,445,30,Select,"Y DEST");BODY(kXAmount,79,999,445,28,Slider,"");BODY(kYAmount,558,999,445,28,Slider,"");
}
#undef BODY
