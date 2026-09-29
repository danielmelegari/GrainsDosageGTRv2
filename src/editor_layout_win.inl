// Shared control geometry from the Cocoa editor.
  double x=slotX[slot(0)];
  const ParamID granular[]={kSize,kDensity,kPitch,kPosition,kChaos,kGrainMix,kStretchSpeed,kTranspose};
  const char* grainNames[]={"SIZE","DENSITY","PITCH","LOOKBACK","CHAOS","MIX","SPEED","TRANSPOSE"};
  for(int i=0;i<8;++i)ADD(granular[i],x+14+(i%4)*99,i<4?151:(i<6?257:274),90,92,Knob,grainNames[i]);
  ADD(kDensityFlow,x+14,368,184,25,Toggle,"DENSITY FLOW");
  ADD(kStretchOn,x+218,369,180,24,Toggle,"FREE STRETCH");

  ADD(kPanMode,x+14,398,144,30,Select,"PAN MODE");
  if(value(kPanMode)<.25)ADD(kGrainPan,x+170,396,228,30,Pan,"POSITION");
  x=slotX[slot(1)];
  ADD(kGlitchDivision,x+18,151,132,46,Select,"SLICE");ADD(kGlitchReverse,x+18,210,132,28,Toggle,"REVERSE");
  ADD(kGlitchChance,x+164,151,112,92,Knob,"CHANCE");ADD(kGlitchMix,x+285,151,112,92,Knob,"MIX");
  ADD(kGlitchTriggerRate,x+18,294,240,46,Select,"TRIGGER EVERY");
  x=slotX[slot(2)];
  ADD(kRepeatDivision,x+18,151,130,46,Select,"LENGTH");ADD(kRepeatOn,x+18,210,130,28,Toggle,"HOLD");
  ADD(kRepeatMix,x+165,151,112,92,Knob,"MIX");ADD(kRepeatSeq,x+282,156,115,28,Toggle,"GRID ON");
  ADD(kRepeatStep0+selectedRepeat,x+18,354,99,34,Toggle,"STEP ON");
  ADD(kRepeatRate0+selectedRepeat,x+126,350,127,44,Select,"STEP DIVISION");
  ADD(kRepeatPitch0+selectedRepeat,x+268,353,130,42,Slider,"STEP PITCH");
  x=slotX[slot(1)];
  ADD(kGlitchMove,x+18,415,116,40,Slider,"MOVE");ADD(kGlitchVariation,x+150,415,116,40,Slider,"VARIATION");
  // Random trigger interval replaces the previous refresh/grid controls.
  x=slotX[slot(2)];
  ADD(kRepeatAuto,x+282,199,115,28,Toggle,"AUTO");
  ADD(kRepeatInterval,x+18,415,116,42,Select,"INTERVAL");ADD(kRepeatDuration,x+150,415,116,40,Slider,"DURATION");
  ADD(kRepeatChance,x+282,415,115,40,Slider,"CHANCE");
  ADD(lfoID(selectedLfo,lEnabled),710,529,118,27,Toggle,"ENABLE");
  ADD(lfoID(selectedLfo,lWave),32,570,220,34,Select,"WAVE");
  ADD(lfoID(selectedLfo,lSync),264,574,68,27,Toggle,"SYNC");ADD(lfoID(selectedLfo,lReset),344,574,108,27,Toggle,"RETRIGGER");
  ADD(kRandomSteps0+selectedLfo,464,570,130,34,Select,"POINTS");ADD(lfoID(selectedLfo,lGrid),606,570,222,34,Select,"SYNC RATE");
  ADD(lfoID(selectedLfo,lHz),278,610,124,40,Slider,"FREE RATE");ADD(lfoID(selectedLfo,lDepth),414,610,124,40,Slider,"DEPTH");
  ADD(lfoID(selectedLfo,lPhase),550,610,124,40,Slider,"PHASE");ADD(lfoID(selectedLfo,lGlide),686,610,124,40,Slider,"GLIDE");
  ADD(kLfoSpeed0+selectedLfo,32,690,104,40,Select,"SPEED");
  ADD(kModWaveRnd0+selectedLfo,144,690,108,40,Select,"MOD WAVE RND");
  for(int i=0;i<6;++i){ADD(slotTarget(selectedLfo,i),268+(i%3)*190,662+(i/3)*44,112,40,Select,"DESTINATION");ADD(slotAmount(selectedLfo,i),384+(i%3)*190,662+(i/3)*44,60,40,Slider,"AMT");}
  ADD(kXYEnable,1325,529,101,27,Toggle,"ENABLE");
  ADD(kXTarget,1062,565,224,38,Select,"X DESTINATION");ADD(kXAmount,1062,608,224,40,Slider,"X AMOUNT");
  ADD(kYTarget,1062,653,224,38,Select,"Y DESTINATION");ADD(kYAmount,1062,697,224,40,Slider,"Y AMOUNT");
  ADD(kMasterFilter,142,1016,110,24,Toggle,"FILTER ON");
  ADD(kFilterType,32,1046,142,38,Select,"TYPE");ADD(kFilterSlope,32,1092,142,38,Select,"SLOPE");
  ADD(kFilterCutoff,190,1046,108,92,Knob,"CUTOFF");ADD(kFilterResonance,310,1046,108,92,Knob,"RESONANCE");
  ADD(kFilterDrive,430,1046,108,92,Knob,"DRIVE");
  ADD(kReverbOn,690,1016,110,24,Toggle,"REVERB ON");ADD(kReverbKill,818,1016,120,24,Toggle,"KILL DRY");
  ADD(kReverbModel,584,1046,146,24,Select,"MODEL");if(value(kReverbModel)<.125)ADD(kReverbType,584,1074,146,24,Select,"TYPE");if(value(kReverbSource)<.5){ADD(kReverbGrid,742,1050,114,38,Select,"STEP RATE");}else{ADD(kReverbRandomRate,742,1050,114,38,Select,"RANDOM RATE");}
  ADD(kReverbMix,868,1050,148,40,Slider,"AMOUNT");ADD(kReverbLength,1030,1050,246,40,Slider,"LENGTH");
  ADD(kReverbSource,946,1016,330,24,Select,"TRIGGER SOURCE");
  if(value(kReverbSource)<.5)for(int i=0;i<16;++i)ADD(kReverbStep0+i,584+i*44,1100,39,30,Pad,"");
  ADD(kMix,116,1302,174,40,Slider,"DRY / WET");ADD(kNormalize,306,1312,116,26,Toggle,"NORMALIZE");
  ADD(kMasterLimiter,438,1312,102,26,Toggle,"LIMITER");ADD(kLimiterCeiling,548,1312,80,26,Select,"CEILING");ADD(kBypass,636,1312,90,26,Toggle,"BYPASS");
  ADD(kModuleOrder,734,1302,244,40,Select,"AUDIO ORDER");
  for(int i=0;i<3;++i)ADD(kGrainEnabled+i,slotX[slot(i)]+238,106,68,28,Toggle,"ON");

  ADD(kGaterEnabled,148,898,72,27,Toggle,"ON");
  ADD(kGaterGrid,232,893,100,40,Select,"GATE RATE");
  ADD(kGaterLengthRnd,346,898,126,27,Toggle,"LENGTH RND");
  ADD(kGaterStepRnd,486,898,118,27,Toggle,"STEP RND");
  ADD(kGaterChance,620,893,126,40,Slider,"CHANCE");
  ADD(kGaterMinLength,758,893,166,40,Slider,"MIN LENGTH");
  ADD(kGaterLength0+selectedGate,932,893,96,40,Slider,"STEP LENGTH");
  ADD(kGaterSustain0+selectedGate,1034,893,122,40,Slider,"SUSTAIN");
  ADD(kGaterTie,1166,898,62,27,Toggle,"TIE");
  ADD(kGaterLatch,1234,898,70,27,Toggle,"LATCH");

  ADD(kInputDeclick,420,55,166,24,Toggle,"INPUT DE-CLICK");
  ADD(kDeclickSensitivity,600,53,282,28,Slider,"SENSITIVITY");

  ADD(kResliceEnabled,148,778,72,27,Toggle,"ON");
  ADD(kResliceLength,232,773,132,40,Select,"WINDOW");
  ADD(kResliceMix,384,773,156,40,Slider,"MIX");
  ADD(kResliceIndex0+selectedReslice,560,773,182,40,Select,"SOURCE SLICE");
  ADD(kResliceRndOn,754,778,126,27,Toggle,"STEP RND");
  ADD(kResliceRndRate,892,773,142,40,Select,"RND RATE");
  ADD(kFilterModel,266,1016,270,24,Select,"MODEL");

  ADD(kFilterSeqOn,220,1170,100,28,Toggle,"ON");
  ADD(kFilterSeqMode,334,1163,174,40,Select,"MODE");
  if(value(kFilterSeqMode)<.5)ADD(kFilterSeqPattern,520,1163,230,40,Select,"PATTERN");
  ADD(kFilterSeqRate,766,1163,122,40,Select,"RATE");
  ADD(kFilterSeqDepth,904,1163,174,40,Slider,"DEPTH");
  ADD(kFilterSeqGlide,1094,1163,184,40,Slider,"GLIDE");
  const int combModel=int(std::round(value(kFilterModel)*18.));
  if(combModel==7||combModel==8){
    ADD(kCombRoot,766,1221,122,40,Select,"COMB ROOT");
    ADD(kCombOctave,904,1221,122,40,Select,"OCTAVE");
    ADD(kCombScale,1042,1221,236,40,Select,"NOTES");
  }
