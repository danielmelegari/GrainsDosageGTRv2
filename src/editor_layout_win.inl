// Shared geometry in the approved artwork's 1083 x 1452 coordinate system.
const int tab=currentTab;
ADD(kInputDeclick,835,65,55,23,Toggle,"ON");
ADD(kDeclickSensitivity,895,62,108,27,Slider,"");
const ParamID stageEnabled[]={kGrainEnabled,kGlitchEnabled,kRepeatEnabled,kResliceEnabled,kGaterEnabled};
ADD(stageEnabled[tab],759,451,75,32,Toggle,"ON");
ADD(kTranspose,876,194,139,137,Knob,"TRANSPOSE");
ADD(kStretchSpeed,876,337,139,129,Knob,"STRETCH");
ADD(kStretchOn,876,472,139,27,Toggle,"FREE STRETCH");
if(tab==0){
 const ParamID ids[]={kSize,kDensity,kPosition,kChaos,kPitch,kGrainMix};
 const char* labels[]={"SIZE","DENSITY","LOOKBACK","CHAOS","PITCH","MIX"};
 for(int i=0;i<6;++i)ADD(ids[i],75+i*127,211,119,123,Knob,labels[i]);
 ADD(kDensityFlow,76,357,166,29,Toggle,"DENSITY FLOW");
 ADD(kFreeze,252,357,100,29,Toggle,"FREEZE");
 ADD(kGrainBuffer,364,357,134,29,Select,"BUFFER");
 ADD(kPanMode,512,357,145,29,Select,"PAN MODE");
 if(value(kPanMode)<.25)ADD(kGrainPan,676,353,157,37,Slider,"PAN");
}
if(tab==1){
 ADD(kGlitchDivision,77,218,179,30,Select,"SLICE");
 ADD(kGlitchReverse,77,266,179,30,Toggle,"REVERSE");
 ADD(kGlitchTriggerRate,77,315,179,30,Select,"TRIGGER");
 ADD(kGlitchChance,292,216,124,127,Knob,"CHANCE");
 ADD(kGlitchMix,454,216,124,127,Knob,"MIX");
 ADD(kGlitchVariation,634,246,181,62,Slider,"VARIATION");
}
if(tab==2){
 ADD(kRepeatDivision,76,217,141,29,Select,"LENGTH");
 ADD(kRepeatOn,76,261,141,29,Toggle,"HOLD");
 ADD(kRepeatMix,238,210,115,117,Knob,"MIX");
 ADD(kRepeatSeq,378,217,124,29,Toggle,"GRID ON");
 ADD(kRepeatAuto,378,261,124,29,Toggle,"AUTO");
 ADD(kRepeatInterval,526,217,286,29,Select,"INTERVAL");
 ADD(kRepeatDuration,526,265,132,57,Slider,"DURATION");
 ADD(kRepeatChance,680,265,132,57,Slider,"CHANCE");
 ADD(kRepeatStep0+selectedRepeat,76,447,116,30,Toggle,"STEP ON");
 ADD(kRepeatRate0+selectedRepeat,205,447,184,30,Select,"DIVISION");
 ADD(kRepeatPitch0+selectedRepeat,405,430,193,57,Slider,"STEP PITCH");
}
if(tab==3){
 ADD(kResliceLength,76,223,152,60,Slider,"WINDOW");
 ADD(kResliceMix,258,223,151,60,Slider,"MIX");
 ADD(kResliceRndOn,439,233,151,30,Toggle,"STEP RND");
 ADD(kResliceRndRate,619,233,207,30,Select,"RND RATE");
 ADD(kResliceIndex0+selectedReslice,76,449,212,30,Select,"SOURCE SLICE");
}
if(tab==4){
 ADD(kGaterGrid,76,212,151,32,Select,"RATE");
 ADD(kGaterLengthRnd,244,212,145,32,Toggle,"LENGTH RND");
 ADD(kGaterStepRnd,400,212,151,32,Toggle,"STEP RND");
 ADD(kGaterLatch,563,212,138,32,Toggle,"LATCH");
 ADD(kGaterChance,76,286,151,65,Slider,"CHANCE");
 ADD(kGaterMinLength,258,286,150,65,Slider,"MIN LENGTH");
 ADD(kGaterLength0+selectedGate,439,286,151,65,Slider,"STEP LENGTH");
 ADD(kGaterSustain0+selectedGate,619,286,149,65,Slider,"SUSTAIN");
}
ADD(lfoID(selectedLfo,lEnabled),348,533,94,35,Toggle,"ON");
ADD(lfoID(selectedLfo,lWave),77,570,249,25,Select,"");
ADD(lfoID(selectedLfo,lSync),350,576,95,28,Toggle,"SYNC");
ADD(kRandomSteps0+selectedLfo,451,576,126,28,Select,"POINTS");
ADD(lfoID(selectedLfo,lGrid),588,576,124,28,Select,"LENGTH");
ADD(lfoID(selectedLfo,lReset),77,796,249,26,Toggle,"RETRIGGER");
ADD(lfoID(selectedLfo,lHz),350,617,74,62,Slider,"RATE");
ADD(lfoID(selectedLfo,lDepth),446,617,74,62,Slider,"DEPTH");
ADD(lfoID(selectedLfo,lPhase),542,617,74,62,Slider,"PHASE");
ADD(lfoID(selectedLfo,lGlide),638,617,74,62,Slider,"GLIDE");
ADD(kLfoSpeed0+selectedLfo,77,830,116,26,Select,"SPEED");
ADD(kModWaveRnd0+selectedLfo,205,830,121,26,Select,"MOD W.");
for(int i=0;i<6;++i){
 ADD(slotTarget(selectedLfo,i),349+(i%2)*187,688+(i/2)*58,138,26,Select,"DEST");
 ADD(slotPolarity(selectedLfo,i),491+(i%2)*187,688+(i/2)*58,31,26,Select,"");
 ADD(slotAmount(selectedLfo,i),349+(i%2)*187,718+(i/2)*58,173,22,Slider,"");
}
ADD(kXYEnable,932,533,75,35,Toggle,"ON");
ADD(kXTarget,757,810,120,28,Select,"DEST");
ADD(kXAmount,757,847,120,28,Slider,"");
ADD(kYTarget,889,810,120,28,Select,"DEST");
ADD(kYAmount,889,847,120,28,Slider,"");
ADD(kMasterFilter,182,906,79,29,Toggle,"ON");
ADD(kFilterCutoff,79,951,120,136,Knob,"CUTOFF");
ADD(kFilterResonance,217,951,120,136,Knob,"RESONANCE");
ADD(kFilterDrive,355,951,120,136,Knob,"DRIVE");
ADD(kFilterType,78,1105,119,25,Select,"TYPE");
ADD(kFilterSlope,208,1105,119,25,Select,"SLOPE");
ADD(kFilterModel,338,1105,138,25,Select,"ALG");
ADD(kFilterSeqOn,771,906,79,29,Toggle,"ON");
ADD(kFilterSeqMode,521,951,145,27,Select,"MODE");
if(value(kFilterSeqMode)<.5)ADD(kFilterSeqPattern,676,951,150,27,Select,"PATTERN");
ADD(kFilterSeqRate,840,951,169,27,Select,"RATE");
ADD(kFilterSeqDepth,523,984,220,29,Slider,"DEPTH");
ADD(kFilterSeqGlide,771,984,237,29,Slider,"GLIDE");
const int combModel=int(std::round(value(kFilterModel)*18.));
if(combModel==7||combModel==8){
 ADD(kCombRoot,521,1115,122,22,Select,"ROOT");
 ADD(kCombOctave,651,1115,122,22,Select,"OCTAVE");
 ADD(kCombScale,781,1115,227,22,Select,"NOTES");
}
ADD(kReverbOn,190,1162,78,29,Toggle,"ON");
ADD(kReverbModel,76,1201,127,28,Select,"TYPE");
if(value(kReverbModel)<.125)ADD(kReverbType,213,1201,125,28,Select,"MODE");
ADD(kReverbSource,348,1201,169,28,Select,"SOURCE");
ADD(kReverbRateV2,527,1201,112,28,Select,"RATE");
ADD(kReverbKill,318,1162,112,29,Toggle,"KILL DRY");
ADD(kReverbMix,76,1254,74,148,VSlider,"AMOUNT");
ADD(kReverbLength,166,1254,74,148,VSlider,"LENGTH");
if(value(kReverbSource)<.5)for(int i=0;i<16;++i)ADD(kReverbStep0+i,255+i*24,1254,16,121,Pad,"");
ADD(kMix,683,1302,72,97,Knob,"MIX");
ADD(kNormalize,765,1321,110,27,Toggle,"NORMALIZE");
ADD(kMasterLimiter,765,1357,110,27,Toggle,"LIMITER");
ADD(kLimiterCeiling,884,1308,127,27,Select,"LIMIT");
// Locks only protect randomisation; manual editing and automation remain available.
if(tab==0)ADD(kMixLock0,754,334,72,19,Toggle,"LOCK");
if(tab==1)ADD(kMixLock0+1,480,346,72,21,Toggle,"LOCK");
if(tab==2)ADD(kMixLock0+2,259,330,72,21,Toggle,"LOCK");
if(tab==3)ADD(kMixLock0+3,336,292,72,21,Toggle,"LOCK");
ADD(kMixLock0+4,77,1231,71,21,Toggle,"LOCK");
ADD(kMixLock0+5,685,1400,69,19,Toggle,"LOCK");
