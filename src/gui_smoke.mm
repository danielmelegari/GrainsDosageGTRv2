#import <Cocoa/Cocoa.h>
#include "plugin.cpp"
#include "mockup_ui.h"
#include "gui_host_guard.h"
#include <stdexcept>
#include <iostream>
@protocol GrainsGuiInspection
- (BOOL)skinLoaded;
- (BOOL)controlsFit;
- (NSUInteger)staticBuilds;
- (void)invalidateStaticScene;
@end
static void checkGui(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
int main(int argc,char** argv){@autoreleasepool {try{
 [NSApplication sharedApplication];[NSApp setActivationPolicy:NSApplicationActivationPolicyAccessory];
 auto* c=new Controller;checkGui(c->initialize(nullptr)==kResultOk,"Controller init");
 aztec::GuiHostGuard hostGuard;c->setComponentHandler(&hostGuard);
 auto* view=c->createView(ViewType::kEditor);checkGui(view,"Editor factory");ViewRect size;view->getSize(&size);
 NSWindow* window=[[NSWindow alloc] initWithContentRect:NSMakeRect(0,0,size.getWidth(),size.getHeight()) styleMask:NSWindowStyleMaskTitled backing:NSBackingStoreBuffered defer:NO];window.releasedWhenClosed=NO;
 checkGui(view->attached((__bridge void*)window.contentView,kPlatformTypeNSView)==kResultOk,"Attach");
 NSView* surface=window.contentView.subviews.firstObject;checkGui(surface,"Native view");
 checkGui([(id<GrainsGuiInspection>)surface skinLoaded],"Approved rack PNG assets loaded");
 auto send=[&](NSEventType type,double x,double y,int clicks=1){
  aztec::mockup::Viewport fit(surface.bounds.size.width,surface.bounds.size.height);
  NSPoint point=[surface convertPoint:NSMakePoint(fit.x+x*fit.scale,fit.y+y*fit.scale) toView:nil];
  NSEvent* e=[NSEvent mouseEventWithType:type location:point modifierFlags:0 timestamp:0 windowNumber:window.windowNumber context:nil eventNumber:1 clickCount:clicks pressure:1.];
  if(type==NSEventTypeLeftMouseDown)[surface mouseDown:e];else if(type==NSEventTypeLeftMouseDragged)[surface mouseDragged:e];else [surface mouseUp:e];
 };
 auto click=[&](double x,double y,int n=1){send(NSEventTypeLeftMouseDown,x,y,n);send(NSEventTypeLeftMouseUp,x,y,n);};
 auto toggle=[&](int id,double x,double y){double before=c->getParamNormalized(id);click(x,y);checkGui(c->getParamNormalized(id)==(before>=.5?0.:1.),"Toggle binding");c->setParamNormalized(id,before);};
 using namespace aztec;
 auto controlRect=[&](ParamID id){auto list=mockup::controls([&](ParamID p){return c->getParamNormalized(p);},tabFromValue(c->getParamNormalized(kUiTab)),id==lfoID(2,lEnabled)?2:0,0,0,0);for(const auto& item:list)if(item.id==id)return item.r;throw std::runtime_error("Missing control");};
 auto toggleControl=[&](ParamID id){auto r=controlRect(id);toggle(id,r.x+r.w/2,r.y+r.h/2);};
 auto clickRect=[&](mockup::Rect r){click(r.x+r.w/2,r.y+r.h/2);};
 auto chooseTab=[&](int t){auto r=mockup::tabRect(t);send(NSEventTypeLeftMouseDown,r.x+r.w*.4,r.y+r.h/2);checkGui(tabFromValue(c->getParamNormalized(kUiTab))==mockup::routeChain([&](ParamID id){return c->getParamNormalized(id);})[t],"Module selects on mouse down");send(NSEventTypeLeftMouseUp,r.x+r.w*.4,r.y+r.h/2);};
 toggleControl(kFreeze);toggleControl(kInputDeclick);toggleControl(kXYEnable);toggleControl(kMasterFilter);toggleControl(kMasterLimiter);
 auto sizeRect=controlRect(kSize);double knobX=sizeRect.x+sizeRect.w/2,knobY=sizeRect.y+sizeRect.h/2;
 c->setParamNormalized(kSize,.25);send(NSEventTypeLeftMouseDown,knobX,knobY);send(NSEventTypeLeftMouseDragged,knobX,knobY-45);send(NSEventTypeLeftMouseUp,knobX,knobY-45);
 checkGui(std::abs(c->getParamNormalized(kSize)-.5)<1e-6,"Grain knob drag");click(knobX,knobY,2);checkGui(std::abs(c->getParamNormalized(kSize)-.25)<1e-6,"Double click reset");
 const ParamID enabled[]={kGrainEnabled,kGlitchEnabled,kRepeatEnabled,kResliceEnabled,kGaterEnabled};
 for(int t=0;t<5;++t){chooseTab(t);checkGui(tabFromValue(c->getParamNormalized(kUiTab))==t,"Tab hit target");checkGui([(id<GrainsGuiInspection>)surface controlsFit],"Controls fit each tab");toggleControl(enabled[t]);auto led=mockup::ledRect(t);toggle(enabled[t],led.x+led.w/2,led.y+led.h/2);[surface display];}
 chooseTab(3);toggleControl(kResliceRndOn);clickRect(mockup::stepRect(5,3));// single click selects without toggling
 // Double-click a reslice step toggles its stored enabled state.
 double old=c->getParamNormalized(kResliceStep0+5);click(mockup::stepRect(5,3).x+12,mockup::stepRect(5,3).y+30,2);checkGui(c->getParamNormalized(kResliceStep0+5)==(old>.5?0.:1.),"Reslice step toggle");
 chooseTab(4);toggle(kGaterState0,mockup::stepRect(0,4).x+12,mockup::stepRect(0,4).y+20);
 chooseTab(0);clickRect(mockup::lfoRect(2));toggleControl(lfoID(2,lEnabled));
 click(aztec::mockup::xy.x+12,aztec::mockup::xy.y+12);checkGui(c->getParamNormalized(kXYX)==0&&c->getParamNormalized(kXYY)==1,"XY corner");
 c->setParamNormalized(kXYX,.5);c->setParamNormalized(kXYY,.5);
 auto routeValue=[&](ParamID id){return c->getParamNormalized(id);};
 auto original=mockup::routeChain(routeValue);
 send(NSEventTypeLeftMouseDown,mockup::tabRect(4).x+40,175);send(NSEventTypeLeftMouseDragged,mockup::tabRect(0).x+20,175);send(NSEventTypeLeftMouseUp,mockup::tabRect(0).x+20,175);
 auto moved=mockup::routeChain(routeValue);checkGui(moved[0]==original[4]&&moved[1]==original[0],"Routing inserts before first");
 click(mockup::tabRect(0).x+20,175);checkGui(tabFromValue(c->getParamNormalized(kUiTab))==original[4],"Moved top button selects its module");
 auto led0=mockup::ledRect(0);toggle(enabled[original[4]],led0.x+led0.w/2,led0.y+led0.h/2);
 send(NSEventTypeLeftMouseDown,mockup::tabRect(0).x+20,175);send(NSEventTypeLeftMouseDragged,mockup::tabRect(4).x+mockup::tabRect(4).w-3,175);send(NSEventTypeLeftMouseUp,mockup::tabRect(4).x+mockup::tabRect(4).w-3,175);
 checkGui(mockup::routeChain(routeValue)==original,"Routing inserts after last");
 double beforeCancel=c->getParamNormalized(kRoutingOrder);
 send(NSEventTypeLeftMouseDown,mockup::tabRect(0).x+20,175);send(NSEventTypeLeftMouseDragged,100,100);send(NSEventTypeLeftMouseUp,100,100);
 checkGui(c->getParamNormalized(kRoutingOrder)==beforeCancel,"Routing outside drop cancels");
 // Locks bind to their mixes and only intercept randomisation.
 chooseTab(0);c->setParamNormalized(kGrainMix,.271);c->setParamNormalized(kMixLock0,0.);clickRect(controlRect(kMixLock0));checkGui(c->getParamNormalized(kMixLock0)==1.,"Mix lock click");
 clickRect(mockup::randomRect(0));checkGui(c->getParamNormalized(kGrainMix)==.271,"Locked mix survives Random");clickRect(controlRect(kMixLock0));clickRect(mockup::randomRect(0));checkGui(c->getParamNormalized(kGrainMix)!=.271,"Unlocked mix randomises");
 c->setParamNormalized(kReverbMix,.25);send(NSEventTypeLeftMouseDown,113,1310);send(NSEventTypeLeftMouseDragged,113,1280);send(NSEventTypeLeftMouseUp,113,1280);checkGui(c->getParamNormalized(kReverbMix)>.4,"Vertical reverb fader");
 clickRect(mockup::next);checkGui(c->getParamNormalized(kRoutingOrder)==factoryPreset(0)[kRoutingOrder],"First categorised preset");clickRect(mockup::next);checkGui(c->getParamNormalized(kRoutingOrder)==factoryPreset(1)[kRoutingOrder],"Next categorised preset");
 c->setParamNormalized(kReverbSource,1.);
 // A host can call onSize without first honouring checkSizeConstraint.
 ViewRect staleHostSize(0,0,900,600);view->onSize(&staleHostSize);for(int slot=0;slot<5;++slot)chooseTab(slot);
 [surface display];NSUInteger before=[(id<GrainsGuiInspection>)surface staticBuilds];
 auto benchmark=[&](bool full){auto start=std::chrono::steady_clock::now();for(int frame=0;frame<12;++frame){c->setParamNormalized(kUiLfoPhase0,frame/12.);if(full)[(id<GrainsGuiInspection>)surface invalidateStaticScene];[surface display];}return std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count()/12.;};
 double cachedMs=benchmark(false);checkGui([(id<GrainsGuiInspection>)surface staticBuilds]==before,"Monitor frames reuse static artwork cache");
 c->setParamNormalized(kMix,.37);[surface display];checkGui([(id<GrainsGuiInspection>)surface staticBuilds]==before+1,"Control edit refreshes cached readout");
 double fullMs=benchmark(true);std::cout<<"GUI paint benchmark: cached="<<cachedMs<<" ms/frame; full="<<fullMs<<" ms/frame; static artwork reused across monitor frames\n";
 ViewRect resized(0,0,816,759);checkGui(view->checkSizeConstraint(&resized)==kResultOk,"Resize constraints");view->onSize(&resized);checkGui([(id<GrainsGuiInspection>)surface controlsFit],"Resize keeps controls visible");
 [surface display];NSBitmapImageRep* bitmap=[surface bitmapImageRepForCachingDisplayInRect:surface.bounds];[surface cacheDisplayInRect:surface.bounds toBitmapImageRep:bitmap];
 NSString* path=argc>1?[NSString stringWithUTF8String:argv[1]]:@"GrainsDosage-GUI.png";
 checkGui([[bitmap representationUsingType:NSBitmapImageFileTypePNG properties:@{}] writeToFile:path atomically:YES],"Native screenshot");
 for(int t=0;t<5;++t){c->setParamNormalized(kUiTab,tabToValue(t));[surface display];NSBitmapImageRep* shot=[surface bitmapImageRepForCachingDisplayInRect:surface.bounds];[surface cacheDisplayInRect:surface.bounds toBitmapImageRep:shot];NSString* name=[path.stringByDeletingPathExtension stringByAppendingFormat:@"-tab-%d.png",t];checkGui([[shot representationUsingType:NSBitmapImageFileTypePNG properties:@{}] writeToFile:name atomically:YES],"Module screenshot");}

 checkGui(hostGuard.rejectedTabEdits==0,"Module selection never edits a host read-only parameter");
 view->removed();view->release();c->setComponentHandler(nullptr);c->terminate();c->release();[window close];std::cout<<"PASS: native skin, tabs, controls, step editing, XY, resize and screenshot\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}}
