// Shared mockup geometry. ADD is supplied by the native editor or layout test.
// Only the selected processor registers controls; DSP order and IDs are unchanged.
const int tab=currentTab;
ADD(kInputDeclick,1320,52,64,30,Toggle,"ON");
ADD(kDeclickSensitivity,1394,49,204,33,Slider,"");
const ParamID stageEnabled[]={kGrainEnabled,kGlitchEnabled,kRepeatEnabled,kResliceEnabled,kGaterEnabled};
ADD(stageEnabled[tab],1230,(tab==3?576:tab==2?389:421),125,42,Toggle,"ON");
ADD(kTranspose,1420,195,180,182,Knob,"TRANSPOSE");
ADD(kStretchSpeed,1420,386,180,182,Knob,"STRETCH");
ADD(kStretchOn,1394,584,210,44,Toggle,"FREE STRETCH");
if(tab==0){
 const ParamID ids[]={kSize,kDensity,kPosition,kChaos,kPitch,kGrainMix};
 const char* labels[]={"SIZE","DENSITY","LOOKBACK","CHAOS","PITCH","MIX"};
 for(int i=0;i<6;++i)ADD(ids[i],68+i*194,201,180,182,Knob,labels[i]);
 ADD(kDensityFlow,39,421,297,42,Toggle,"DENSITY FLOW");
 ADD(kFreeze,359,421,170,42,Toggle,"FREEZE");
 ADD(kGrainBuffer,548,421,198,42,Select,"BUFFER");
 ADD(kPanMode,40,475,220,28,Select,"PAN MODE");
 if(value(kPanMode)<.25)ADD(kGrainPan,284,468,255,38,Slider,"PAN");
}
if(tab==1){
 ADD(kGlitchDivision,58,230,280,44,Select,"SLICE");
 ADD(kGlitchReverse,58,302,280,44,Toggle,"REVERSE");
 ADD(kGlitchTriggerRate,58,378,280,44,Select,"TRIGGER EVERY");
 ADD(kGlitchChance,420,201,180,182,Knob,"CHANCE");
 ADD(kGlitchMix,664,201,180,182,Knob,"MIX");
 ADD(kGlitchVariation,902,270,302,66,Slider,"VARIATION");
}
if(tab==2){
 ADD(kRepeatDivision,50,226,246,42,Select,"LENGTH");
 ADD(kRepeatOn,50,290,246,44,Toggle,"HOLD");
 ADD(kRepeatMix,340,201,180,182,Knob,"MIX");
 ADD(kRepeatSeq,560,226,220,44,Toggle,"GRID ON");
 ADD(kRepeatAuto,560,290,220,44,Toggle,"AUTO");
 ADD(kRepeatInterval,828,226,270,44,Select,"INTERVAL");
 ADD(kRepeatDuration,828,298,170,66,Slider,"DURATION");
 ADD(kRepeatChance,1020,298,170,66,Slider,"CHANCE");
 ADD(kRepeatStep0+selectedRepeat,50,580,210,38,Toggle,"STEP ON");
 ADD(kRepeatRate0+selectedRepeat,292,580,340,38,Select,"DIVISION");
 ADD(kRepeatPitch0+selectedRepeat,670,568,300,66,Slider,"STEP PITCH");
}
if(tab==3){
 ADD(kResliceLength,55,215,250,76,Slider,"WINDOW");
 ADD(kResliceMix,355,215,230,76,Slider,"MIX");
 ADD(kResliceRndOn,635,225,230,46,Toggle,"STEP RND");
 ADD(kResliceRndRate,925,225,390,46,Select,"RND RATE");
 ADD(kResliceIndex0+selectedReslice,55,490,370,34,Select,"SOURCE SLICE");
}
if(tab==4){
 ADD(kGaterGrid,50,230,240,42,Select,"RATE");
 ADD(kGaterLengthRnd,320,230,242,42,Toggle,"LENGTH RND");
 ADD(kGaterStepRnd,592,230,220,42,Toggle,"STEP RND");
 ADD(kGaterLatch,848,230,200,42,Toggle,"LATCH");
 ADD(kGaterChance,50,330,240,68,Slider,"CHANCE");
 ADD(kGaterMinLength,340,330,250,68,Slider,"MIN LENGTH");
 ADD(kGaterLength0+selectedGate,640,330,250,68,Slider,"STEP LENGTH");
 ADD(kGaterSustain0+selectedGate,940,330,250,68,Slider,"SUSTAIN");
}
ADD(lfoID(selectedLfo,lEnabled),472,680,142,46,Toggle,"ON");
ADD(lfoID(selectedLfo,lWave),40,731,390,34,Select,"");
ADD(lfoID(selectedLfo,lSync),480,744,140,32,Toggle,"SYNC");
ADD(kRandomSteps0+selectedLfo,638,744,176,32,Select,"POINTS");
ADD(lfoID(selectedLfo,lGrid),832,744,196,32,Select,"LENGTH");
ADD(lfoID(selectedLfo,lReset),40,1031,390,23,Toggle,"RETRIGGER");
ADD(lfoID(selectedLfo,lHz),480,799,115,72,Slider,"RATE");
ADD(lfoID(selectedLfo,lDepth),624,799,115,72,Slider,"DEPTH");
ADD(lfoID(selectedLfo,lPhase),768,799,115,72,Slider,"PHASE");
ADD(lfoID(selectedLfo,lGlide),912,799,115,72,Slider,"GLIDE");
ADD(kLfoSpeed0+selectedLfo,40,1060,190,32,Select,"SPEED");
ADD(kModWaveRnd0+selectedLfo,240,1060,190,32,Select,"MOD W.");
for(int i=0;i<6;++i){
 ADD(slotTarget(selectedLfo,i),480+(i%2)*278,900+(i/2)*64,202,30,Select,"DEST");
 ADD(slotPolarity(selectedLfo,i),690+(i%2)*278,900+(i/2)*64,56,30,Select,"");
 ADD(slotAmount(selectedLfo,i),480+(i%2)*278,934+(i/2)*64,266,25,Slider,"");
}
ADD(kXYEnable,1490,680,110,46,Toggle,"ON");
ADD(kXTarget,1112,1014,220,32,Select,"X DEST");
ADD(kXAmount,1112,1060,228,32,Slider,"");
ADD(kYTarget,1370,1014,220,32,Select,"Y DEST");
ADD(kYAmount,1370,1060,228,32,Slider,"");
ADD(kMasterFilter,192,1148,124,34,Toggle,"ON");
ADD(kFilterCutoff,66,1214,170,182,Knob,"CUTOFF");
ADD(kFilterResonance,294,1214,170,182,Knob,"RESONANCE");
ADD(kFilterDrive,522,1214,170,182,Knob,"DRIVE");
ADD(kFilterType,43,1406,205,32,Select,"TYPE");
ADD(kFilterSlope,261,1406,205,32,Select,"SLOPE");
ADD(kFilterModel,479,1406,259,32,Select,"ALGOR");
ADD(kFilterSeqOn,1109,1148,110,34,Toggle,"ON");
ADD(kFilterSeqMode,792,1210,235,34,Select,"MODE");
if(value(kFilterSeqMode)<.5)ADD(kFilterSeqPattern,792,1263,235,34,Select,"PATTERN");
ADD(kFilterSeqRate,792,1316,235,34,Select,"RATE");
ADD(kFilterSeqDepth,1240,1150,164,46,Slider,"DEPTH");
ADD(kFilterSeqGlide,1428,1150,164,46,Slider,"GLIDE");
const int combModel=int(std::round(value(kFilterModel)*18.));
if(combModel==7||combModel==8){
 ADD(kCombRoot,799,1407,184,27,Select,"ROOT");
 ADD(kCombOctave,995,1407,184,27,Select,"OCTAVE");
 ADD(kCombScale,1191,1407,383,27,Select,"NOTES");
}
ADD(kReverbOn,53,1520,120,32,Toggle,"ON");
ADD(kReverbModel,212,1480,206,32,Select,"TYPE");
if(value(kReverbModel)<.125)ADD(kReverbType,212,1526,206,32,Select,"MODE");
ADD(kReverbSource,438,1480,300,32,Select,"SOURCE");
ADD(kReverbRateV2,754,1480,245,32,Select,"RATE");
ADD(kReverbKill,438,1526,184,32,Toggle,"KILL DRY");
ADD(kReverbMix,51,1590,130,206,VSlider,"AMOUNT");
ADD(kReverbLength,222,1590,130,206,VSlider,"LENGTH");
if(value(kReverbSource)<.5)for(int i=0;i<16;++i)ADD(kReverbStep0+i,405+i*37,1602,28,166,Pad,"");
ADD(kMix,1062,1694,174,112,Slider,"MIX");
ADD(kNormalize,1270,1700,145,34,Toggle,"NORMALIZE");
ADD(kMasterLimiter,1430,1700,151,34,Toggle,"LIMITER");
ADD(kLimiterCeiling,1270,1750,312,34,Select,"LIMIT");
// Locks protect randomisation only; normal editing and automation remain available.
if(tab==0)ADD(kMixLock0,1178,389,110,30,Toggle,"LOCK");
if(tab==1)ADD(kMixLock0+1,700,389,110,30,Toggle,"LOCK");
if(tab==2)ADD(kMixLock0+2,373,389,110,30,Toggle,"LOCK");
if(tab==3)ADD(kMixLock0+3,465,306,110,30,Toggle,"LOCK");
ADD(kMixLock0+4,57,1558,112,26,Toggle,"LOCK");
ADD(kMixLock0+5,1103,1810,110,28,Toggle,"LOCK");
