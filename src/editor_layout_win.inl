// Shared control geometry from the Cocoa editor.
  double x=slotX[slot(0)];
  // The granular knob grid: SPEED used to sit here as a second, unlabeled
  // kStretchSpeed knob. It moved into the MASTER OPTIONS area as the
  // "STRETCH %" slider below, so the grid now has seven knobs and the
  // y=274 strip holds only TRANSPOSE plus that free-stretch control.
  const ParamID granular[]={kSize,kDensity,kPitch,kPosition,kChaos,kGrainMix,kTranspose};
  const char* grainNames[]={"SIZE","DENSITY","PITCH","LOOKBACK","CHAOS","MIX","TRANSPOSE"};
  for(int i=0;i<7;++i)ADD(granular[i],x+14+(i%4)*99,i<4?151:274,90,92,Knob,grainNames[i]);
  ADD(kDensityFlow,x+14,368,184,25,Toggle,"DENSITY FLOW");

  // STRETCH SPEED / BUFFER SIZE / FREEZE / RANDOM ALL live inside the
  // GRANULIZER "MASTER OPTIONS" box (drawn at x+208,248 size 196x147 by both
  // editors; it ends at y=395). They were previously registered as VST
  // parameters but had no hit area, so nothing appeared on screen. The old
  // FREE STRETCH row was reclaimed for these controls — the whole stack now
  // starts below the knob-grid readouts (last row ends at y=366) and stays
  // clear of DENSITY FLOW and PAN MODE.
  ADD(kStretchOn,x+218,368,180,18,Toggle,"FREE STRETCH");
  // STRETCH SPEED (the free-stretch rate, % readout) lives here too: the
  // granular grid used to carry a second, unlabeled kStretchSpeed knob that
  // collided with the LFO SPEED selector in the layout test. The knob is
  // gone; this slider owns the parameter now. It sits below the MASTER
  // OPTIONS box (ends y=395) because the PAN MODE / POSITION row spans
  // x+14..x+398 at y=396..428 and nothing may overlap it.
  ADD(kStretchSpeed,x+218,432,180,18,Slider,"STRETCH %");
  ADD(kGrainBuffer,x+218,452,176,24,Select,"BUFFER SIZE");
  ADD(kFreeze,x+218,478,84,22,Toggle,"FREEZE");
  ADD(kRandomAll,x+310,478,84,22,Toggle,"RANDOM");

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
  ADD(kGlitchVariation,x+18,415,116,40,Slider,"VARIATION");
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
  ADD(kXYEnable,1185,529,101,27,Toggle,"ENABLE");
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
  ADD(kMasterLimiter,438,1312,102,26,Toggle,"LIMITER");ADD(kLimiterCeiling,548,1312,80,26,Select,"CEILING");
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
  // TIE removed from the Gater (LATCH kept); LATCH moved into the freed slot.
  ADD(kGaterLatch,1166,898,70,27,Toggle,"LATCH");

  // INPUT DE-CLICK / SENSITIVITY are drawn in the header (row 2, next to LOAD/SAVE);
  // they are registered here for hit-testing and value display only.
  ADD(kInputDeclick,684,56,56,28,Toggle,"");
  ADD(kDeclickSensitivity,548,56,130,28,Slider,"");

  ADD(kResliceEnabled,148,778,72,27,Toggle,"ON");
  // PRESLICER knobs: WINDOW and MIX are real knobs now (bigger than the old
  // compact select/slider row), so they use the standard 92px knob strip.
  // WINDOW is a stepped knob over the four lengths; MIX stays continuous.
  ADD(kResliceLength,384,773,112,92,Knob,"WINDOW");
  ADD(kResliceMix,504,773,112,92,Knob,"MIX");
  ADD(kResliceRndOn,624,778,126,27,Toggle,"STEP RND");
  ADD(kResliceRndRate,758,773,142,40,Select,"RND RATE");
  ADD(kResliceIndex0+selectedReslice,908,773,132,40,Select,"SOURCE SLICE");
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
