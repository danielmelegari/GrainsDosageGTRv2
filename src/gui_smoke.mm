#import <Cocoa/Cocoa.h>
#include "plugin.cpp"
#include <cmath>
#include <stdexcept>
#include <iostream>
@protocol GrainsGuiInspection
- (BOOL)skinLoaded;
- (BOOL)controlsFit;
@end
static void checkGui(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
int main(int argc,char** argv){
  @autoreleasepool {
    try{
      [NSApplication sharedApplication];[NSApp setActivationPolicy:NSApplicationActivationPolicyAccessory];
      auto* controller=new Controller;checkGui(controller->initialize(nullptr)==kResultOk,"Controller init");
      IPlugView* view=controller->createView(ViewType::kEditor);checkGui(view!=nullptr,"Editor factory");
      ViewRect size;checkGui(view->getSize(&size)==kResultOk,"Size");
      NSWindow* window=[[NSWindow alloc] initWithContentRect:NSMakeRect(0,0,size.getWidth(),size.getHeight()) styleMask:NSWindowStyleMaskTitled backing:NSBackingStoreBuffered defer:NO];window.releasedWhenClosed=NO;
      checkGui(view->attached((__bridge void*)window.contentView,kPlatformTypeNSView)==kResultOk,"Attach");
      checkGui(window.contentView.subviews.count==1,"One native editor surface");
      NSView* surface=window.contentView.subviews.firstObject;
      checkGui(![surface isKindOfClass:[NSScrollView class]],"No scroll container");
      checkGui(surface.frame.size.width==792&&surface.frame.size.height==816,"All controls in one 792x816 window");
      checkGui([(id<GrainsGuiInspection>)surface skinLoaded],"Artwork details loaded");
      checkGui([(id<GrainsGuiInspection>)surface controlsFit],"Every control fits the canvas");
      NSString* path=argc>1?[NSString stringWithUTF8String:argv[1]]:@"GrainsDosage-GUI.png";
      auto render=[&]()->NSData*{
        [surface setNeedsDisplay:YES];[surface displayIfNeeded];
        NSRect bounds=surface.bounds;
        const NSInteger pixelWidth=NSInteger(std::lround(bounds.size.width*2.));
        const NSInteger pixelHeight=NSInteger(std::lround(bounds.size.height*2.));
        NSBitmapImageRep* bitmap=[[NSBitmapImageRep alloc] initWithBitmapDataPlanes:nil
          pixelsWide:pixelWidth pixelsHigh:pixelHeight bitsPerSample:8 samplesPerPixel:4
          hasAlpha:YES isPlanar:NO colorSpaceName:NSCalibratedRGBColorSpace
          bytesPerRow:0 bitsPerPixel:0];
        checkGui(bitmap!=nil,"2x bitmap allocation");bitmap.size=bounds.size;
        [surface cacheDisplayInRect:surface.bounds toBitmapImageRep:bitmap];
        checkGui(bitmap.pixelsWide==pixelWidth&&bitmap.pixelsHigh==pixelHeight,"Screenshot rendered at 2x resolution");
        return [bitmap representationUsingType:NSBitmapImageFileTypePNG properties:@{}];
      };
      auto send=[&](NSEventType type,double x,double y,int clicks=1){
        NSPoint point=[surface convertPoint:NSMakePoint(x*surface.bounds.size.width/1320.,y*surface.bounds.size.height/1360.) toView:nil];
        NSEvent* e=[NSEvent mouseEventWithType:type location:point modifierFlags:0 timestamp:0 windowNumber:window.windowNumber context:nil eventNumber:1 clickCount:clicks pressure:1.];
        if(type==NSEventTypeLeftMouseDown)[surface mouseDown:e];
        if(type==NSEventTypeLeftMouseDragged)[surface mouseDragged:e];
        if(type==NSEventTypeLeftMouseUp)[surface mouseUp:e];
      };
      // Exercise new controls through native mouse dispatch, then restore defaults.
      auto toggleCheck=[&](int id,double x,double y,const char* message){double before=controller->getParamNormalized(id);send(NSEventTypeLeftMouseDown,x,y);send(NSEventTypeLeftMouseUp,x,y);checkGui(controller->getParamNormalized(id)==(before>=.5?0.:1.),message);controller->setParamNormalized(id,before);};
      toggleCheck(aztec::kXYEnable,1235,542,"XY Enable hit target");
      toggleCheck(aztec::kResliceRndOn,687,791,"Reslice Step RND hit target");
      toggleCheck(aztec::kFilterSeqOn,270,1184,"Filter Sequencer hit target");
      NSData* initial=render();checkGui([initial writeToFile:path atomically:YES],"Initial screenshot");
      send(NSEventTypeLeftMouseDown,96,194);send(NSEventTypeLeftMouseDragged,96,149);send(NSEventTypeLeftMouseUp,96,149);
      checkGui(std::abs(controller->getParamNormalized(aztec::kSize)-.5)<1e-9,"Knob drag updates exact value");
      NSData* changed=render();checkGui(![initial isEqualToData:changed],"Knob image and readout redraw after drag");
      send(NSEventTypeLeftMouseDown,273,194);send(NSEventTypeLeftMouseDragged,273,-1000);send(NSEventTypeLeftMouseUp,273,-1000);
      checkGui(controller->getParamNormalized(aztec::kPitch)==1.,"Pitch knob reaches +48");
      NSData* high=render();
      send(NSEventTypeLeftMouseDown,273,194);send(NSEventTypeLeftMouseDragged,273,1120);send(NSEventTypeLeftMouseUp,273,1120);
      checkGui(controller->getParamNormalized(aztec::kPitch)==0.,"Pitch knob reaches -48");
      checkGui(![high isEqualToData:render()],"Opposite positions render differently");
      controller->setParamNormalized(aztec::kPitch,.5);
      [surface performSelector:@selector(tick:) withObject:nil];
      // STRETCH moved into the granular knob grid: it now sits right next to
      // TRANSPOSE (row 2 = y 274..366; STRETCH center x=328.5, TRANSPOSE
      // center x=223.5). A drag from y=300 up to y=282 maps v .5 -> .3 for
      // both knobs (18 px over the full 0..1 range).
      send(NSEventTypeLeftMouseDown,328,300);send(NSEventTypeLeftMouseDragged,328,282);send(NSEventTypeLeftMouseUp,328,282);
      checkGui(std::abs(controller->getParamNormalized(aztec::kStretchSpeed)-.3)<1e-9,"STRETCH knob in granular panel");
      send(NSEventTypeLeftMouseDown,223,300);send(NSEventTypeLeftMouseDragged,223,282);send(NSEventTypeLeftMouseUp,223,282);
      checkGui(std::abs(controller->getParamNormalized(aztec::kTranspose)-58./96.)<1e-9,"Transpose knob in granular panel");
      send(NSEventTypeLeftMouseDown,400,725);send(NSEventTypeLeftMouseDragged,415,725);send(NSEventTypeLeftMouseUp,415,725);
      checkGui(std::abs(controller->getParamNormalized(aztec::slotAmount(0,3))-.75)<1e-9,"LFO assignment amount slider");
      send(NSEventTypeLeftMouseDown,150,120);send(NSEventTypeLeftMouseDragged,600,120);send(NSEventTypeLeftMouseUp,600,120);
      checkGui(std::abs(controller->getParamNormalized(aztec::kModuleOrder)-2./6.)<1e-9,"Drag module headers changes audio order");
      send(NSEventTypeLeftMouseDown,56,222);send(NSEventTypeLeftMouseUp,56,222);
      checkGui(controller->getParamNormalized(aztec::kGlitchReverse)==1.,"Moved glitch reverse remains clickable");
      send(NSEventTypeLeftMouseDown,532,194);send(NSEventTypeLeftMouseDragged,532,176);send(NSEventTypeLeftMouseUp,532,176);
      checkGui(std::abs(controller->getParamNormalized(aztec::kSize)-.6)<1e-9,"Moved knob retains parameter identity");
      send(NSEventTypeLeftMouseDown,1006,607.5);send(NSEventTypeLeftMouseUp,1006,607.5);
      checkGui(std::abs(controller->getParamNormalized(aztec::kXYX)-.75)<.001,"XY X");
      checkGui(std::abs(controller->getParamNormalized(aztec::kXYY)-.75)<.001,"XY Y");
      NSString* changedPath=[[path stringByDeletingPathExtension] stringByAppendingString:@"-Reordered.png"];
      checkGui([render() writeToFile:changedPath atomically:YES],"Reordered screenshot");
      ViewRect resized(0,0,792,816);checkGui(view->checkSizeConstraint(&resized)==kResultOk,"Resize constraints");
      checkGui(view->onSize(&resized)==kResultOk,"Resize");
      checkGui(surface.bounds.size.width==792&&surface.bounds.size.height==816,"Minimum size keeps aspect ratio");
      checkGui([(id<GrainsGuiInspection>)surface controlsFit],"All controls fit after resize");
      send(NSEventTypeLeftMouseDown,532,194);send(NSEventTypeLeftMouseDragged,532,176);send(NSEventTypeLeftMouseUp,532,176);
      checkGui(std::abs(controller->getParamNormalized(aztec::kSize)-.7)<1e-9,"Scaled knob keeps accurate hit testing");
      send(NSEventTypeLeftMouseDown,532,194,2);send(NSEventTypeLeftMouseUp,532,194,2);
      checkGui(std::abs(controller->getParamNormalized(aztec::kSize)-.25)<1e-9,"Double click restores default");
      send(NSEventTypeLeftMouseDown,489,1325);send(NSEventTypeLeftMouseUp,489,1325);
      checkGui(controller->getParamNormalized(aztec::kMasterLimiter)==0.,"Master limiter toggle");
      send(NSEventTypeLeftMouseDown,197,1028);send(NSEventTypeLeftMouseUp,197,1028);
      checkGui(controller->getParamNormalized(aztec::kMasterFilter)==1.,"Master filter toggle");
      send(NSEventTypeLeftMouseDown,876,1028);send(NSEventTypeLeftMouseUp,876,1028);checkGui(controller->getParamNormalized(aztec::kReverbKill)==1.,"Kill Dry button");
      double cutoffBefore=controller->getParamNormalized(aztec::kFilterCutoff);
      send(NSEventTypeLeftMouseDown,244,1089);send(NSEventTypeLeftMouseDragged,244,1071);send(NSEventTypeLeftMouseUp,244,1071);
      checkGui(std::abs(controller->getParamNormalized(aztec::kFilterCutoff)-cutoffBefore-.1)<1e-9,"Master cutoff knob");
      send(NSEventTypeLeftMouseDown,364,1325);send(NSEventTypeLeftMouseUp,364,1325);
      checkGui(controller->getParamNormalized(aztec::kNormalize)==1.,"Normalize toggle");
      send(NSEventTypeLeftMouseDown,745,1028);send(NSEventTypeLeftMouseUp,745,1028);
      checkGui(controller->getParamNormalized(aztec::kReverbOn)==1.,"Reverb toggle");
      send(NSEventTypeLeftMouseDown,603,1115);send(NSEventTypeLeftMouseUp,603,1115);
      checkGui(controller->getParamNormalized(aztec::kReverbStep0)==0.,"Reverb step toggle");
      // Module ON/OFF toggles sit at slotX[slot(stage)]+238..306, y 106..134
      // (editor_layout_win.inl, shared with Cocoa). Default order: stage 0
      // (kGrainEnabled) is in physical slot 0 -> logical x 254, y 120.
      // The old x=724 click hit slot 1's RANDOM button and never toggled the
      // module bypass, so this check always failed.
      send(NSEventTypeLeftMouseDown,254,120);send(NSEventTypeLeftMouseUp,254,120);
      checkGui(controller->getParamNormalized(aztec::kGrainEnabled)==0.,"Module bypass");
      controller->setParamNormalized(aztec::kPanMode,.5);
      checkGui(controller->getParamNormalized(aztec::kPanMode)==.5,"Alternate pan mode");
      controller->setParamNormalized(aztec::kPanMode,0.);
      checkGui(controller->getParamNormalized(aztec::kPanMode)==0.,"Manual pan mode");
      controller->setParamNormalized(aztec::kGrainPan,.5);
      send(NSEventTypeLeftMouseDown,690,414);send(NSEventTypeLeftMouseDragged,727,414);send(NSEventTypeLeftMouseUp,727,414);
      checkGui(controller->getParamNormalized(aztec::kGrainPan)>.59,"Pan slider");
      for(int i=0;i<128;++i)controller->setParamNormalized(aztec::kUiWave0+i,.3+.25*std::sin(i*.2));
      controller->setParamNormalized(aztec::kUiGrainStart,.4);controller->setParamNormalized(aztec::kUiGrainEnd,.6);controller->setParamNormalized(aztec::kUiGrainHead,.5);controller->setParamNormalized(aztec::kUiGrainActive,1.);controller->setParamNormalized(aztec::kUiWaveSeconds,2.5/16.);
      NSData* liveWave=render();controller->setParamNormalized(aztec::kUiGrainActive,0.);checkGui(![liveWave isEqualToData:render()],"Active grain region changes waveform rendering");
      NSString* wavePath=[[path stringByDeletingPathExtension] stringByAppendingString:@"-Waveform.png"];checkGui([liveWave writeToFile:wavePath atomically:YES],"Waveform screenshot");
      double otherMix=controller->getParamNormalized(aztec::kGlitchMix);
      send(NSEventTypeLeftMouseDown,820,120);send(NSEventTypeLeftMouseUp,820,120);
      checkGui(controller->getParamNormalized(aztec::kSize)!=.25,"Random granular changes its knobs");
      checkGui(controller->getParamNormalized(aztec::kGlitchMix)==otherMix,"Random leaves other modules alone");
      send(NSEventTypeLeftMouseDown,370,120);send(NSEventTypeLeftMouseUp,370,120);
      checkGui(controller->getParamNormalized(aztec::kGlitchMix)>=.5,"Random glitch");
      send(NSEventTypeLeftMouseDown,1240,120);send(NSEventTypeLeftMouseUp,1240,120);
      checkGui(controller->getParamNormalized(aztec::kRepeatMix)>=.5,"Random repeater");
      controller->setParamNormalized(aztec::kUiGlitch,1.);controller->setParamNormalized(aztec::kUiRepeat,1.);controller->setParamNormalized(aztec::kUiLevel,.85);controller->setParamNormalized(aztec::kUiStep,7./15.);
      NSString* ledPath=[[path stringByDeletingPathExtension] stringByAppendingString:@"-LED.png"];
      checkGui([render() writeToFile:ledPath atomically:YES],"LED state screenshot");
      checkGui(view->removed()==kResultOk&&window.contentView.subviews.count==0,"Detach");
      checkGui(view->attached((__bridge void*)window.contentView,kPlatformTypeNSView)==kResultOk,"Reopen");
      // attached() creates a new native surface; send/render capture this variable by reference.
      // Never dispatch to the detached surface retained by this test's strong local.
      checkGui(window.contentView.subviews.count==1,"One surface after reopen");
      surface=window.contentView.subviews.firstObject;
      checkGui(surface!=nil&&surface.superview==window.contentView,"Reopened surface belongs to host");
      checkGui([(id<GrainsGuiInspection>)surface controlsFit],"Reopened controls fit");
      send(NSEventTypeLeftMouseDown,182,912);send(NSEventTypeLeftMouseUp,182,912);checkGui(controller->getParamNormalized(aztec::kGaterEnabled)==1.,"gater on");
      send(NSEventTypeLeftMouseDown,1200,912);send(NSEventTypeLeftMouseUp,1200,912);checkGui(controller->getParamNormalized(aztec::kGaterLatch)==1.,"gater latch");
      send(NSEventTypeLeftMouseDown,67,963);send(NSEventTypeLeftMouseUp,67,963);checkGui(controller->getParamNormalized(aztec::kGaterState0)==0.,"Wet/Off step toggle");
      send(NSEventTypeLeftMouseDown,712,67);send(NSEventTypeLeftMouseUp,712,67);checkGui(controller->getParamNormalized(aztec::kInputDeclick)==0.,"input de-click toggle");
      view->removed();view->release();controller->terminate();controller->release();[window close];
      std::cout<<"PASS: single window, artwork, live knob rendering/values/endpoints, module drag, moved controls, XY, resize and reopen\n";
      return 0;
    }catch(const std::exception& e){std::cerr<<"GUI smoke failed: "<<e.what()<<"\n";return 1;}
  }
}
