// Shared control geometry from the Cocoa editor.
  // Tab strip (Option B): kUiTab selects which of the five top-row modules is
  // visible in the shared panel area. Hidden stages register NO controls, so
  // there are no invisible hit areas and no overlap between relocated panels.
  // Stage tabs (0-4) show exactly ONE module panel in the shared slot rect;
  // artefact tabs (5/6) stack GRANULIZER + the ordered RESLICE/GATER pair.
  const int tab=currentTab;
  // Shared panel slot: every module tab draws its ONE visible panel at
  // x=16..1304, y=98..502 (tabSlotRect). The modulation band below starts at
  // y=516 and is always visible.
  double px=16.,py=98,pw=1288,ph=404;tabSlotRect(tab,0,px,py,pw,ph);
  // Stage-slot helpers defined FIRST so every block below can use them:
  //   tabSlot(i) = which module (0 GRANULIZER / 1 PRESLICER / 2 BEAT REPEATER)
  //                occupies shared slot i for this tab+audio-order state.
  //   showX      = is that module visible at all on this tab.
  // On stage tabs (0-4) exactly ONE module shows and it always paints at the
  // shared slot rect origin (px,py). On artefact tabs (5/6) GRANULIZER keeps
  // the full-width band while PRE SLICER / BEAT REPEATER paint inside their
  // own partner slots - xPre/xRep are those slot origins, chosen by which
  // slot each module currently occupies (this follows AUDIO ORDER live).
  static constexpr int tabPerms[6][3]={{0,1,2},{0,2,1},{1,0,2},{1,2,0},{2,0,1},{2,1,0}};
  const int permIdx=int(std::round(value(kModuleOrder)*5.));
  auto tabSlot=[&](int i){return tabPerms[permIdx][i];};
  const bool showGran=tabShowsGranular(tab),showPre=tabShowsPreslicer(tab),showRep=tabShowsRepeater(tab);
  const bool showRes=tabShowsReslice(tab),showGat=tabShowsGater(tab);
  double x=16.;
  if(showGran){x=px;
  // The granular knob grid: STRETCH (kStretchSpeed) now sits right next to
  // TRANSPOSE inside the knob grid — it was moved out of the MASTER OPTIONS
  // stack, so the y=274 strip holds both controls.
  const ParamID granular[]={kSize,kDensity,kPitch,kPosition,kChaos,kGrainMix,kTranspose,kStretchSpeed};
  const char* grainNames[]={"SIZE","DENSITY","PITCH","LOOKBACK","CHAOS","MIX","TRANSPOSE","STRETCH"};
  for(int i=0;i<8;++i)ADD(granular[i],x+14+(i%4)*99,i<4?151:274,90,92,Knob,grainNames[i]);
  ADD(kDensityFlow,x+14,368,184,25,Toggle,"DENSITY FLOW");

  // Option A: FREEZE + BUFFER SELECTION live inside the MASTER OPTIONS stack
  // box (drawn at x+208..x+404 / y=248..395 whenever the Granulizer stage is
  // slotted in the LEFT panel - see editor_mac.mm). They occupy the rows
  // freed by moving STRETCH out of this stack into the knob grid (next to
  // TRANSPOSE). Geometry notes (the layout test enforces strict rect
  // non-overlap): the lower knob row spans x+14..x+413 at y=274..366 (STRETCH
  // is its fourth knob), so the stacked controls must start at y>=367; PAN
  // MODE starts at y=398 and POSITION at x+170. The two collision-free bands
  // inside the box column are therefore:
  //   y=367..379 : FREE STRETCH toggle (original row, shrunk from 18 px)
  //   y=380..392 : FREEZE (left half) + BUFFER selection (right half), side
  //                by side - freeze beside the buffer it captures from.
  // The widgets themselves are painted by the generic control pass (see
  // drawControl: in editor_mac.mm), which switches to a compact horizontal
  // style for Toggle/Select rects shorter than 16 px tall - these three rows
  // use that style.
  // FREEZE captures audio according to the buffer selection (see
  // Engine::setFrozen / applyFreezeSelection: 50 ms fade-in, 100 ms fade-out
  // crossfades). The old RANDOM-ALL hit area next to the ON button was
  // removed per request (the parameter stays registered for automation/state
  // compatibility but has no control on screen). Only the Granulizer stage
  // carries these controls, so they are added when the Granulizer stage
  // occupies the left panel (slot(0)==0): that is exactly when the Cocoa
  // paint pass draws the MASTER OPTIONS box there. When Granulizer sits in
  // the middle/right slot, slot(0)!=0, the box is not drawn and no controls
  // are registered either - same behaviour as before.
  // (MASTER OPTIONS box is drawn by the paint pass only for the visible
  // GRANULIZER panel, which always sits at x=16 under the tabs — so the
  // FREEZE/BUFFER controls register unconditionally inside the showGran block.)
  ADD(kFreeze,x+218,380,88,12,Toggle,"FREEZE");
  ADD(kGrainBuffer,x+308,380,90,12,Select,"BUFFER");
  // FREE STRETCH keeps its column (x+218..x+398) inside the MASTER OPTIONS
  // box on the band above FREEZE/BUFFER. The old STRETCH % slider was
  // removed: kStretchSpeed is now the STRETCH knob inside the grid next to
  // TRANSPOSE - registering it twice would fail the unique-parameter-ID
  // layout test. FREE STRETCH shrinks from 18 px to a 12 px band so the
  // three stacked rows (FREE STRETCH / FREEZE+BUFFER) stay collision-free
  // and everything fits inside the box, which ends at y=395.
  ADD(kStretchOn,x+218,367,180,12,Toggle,"FREE STRETCH");

  ADD(kPanMode,x+14,398,144,30,Select,"PAN MODE");
  if(value(kPanMode)<.25)ADD(kGrainPan,x+170,396,228,30,Pan,"POSITION");
  } // showGran
  // On artefact tabs the PRE SLICER and BEAT REPEATER share their slots with
  // the compact RESLICE / GATER panels (middle slot x=452..1288, right slot
  // x=888..1304), so their controls live in the LOWER half of the panel
  // (y>=300) while the partner panel occupies y<=292. Bands are vertically
  // disjoint and columns spaced by at least their own width, which keeps the
  // strict non-overlap invariant true for every tab/order/mode/step combo.
  // PRE SLICER / BEAT REPEATER paint origin: stage tabs -> shared slot (px);
  // artefact tabs -> their own partner slot, picked by which slot the module
  // currently occupies in the audio order (slot(1)=middle x=452, slot(2)=right
  // x=888). The compact RESLICE/GATER panels live at y<=292 / y>=300 rows that
  // intersect these columns, so on artefact tabs the stage controls are packed
  // into a single compact row per module inside the LOWER half of the slot
  // (y=396..436): that band is below the GATER panel's last control row
  // (ends y=440 only at x<=1168; our row starts after it or in the next
  // column) and above the shared-slot bottom (y=502), with every control
  // width <= its column gap, keeping the strict non-overlap invariant true
  // for every tab/order/mode/step combination.
  // Card model: exactly ONE top-row module is visible at a time and it always
  // paints inside the shared full-width card rect (px,py,pw,ph). There are no
  // partner slots and no stacked artefact panels.
  const double bxPre=px,bxRep=px;
  const bool artRow=false; // legacy flag kept for readability; never set now
  // Preslicer card: its native rows inside the full-width band.
  if(showPre){x=bxPre;
  ADD(kGlitchDivision,x+18,151,132,46,Select,"SLICE");ADD(kGlitchReverse,x+18,210,132,28,Toggle,"REVERSE");
  ADD(kGlitchChance,x+164,151,112,92,Knob,"CHANCE");ADD(kGlitchMix,x+285,151,112,92,Knob,"MIX");
  ADD(kGlitchTriggerRate,x+18,294,240,46,Select,"TRIGGER EVERY");
  ADD(kGlitchVariation,x+18,415,116,40,Slider,"VARIATION");}
  // BeatRepeater card: native rows inside the full-width band. The per-stage
  // ON button sits at px+238..px+306 (y=py+8), clear of every row below.
  if(showRep){x=bxRep;
  ADD(kRepeatDivision,x+18,151,130,46,Select,"LENGTH");ADD(kRepeatOn,x+18,210,130,28,Toggle,"HOLD");
  ADD(kRepeatMix,x+165,151,112,92,Knob,"MIX");ADD(kRepeatSeq,x+282,156,115,28,Toggle,"GRID ON");
  ADD(kRepeatStep0+selectedRepeat,x+18,354,99,34,Toggle,"STEP ON");
  ADD(kRepeatRate0+selectedRepeat,x+126,350,127,44,Select,"STEP DIVISION");
  ADD(kRepeatPitch0+selectedRepeat,x+268,353,130,42,Slider,"STEP PITCH");
  ADD(kRepeatAuto,x+282,199,115,28,Toggle,"AUTO");
  ADD(kRepeatInterval,x+18,415,116,42,Select,"INTERVAL");ADD(kRepeatDuration,x+150,415,116,40,Slider,"DURATION");
  ADD(kRepeatChance,x+282,415,115,40,Slider,"CHANCE");
  }
  // LFO ENABLE keeps its home column; the artefact-tab RESLICE/GATER panels
  // live at y<=502 now, so they can no longer reach the modulation band.
  ADD(lfoID(selectedLfo,lEnabled),710,529,118,27,Toggle,"ENABLE");
  ADD(lfoID(selectedLfo,lWave),32,570,220,34,Select,"WAVE");
  ADD(lfoID(selectedLfo,lSync),264,574,68,27,Toggle,"SYNC");ADD(lfoID(selectedLfo,lReset),344,574,108,27,Toggle,"RETRIGGER");
  ADD(kRandomSteps0+selectedLfo,464,570,130,34,Select,"POINTS");ADD(lfoID(selectedLfo,lGrid),606,570,222,34,Select,"SYNC RATE");
  ADD(lfoID(selectedLfo,lHz),278,610,124,40,Slider,"FREE RATE");ADD(lfoID(selectedLfo,lDepth),414,610,124,40,Slider,"DEPTH");
  ADD(lfoID(selectedLfo,lPhase),550,610,124,40,Slider,"PHASE");ADD(lfoID(selectedLfo,lGlide),686,610,124,40,Slider,"GLIDE");
  ADD(kLfoSpeed0+selectedLfo,32,690,104,40,Select,"SPEED");
  ADD(kModWaveRnd0+selectedLfo,144,690,108,40,Select,"MOD WAVE RND");
  for(int i=0;i<6;++i){ADD(slotTarget(selectedLfo,i),268+(i%3)*190,662+(i/3)*44,112,40,Select,"DESTINATION");ADD(slotAmount(selectedLfo,i),384+(i%3)*190,662+(i/3)*44,60,40,Slider,"AMT");}
  // XY ENABLE lives at x=1185; the per-stage ON button (px+238..px+306) never
  // reaches that column in the card model.
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
  // Per-stage ON buttons: one per VISIBLE top-row module, at its tab rect.
  for(int stage=0;stage<5;++stage){double sx,sy,sw,sh;tabSlotRect(tab,stage,sx,sy,sw,sh);
    if(!tabShowsStagePanel(tab,stage))continue;
    ADD(kGrainEnabled+stage,sx+238,sy+8,68,28,Toggle,"ON");}

  // GATER module controls: the original full-width row (y~893) stays on tabs
  // where the bottom rack is visible AND the GATER is not pulled up into the
  // shared slot band (tabs 0-2). On the GATER solo tab and on the RESLICE
  // solo tab when the GATER occupies the LEFT slot (GATER->RESLICE audio
  // order), the panel lives in the shared slot band (y=98..502) - the other
  // module's ON button keeps that row free. On the artefact tabs it becomes
  // the compact lower-right panel below RESLICE.
  // GATER card: when the GATER is the visible module it owns the full-width
  // band; its controls paint in the native fixed columns (the same geometry
  // the classic single-page layout used), starting after the per-stage ON
  // button at px+238..px+306. When another card is active, GATER registers
  // nothing (its audio still runs through the chain unchanged).
  if(showGat) {
    ADD(kGaterGrid,px+320,py+62,100,40,Select,"GATE RATE");
    ADD(kGaterLengthRnd,px+450,py+67,126,27,Toggle,"LENGTH RND");
    ADD(kGaterStepRnd,px+586,py+67,118,27,Toggle,"STEP RND");
    ADD(kGaterChance,px+714,py+62,126,40,Slider,"CHANCE");
    ADD(kGaterMinLength,px+850,py+62,166,40,Slider,"MIN LENGTH");
    ADD(kGaterLength0+selectedGate,px+1026,py+62,96,40,Slider,"STEP LENGTH");
    ADD(kGaterSustain0+selectedGate,px+1132,py+62,122,40,Slider,"SUSTAIN");
    ADD(kGaterLatch,px+320,py+117,70,27,Toggle,"LATCH");
  } else if(!showRes) {
    // Neither RESLICE nor GATER is the active card (tabs 0-2): the legacy
    // bottom rack rows stay registered for hit-testing continuity exactly as
    // before the tab feature existed.
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
  }

  // INPUT DE-CLICK / SENSITIVITY are drawn in the header (row 2, next to LOAD/SAVE);
  // they are registered here for hit-testing and value display only.
  ADD(kInputDeclick,684,56,56,28,Toggle,"");
  ADD(kDeclickSensitivity,548,56,130,28,Slider,"");

  // RESLICE keeps its original full-width row (y~773..818) whenever the
  // bottom rack is visible and the GATER has NOT been pulled up into the
  // shared slot band; on the RESLICE solo tab (and when it shares the top
  // area with a pulled-up GATER) it lives inside that band instead. The two
  // solo panels occupy disjoint column ranges, so they never intersect.
  // RESLICE card: when visible it owns the full-width band (native columns,
  // starting after its per-stage ON button at rx+238..rx+306). When another
  // card is active and neither RESLICE nor GATER shows, the legacy bottom-rack
  // RESLICE row stays registered exactly as before the tab feature.
  double rx,ry,rw,rh;tabSlotRect(tab,3,rx,ry,rw,rh); // card rect origin
  if(showRes) {
    ADD(kResliceLength,rx+322,ry+7,112,40,Slider,"WINDOW");
    ADD(kResliceMix,rx+442,ry+7,112,40,Slider,"MIX");
    ADD(kResliceRndOn,rx+562,ry+12,126,27,Toggle,"STEP RND");
    ADD(kResliceRndRate,rx+696,ry+7,142,40,Select,"RND RATE");
    ADD(kResliceIndex0+selectedReslice,rx+846,ry+7,132,40,Select,"SOURCE SLICE");
  } else if(!showGat) {
  ADD(kResliceEnabled,148,778,72,27,Toggle,"ON");
  // PRESLICER: WINDOW and MIX are horizontal slides now (replacing the
  // previous oversized knobs), in the same compact row style as RND RATE /
  // SOURCE SLICE. WINDOW snaps to the four slice lengths as it drags; MIX is
  // a continuous 0-100% wet slide. Both sit at y=773 so their value readouts
  // (drawn below the track) stay clear of the STEP RND toggle at y=778.
  ADD(kResliceLength,384,773,112,40,Slider,"WINDOW");
  ADD(kResliceMix,504,773,112,40,Slider,"MIX");
  ADD(kResliceRndOn,624,778,126,27,Toggle,"STEP RND");
  ADD(kResliceRndRate,758,773,142,40,Select,"RND RATE");
  ADD(kResliceIndex0+selectedReslice,908,773,132,40,Select,"SOURCE SLICE");
  }
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