#import <Cocoa/Cocoa.h>
#include "plugin.cpp"
#include "mockup_ui.h"
#include "gui_host_guard.h"
#include "gui_scenarios.h"
#include <iostream>
@protocol GrainsGuiInspection
- (BOOL)skinLoaded;
- (BOOL)controlsFit;
- (NSUInteger)staticBuilds;
- (NSUInteger)controlPatches;
- (void)setFullRedrawForInspection:(BOOL)enabled;
- (int)page;
- (double)pageScroll;
- (void)setPageScroll:(double)value;
@end
using namespace aztec;
static void checkGui(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
int main(int argc,char** argv){@autoreleasepool {try{
 [NSApplication sharedApplication];[NSApp setActivationPolicy:NSApplicationActivationPolicyAccessory];
 auto* c=new Controller;checkGui(c->initialize(nullptr)==kResultOk,"Controller init");aztec::GuiHostGuard hostGuard;c->setComponentHandler(&hostGuard);
 auto* view=c->createView(ViewType::kEditor);ViewRect size;view->getSize(&size);
 NSWindow* window=[[NSWindow alloc] initWithContentRect:NSMakeRect(0,0,size.getWidth(),size.getHeight()) styleMask:NSWindowStyleMaskTitled backing:NSBackingStoreBuffered defer:NO];window.releasedWhenClosed=NO;
 checkGui(view->attached((__bridge void*)window.contentView,kPlatformTypeNSView)==kResultOk,"Attach");NSView* surface=window.contentView.subviews.firstObject;id<GrainsGuiInspection> inspection=(id<GrainsGuiInspection>)surface;checkGui([inspection skinLoaded],"Current rack skin loaded");
 auto send=[&](int action,double x,double y,int count=1){mockup::Viewport fit(surface.bounds.size.width,surface.bounds.size.height);NSPoint point=[surface convertPoint:NSMakePoint(fit.x+x*fit.scale,fit.y+y*fit.scale) toView:nil];NSEventType type=action==0?NSEventTypeLeftMouseDown:action==1?NSEventTypeLeftMouseDragged:NSEventTypeLeftMouseUp;NSEvent* e=[NSEvent mouseEventWithType:type location:point modifierFlags:0 timestamp:0 windowNumber:window.windowNumber context:nil eventNumber:1 clickCount:count pressure:1];if(action==0)[surface mouseDown:e];else if(action==1)[surface mouseDragged:e];else [surface mouseUp:e];};
 auto click=[&](double x,double y,int count){send(0,x,y,count);send(2,x,y,count);};auto drag=[&](int a,double x,double y){send(a,x,y);};
 aztec::runGuiScenarios(c,click,drag,[&](double v){[inspection setPageScroll:v];},[&](){return [inspection page];});checkGui([inspection controlsFit],"Controls retain valid dimensions");
 // A stale Cubase size still uses one uniform draw/input transform.
 ViewRect stale(0,0,900,600);view->onSize(&stale);for(int p=0;p<3;++p){auto r=mockup::pageRect(p);click(r.x+r.w/2,r.y+r.h/2,1);checkGui([inspection page]==p,"Stale host size hit alignment");}
 ViewRect benchmarkSize(0,0,int(mockup::width*.7),int(mockup::height*.7));view->onSize(&benchmarkSize);auto nav=mockup::pageRect(1);click(nav.x+80,nav.y+20,1);[inspection setPageScroll:0];
 NSBitmapImageRep* bitmap=[surface bitmapImageRepForCachingDisplayInRect:surface.bounds];auto paint=[&](){[surface cacheDisplayInRect:surface.bounds toBitmapImageRep:bitmap];};paint();NSUInteger builds=[inspection staticBuilds],patches=[inspection controlPatches];
 auto benchmark=[&](bool full){[inspection setFullRedrawForInspection:full?YES:NO];if(!full)paint();auto start=std::chrono::steady_clock::now();for(int i=0;i<12;++i){c->setParamNormalized(kUiLfoPhase0+2,i/12.);paint();}return std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count()/12;};
 // Monitoring, hidden automation and visible control drags never rebuild the full scene.
 auto start=std::chrono::steady_clock::now();for(int i=0;i<12;++i){c->setParamNormalized(kUiLfoPhase0+2,i/12.);paint();}double cachedMs=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count()/12;
 checkGui([inspection staticBuilds]==builds,"Monitor frames reuse artwork");c->setParamNormalized(kGrainMix,.213);paint();checkGui([inspection staticBuilds]==builds,"Hidden module automation skips redraw");start=std::chrono::steady_clock::now();for(int i=0;i<12;++i){c->setParamNormalized(kFilterCutoff,.1+i*.04);paint();}double dragMs=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count()/12;checkGui([inspection staticBuilds]==builds&&[inspection controlPatches]>=patches+12,"Visible knob automation patches its own control");double fullMs=benchmark(true);[inspection setFullRedrawForInspection:NO];
 std::cout<<"GUI paint benchmark: cached="<<cachedMs<<" ms/frame; knob edits="<<dragMs<<" ms/frame; full="<<fullMs<<" ms/frame\n";
 ViewRect resized(0,0,816,759);checkGui(view->checkSizeConstraint(&resized)==kResultOk,"Resize constraints");view->onSize(&resized);
 NSString* base=argc>1?[NSString stringWithUTF8String:argv[1]]:@"GrainsDosage-GUI.png";
 auto screenshot=[&](NSString* suffix){NSBitmapImageRep* shot=[surface bitmapImageRepForCachingDisplayInRect:surface.bounds];[surface cacheDisplayInRect:surface.bounds toBitmapImageRep:shot];NSString* path=[base.stringByDeletingPathExtension stringByAppendingFormat:@"%@.png",suffix];checkGui([[shot representationUsingType:NSBitmapImageFileTypePNG properties:@{}] writeToFile:path atomically:YES],"Native screenshot");};
 for(int p=0;p<3;++p){auto r=mockup::pageRect(p);click(r.x+80,r.y+20,1);[inspection setPageScroll:0];screenshot([NSString stringWithFormat:@"-page-%d",p]);if(p<2){mockup::RackState rack;rack.page=p;[inspection setPageScroll:mockup::maxScroll(rack)];screenshot([NSString stringWithFormat:@"-page-%d-bottom",p]);}}
 auto r=mockup::pageRect(0);click(r.x+80,r.y+20,1);auto val=[&](ParamID id){return c->getParamNormalized(id);};for(int stage=0;stage<5;++stage){mockup::RackState rack;double offset=std::clamp(mockup::moduleRect(stage,val,rack).y-220,0.,mockup::maxScroll(rack));[inspection setPageScroll:offset];screenshot([NSString stringWithFormat:@"-module-%d",stage]);}
 checkGui(hostGuard.rejectedTabEdits==0,"UI navigation never edits host read-only parameters");view->removed();view->release();c->setComponentHandler(nullptr);c->terminate();c->release();[window close];std::cout<<"PASS: native skin, three pages, vertical routing, scroll, Mix locks, sequencers, XY, resize and cached rendering\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}}
