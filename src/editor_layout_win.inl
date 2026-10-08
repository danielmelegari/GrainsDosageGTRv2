// Shared mockup geometry. ADD is supplied by the native editor or layout test.
// Only the selected processor registers controls; DSP order and IDs are unchanged.
const int tab=currentTab;
ADD(kInputDeclick,1320,52,64,30,Toggle,"ON");
ADD(kDeclickSensitivity,1394,49,204,33,Slider,"");
const ParamID stageEnabled[]={kGrainEnabled,kGlitchEnabled,kRepeatEnabled,kResliceEnabled,kGaterEnabled};
ADD(stageEnabled[tab],1230,421,125,42,Toggle,"ON");
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
 ADD(kRepeatStep0+selectedRepeat,50,590,210,32,Toggle,"STEP ON");
 ADD(kRepeatRate0+selectedRepeat,292,590,340,32,Select,"STEP DIVISION");
 ADD(kRepeatPitch0+selectedRepeat,670,578,300,58,Slider,"STEP PITCH");
}
if(tab==3){
 ADD(kResliceLength,55,240,240,68,Slider,"WINDOW");
 ADD(kResliceMix,330,240,220,68,Slider,"MIX");
 ADD(kResliceRndOn,598,250,210,46,Toggle,"STEP RND");
 ADD(kResliceRndRate,850,250,340,46,Select,"RND RATE");
 ADD(kResliceIndex0+selectedReslice,55,590,370,32,Select,"SOURCE SLICE");
}
if(tab==4){
 ADD(kGaterGrid,50,230,240,42,Select,"GATE RATE");
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
ADD(kMasterFilter,154,1143,110,30,Toggle,"ON");
ADD(kFilterType,60,1183,204,31,Select,"TYPE");
ADD(kFilterSlope,60,1223,204,31,Select,"SLOPE");
ADD(kFilterModel,60,1263,204,31,Select,"ALGOR");
ADD(kFilterCutoff,318,1145,106,151,Knob,"CUTOFF");
ADD(kFilterResonance,458,1145,106,151,Knob,"RESONANCE");
ADD(kFilterDrive,598,1145,106,151,Knob,"DRIVE");
ADD(kFilterSeqOn,1023,1140,94,32,Toggle,"ON");
ADD(kFilterSeqMode,792,1183,202,31,Select,"MODE");
if(value(kFilterSeqMode)<.5)ADD(kFilterSeqPattern,792,1223,202,31,Select,"PATTERN");
ADD(kFilterSeqRate,792,1263,202,31,Select,"RATE");
ADD(kFilterSeqDepth,1144,1148,210,46,Slider,"DEPTH");
ADD(kFilterSeqGlide,1373,1148,210,46,Slider,"GLIDE");
const int combModel=int(std::round(value(kFilterModel)*18.));
if(combModel==7||combModel==8){
 ADD(kCombRoot,1020,1295,155,24,Select,"ROOT");
 ADD(kCombOctave,1184,1295,155,24,Select,"OCTAVE");
 ADD(kCombScale,1348,1295,230,24,Select,"NOTES");
}
ADD(kReverbOn,154,1340,110,30,Toggle,"ON");
ADD(kReverbModel,276,1340,202,30,Select,"TYPE");
if(value(kReverbModel)<.125)ADD(kReverbType,40,1393,218,30,Select,"MODE");
ADD(kReverbKill,279,1393,180,30,Toggle,"KILL DRY");
ADD(kReverbSource,498,1340,272,30,Select,"SOURCE");
if(value(kReverbSource)<.5)ADD(kReverbGrid,795,1340,204,30,Select,"RATE");
else ADD(kReverbRandomRate,795,1340,204,30,Select,"RATE");
ADD(kReverbMix,543,1386,202,45,Slider,"AMOUNT");
ADD(kReverbLength,786,1386,211,45,Slider,"LENGTH");
if(value(kReverbSource)<.5)for(int i=0;i<16;++i)ADD(kReverbStep0+i,1022+i*36,1359,32,49,Pad,"");
ADD(kMix,196,1461,204,40,Slider,"MIX");
ADD(kNormalize,420,1466,184,31,Toggle,"NORMALIZE");
ADD(kMasterLimiter,619,1466,182,31,Toggle,"LIMITER");
ADD(kLimiterCeiling,818,1466,204,31,Select,"LIMIT");
