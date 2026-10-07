#import <Cocoa/Cocoa.h>
#include "plugin.cpp"
#include "mockup_ui.h"
#include <stdexcept>
#include <iostream>
@protocol GrainsGuiInspection
- (BOOL)skinLoaded;
- (BOOL)controlsFit;
@end
static void checkGui(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
int main(int argc,char** argv){@autoreleasepool {try{
 [NSApplication sharedApplication];[NSApp setActivationPolicy:NSApplicationActivationPolicyAccessory];
 auto* c=new Controller;checkGui(c->initialize(nullptr)==kResultOk,"Controller init");
 auto* view=c->createView(ViewType::kEditor);checkGui(view,"Editor factory");ViewRect size;view->getSize(&size);
 NSWindow* window=[[NSWindow alloc] initWithContentRect:NSMakeRect(0,0,size.getWidth(),size.getHeight()) styleMask:NSWindowStyleMaskTitled backing:NSBackingStoreBuffered defer:NO];window.releasedWhenClosed=NO;
 checkGui(view->attached((__bridge void*)window.contentView,kPlatformTypeNSView)==kResultOk,"Attach");
 NSView* surface=window.contentView.subviews.firstObject;checkGui(surface,"Native view");
 checkGui([(id<GrainsGuiInspection>)surface skinLoaded],"Supplied knob PNG loaded");
 auto send=[&](NSEventType type,double x,double y,int clicks=1){
  NSPoint point=[surface convertPoint:NSMakePoint(x*surface.bounds.size.width/aztec::mockup::width,y*surface.bounds.size.height/aztec::mockup::height) toView:nil];
  NSEvent* e=[NSEvent mouseEventWithType:type location:point modifierFlags:0 timestamp:0 windowNumber:window.windowNumber context:nil eventNumber:1 clickCount:clicks pressure:1.];
  if(type==NSEventTypeLeftMouseDown)[surface mouseDown:e];else if(type==NSEventTypeLeftMouseDragged)[surface mouseDragged:e];else [surface mouseUp:e];
 };
 auto click=[&](double x,double y,int n=1){send(NSEventTypeLeftMouseDown,x,y,n);send(NSEventTypeLeftMouseUp,x,y,n);};
 auto toggle=[&](int id,double x,double y){double before=c->getParamNormalized(id);click(x,y);checkGui(c->getParamNormalized(id)==(before>=.5?0.:1.),"Toggle binding");c->setParamNormalized(id,before);};
 using namespace aztec;
 toggle(kFreeze,440,442);toggle(kInputDeclick,1350,66);toggle(kXYEnable,1545,703);toggle(kMasterFilter,209,1158);toggle(kMasterLimiter,700,1481);
 c->setParamNormalized(kSize,.25);send(NSEventTypeLeftMouseDown,158,290);send(NSEventTypeLeftMouseDragged,158,245);send(NSEventTypeLeftMouseUp,158,245);
 checkGui(std::abs(c->getParamNormalized(kSize)-.5)<1e-6,"Grain knob drag");click(158,290,2);checkGui(std::abs(c->getParamNormalized(kSize)-.25)<1e-6,"Double click reset");
 const ParamID enabled[]={kGrainEnabled,kGlitchEnabled,kRepeatEnabled,kResliceEnabled,kGaterEnabled};
 for(int t=0;t<5;++t){click(140+252*t,135);checkGui(tabFromValue(c->getParamNormalized(kUiTab))==t,"Tab hit target");checkGui([(id<GrainsGuiInspection>)surface controlsFit],"Controls fit each tab");toggle(enabled[t],1190,442);auto led=mockup::ledRect(t);toggle(enabled[t],led.x+led.w/2,led.y+led.h/2);[surface display];}
 click(140+3*252,135);toggle(kResliceRndOn,700,273);click(61+76*5+16,536);// single click selects without toggling
 // Double-click a reslice step toggles its stored enabled state.
 double old=c->getParamNormalized(kResliceStep0+5);click(61+76*5+16,536,2);checkGui(c->getParamNormalized(kResliceStep0+5)==(old>.5?0.:1.),"Reslice step toggle");
 click(140+4*252,135);toggle(kGaterState0,77,536);
 click(140,135);click(688+105*2,703);toggle(lfoID(2,lEnabled),540,703);
 click(aztec::mockup::xy.x+12,aztec::mockup::xy.y+12);checkGui(c->getParamNormalized(kXYX)==0&&c->getParamNormalized(kXYY)==1,"XY corner");
 c->setParamNormalized(kXYX,.5);c->setParamNormalized(kXYY,.5);
 auto routeValue=[&](ParamID id){return c->getParamNormalized(id);};
 auto original=mockup::routeChain(routeValue);
 send(NSEventTypeLeftMouseDown,1457,1556);send(NSEventTypeLeftMouseDragged,197,1556);send(NSEventTypeLeftMouseUp,197,1556);
 auto moved=mockup::routeChain(routeValue);checkGui(moved[0]==original[4]&&moved[1]==original[0],"Routing inserts before first");
 send(NSEventTypeLeftMouseDown,329,1556);send(NSEventTypeLeftMouseDragged,1597,1556);send(NSEventTypeLeftMouseUp,1597,1556);
 checkGui(mockup::routeChain(routeValue)==original,"Routing inserts after last");
 double beforeCancel=c->getParamNormalized(kRoutingOrder);
 send(NSEventTypeLeftMouseDown,329,1556);send(NSEventTypeLeftMouseDragged,100,100);send(NSEventTypeLeftMouseUp,100,100);
 checkGui(c->getParamNormalized(kRoutingOrder)==beforeCancel,"Routing outside drop cancels");
 ViewRect resized(0,0,816,759);checkGui(view->checkSizeConstraint(&resized)==kResultOk,"Resize constraints");view->onSize(&resized);checkGui([(id<GrainsGuiInspection>)surface controlsFit],"Resize keeps controls visible");
 [surface display];NSBitmapImageRep* bitmap=[surface bitmapImageRepForCachingDisplayInRect:surface.bounds];[surface cacheDisplayInRect:surface.bounds toBitmapImageRep:bitmap];
 NSString* path=argc>1?[NSString stringWithUTF8String:argv[1]]:@"GrainsDosage-GUI.png";
 checkGui([[bitmap representationUsingType:NSBitmapImageFileTypePNG properties:@{}] writeToFile:path atomically:YES],"Native screenshot");
 view->removed();view->release();c->terminate();c->release();[window close];std::cout<<"PASS: native skin, tabs, controls, step editing, XY, resize and screenshot\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}}
