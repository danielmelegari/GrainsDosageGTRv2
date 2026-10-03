#import <Cocoa/Cocoa.h>
#include "editor.h"
#include "parameters.h"
#include "skin_spec.h"
#include "skin_theme.h"
#include "filter_sequencer.h"
#include "randomize.h"
#include "preset_io.h"
#include "factory_presets.h"
#include "public.sdk/source/common/pluginview.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <string>
#include <vector>

@class GrainsSurface;
namespace aztec {
using namespace Steinberg;using namespace Steinberg::Vst;
constexpr double canvasW=1320.,canvasH=1360.;
enum Kind {Knob,Slider,Toggle,Select,Pad,Pan,PanMode};
struct Control {ParamID id;NSRect rect;Kind kind;std::string label;};
class Editor final:public CPluginView {
  EditController* controller_;
  __strong GrainsSurface* surface_=nil;
public:
  explicit Editor(EditController* c);
  ~Editor() override;
  tresult PLUGIN_API isPlatformTypeSupported(FIDString type) override;
  tresult PLUGIN_API attached(void* parent,FIDString type) override;
  tresult PLUGIN_API removed() override;
  tresult PLUGIN_API onSize(ViewRect* size) override;
  tresult PLUGIN_API canResize() override {return kResultTrue;}
  tresult PLUGIN_API checkSizeConstraint(ViewRect* size) override;
  double value(ParamID id) const {return controller_->getParamNormalized(id);}
  ParameterInfo info(ParamID id) const {auto* p=controller_->getParameterObject(id);return p?p->getInfo():ParameterInfo{};}
  NSString* display(ParamID id,double value,bool units=true) const;
  void begin(ParamID id) {controller_->beginEdit(id);}
  void end(ParamID id) {controller_->endEdit(id);}
  void change(ParamID id,double v) {
    v=std::clamp(v,0.,1.);int n=info(id).stepCount;if(n>0)v=std::round(v*n)/n;
    controller_->setParamNormalized(id,v);controller_->performEdit(id,v);
  }
  void edit(ParamID id,double v) {begin(id);change(id,v);end(id);}
  void zoom(double scale);
};
}
static NSColor* rgb(double r,double g,double b,double a=1.) {return [NSColor colorWithSRGBRed:r green:g blue:b alpha:a];}
// Astral/Filigree Green: resolve a shared skin colour to a native NSColor.
static NSColor* C(aztec::skin::Rgb k,double a=1.) {return rgb(k.r/255.,k.g/255.,k.b/255.,a);}
// Resolve a colour ROLE against the live (theme-aware) palette.
static NSColor* C(int role,double a=1.) {return C(aztec::skin::active().at(role),a);}
// Theme-aware reference: map the colour ROLE (kAccent, kPanel, ...) through
// the live palette and return an NSColor. Passing the enum constant directly
// would resolve to the static Rgb in namespace skin, not the active theme.
#define AZSKIN(k) C(int(aztec::skin::k))  // NSColor from live palette role
#define AZCOL(k) aztec::skin::C(aztec::skin::k)  // raw Rgb of a role (live)
static NSColor* green() {return AZSKIN(kAccent);}
static NSColor* cream() {return AZSKIN(kCream);}
static NSColor* muted() {return AZSKIN(kMuted);}
static NSColor* dark() {return AZSKIN(kBgDeep);}
static NSColor* onText() {return AZSKIN(kOnText);}   // dark text for lit green fills
static void box(NSRect r,NSColor* fill,NSColor* stroke,double radius=8.) {
  NSBezierPath* p=[NSBezierPath bezierPathWithRoundedRect:r xRadius:radius yRadius:radius];
  [fill setFill];[p fill];
  if(radius>=5. && r.size.height>=20. && fill.alphaComponent>.9){
    NSColor* top=[fill blendedColorWithFraction:.12 ofColor:AZSKIN(kFiligree)];
    NSGradient* sheen=[[NSGradient alloc] initWithStartingColor:top endingColor:fill];
    [sheen drawInBezierPath:p angle:90.];
  }
  if(stroke){[stroke setStroke];p.lineWidth=1.;[p stroke];}
}
static void label(NSString* s,NSRect r,double size,NSColor* color,bool center=false,double minimumSize=14.) {
  NSMutableParagraphStyle* p=[[NSMutableParagraphStyle alloc] init];p.alignment=center?NSTextAlignmentCenter:NSTextAlignmentLeft;p.lineBreakMode=NSLineBreakByTruncatingTail;
  // At the default 60% editor scale an 11-point canvas label would become
  // unreadable. Keep the smallest labels at 14 canvas points (about 8.4pt on
  // screen) and use a semibold face with the high-contrast palette.
  double extra=std::max(0.,minimumSize-size);r=NSInsetRect(r,0,-extra*.5);
  [s drawInRect:r withAttributes:@{NSFontAttributeName:[NSFont systemFontOfSize:std::max(minimumSize,size) weight:NSFontWeightSemibold],NSForegroundColorAttributeName:color,NSParagraphStyleAttributeName:p}];
}
static void arc(double cx,double cy,double radius,double from,double to,NSColor* color,double width=3.) {
  NSBezierPath* p=[NSBezierPath bezierPath];
  for(int i=0;i<=80;++i){double a=(from+(to-from)*i/80.)*qg::tau/360.;NSPoint pt=NSMakePoint(cx+radius*std::cos(a),cy+radius*std::sin(a));if(i==0)[p moveToPoint:pt];else[p lineToPoint:pt];}
  [color setStroke];p.lineWidth=width;p.lineCapStyle=NSRoundLineCapStyle;[p stroke];
}
static bool bipolar(aztec::ParamID id) {
  using namespace aztec;
  return (id>=kExtraRoutes0&&id<kGlitchMove)||id==kPitch||id==kTranspose||id==kXAmount||id==kYAmount||
    (id>=kRepeatPitch0&&id<kStretchOn)||
    (id>=kLfo0&&id<kLegacyCount&&int(id-kLfo0)%lStride>=lRoute0);
}
static const char* names[3]={"GRANULIZER","PRESLICER","BEAT REPEATER"};
static constexpr double slotX[3]={16.,452.,888.};

@interface GrainsSurface:NSView {
@public
  aztec::Editor* owner;
@private
  NSImage* artwork;
  NSMutableDictionary<NSString*,NSImage*>* sprites;
  NSString* presetName;
  std::vector<aztec::Control> controls;
  std::array<double,aztec::kCount> cached;
  int selectedGate,selectedReslice;
  int selectedLfo,selectedRepeat,dragID,dragSlot,dropSlot;
  bool dragXY;
  aztec::Kind dragKind;
  NSRect dragRect;
  NSPoint origin;
  double starting;
  NSTimer* timer;
  uint32_t randomSeed;
}
- (BOOL)skinLoaded;
- (void)preset:(BOOL)save;
- (void)stepPreset:(int)direction;
- (NSURL*)presetFolder;
- (BOOL)loadPresetURL:(NSURL*)url;
- (void)presetMenu:(NSEvent*)event;
- (void)skinMenu:(NSEvent*)event;
- (void)chooseSkin:(NSMenuItem*)item;
- (void)choosePreset:(NSMenuItem*)item;
- (BOOL)controlsFit;
- (void)stop;
- (void)tick:(NSTimer*)tick;
- (void)choose:(NSMenuItem*)item;
- (void)chooseZoom:(NSMenuItem*)item;
@end
@implementation GrainsSurface
- (instancetype)initWithFrame:(NSRect)frame {
  self=[super initWithFrame:frame];if(self){
    presetName=@"PRESETS ▾";
    selectedGate=selectedReslice=0;
    randomSeed=arc4random()|1;selectedLfo=selectedRepeat=0;dragID=dragSlot=dropSlot=-1;dragXY=false;cached.fill(-1.);
    NSString* path=[[NSBundle bundleForClass:[GrainsSurface class]] pathForResource:@"GrainsDosage-skin" ofType:@"png"];
    if(!path)path=@"assets/GrainsDosage-skin.png";
    if(![[NSFileManager defaultManager] fileExistsAtPath:path])path=@"GrainsDosage/assets/GrainsDosage-skin.png";
    artwork=[[NSImage alloc] initWithContentsOfFile:path];if(artwork)artwork.size=NSMakeSize(2048,1520);
    sprites=[NSMutableDictionary dictionary];
    NSArray<NSString*>* spriteNames=@[@"knob",@"slider",@"step",@"header-left",@"header-right",@"xy-nebula"];
    NSArray<NSString*>* spriteKeys=@[@"385,260,76,76",@"453,636,24,32",@"734,456,54,48",@"1850,12,136,104",@"1726,15,95,104",@"nebula"];
    for(NSUInteger i=0;i<spriteNames.count;++i){NSString* file=[[NSBundle bundleForClass:[GrainsSurface class]] pathForResource:spriteNames[i] ofType:@"png"];if(!file)file=[@"assets/sprites/" stringByAppendingFormat:@"%@.png",spriteNames[i]];if(![[NSFileManager defaultManager] fileExistsAtPath:file])file=[@"GrainsDosage/" stringByAppendingString:file];NSImage* sprite=[[NSImage alloc] initWithContentsOfFile:file];if(sprite)sprites[spriteKeys[i]]=sprite;}
    self.toolTip=@"Drag a knob vertically; Shift gives fine control. Double-click resets. Drag module headers to change audio order.";
    timer=[NSTimer timerWithTimeInterval:1./30. target:self selector:@selector(tick:) userInfo:nil repeats:YES];
    [[NSRunLoop mainRunLoop] addTimer:timer forMode:NSRunLoopCommonModes];
  }return self;
}
- (BOOL)isFlipped{return YES;}
- (BOOL)isOpaque{return YES;}
- (BOOL)acceptsFirstResponder{return YES;}
- (BOOL)skinLoaded{return artwork!=nil;}
- (void)stop {
  if(owner&&dragID>=0)owner->end(aztec::ParamID(dragID));
  if(owner&&dragXY){owner->end(aztec::kXYX);owner->end(aztec::kXYY);}
  dragID=dragSlot=dropSlot=-1;dragXY=false;[timer invalidate];timer=nil;
}
- (void)tick:(NSTimer*)tick {
  (void)tick;if(!owner)return;bool changed=false;
  for(int id=0;id<aztec::kCount;++id){double v=owner->value(id);if(v!=cached[id]){cached[id]=v;changed=true;}}
  if(changed)[self setNeedsDisplay:YES];
}
- (NSPoint)logical:(NSEvent*)event {
  NSPoint p=[self convertPoint:event.locationInWindow fromView:nil];return NSMakePoint(p.x*aztec::canvasW/self.bounds.size.width,p.y*aztec::canvasH/self.bounds.size.height);
}
- (int)order {return std::clamp(int(std::round(owner->value(aztec::kModuleOrder)*6.)),0,6);}
- (int)stage:(int)slot {int order=[self order];return aztec::moduleOrders[order>=6?0:order][slot];}
- (int)slot:(int)stage {for(int i=0;i<3;++i)if([self stage:i]==stage)return i;return 0;}
- (void)add:(aztec::ParamID)id x:(double)x y:(double)y w:(double)w h:(double)h kind:(aztec::Kind)kind label:(const char*)name {
  controls.push_back({id,NSMakeRect(x,y,w,h),kind,name});
}
- (void)layoutControls {
  using namespace aztec;controls.clear();
#define ADD(ID,X,Y,W,H,K,L) [self add:(ID) x:(X) y:(Y) w:(W) h:(H) kind:(K) label:(L)]
  auto slot=[&](int stage){return [self slot:stage];};
  auto value=[&](ParamID id){return owner->value(id);};
#include "editor_layout_win.inl"
#undef ADD
}
- (BOOL)controlsFit {
  [self layoutControls];NSRect all=NSMakeRect(0,0,aztec::canvasW,aztec::canvasH);
  for(const auto& c:controls)if(!NSContainsRect(all,c.rect)){NSLog(@"Control outside canvas: id=%u label=%s rect=%@ canvas=%@",unsigned(c.id),c.label.c_str(),NSStringFromRect(c.rect),NSStringFromRect(all));return NO;}return YES;
}
- (void)art:(NSRect)source in:(NSRect)dest opacity:(double)opacity {
  NSString* key=[NSString stringWithFormat:@"%.0f,%.0f,%.0f,%.0f",source.origin.x,source.origin.y,source.size.width,source.size.height];NSImage* sprite=sprites[key];
  if(sprite){[NSGraphicsContext currentContext].imageInterpolation=NSImageInterpolationHigh;[sprite drawInRect:dest fromRect:NSMakeRect(0,0,sprite.size.width,sprite.size.height) operation:NSCompositingOperationSourceOver fraction:opacity respectFlipped:YES hints:nil];return;}
  if(!artwork)return;source.origin.y=artwork.size.height-source.origin.y-source.size.height;
  [artwork drawInRect:dest fromRect:source operation:NSCompositingOperationSourceOver fraction:opacity respectFlipped:YES hints:nil];
}
- (void)panel:(NSRect)r {
  box(r,AZSKIN(kPanel),AZSKIN(kFiligree),12.);
  box(NSInsetRect(r,3,3),[NSColor clearColor],AZSKIN(kBorderDark),10.);
  // Vector edge filigree stays in the border, clear of labels and hit areas.
  for(int side=0;side<2;++side){double x=side?NSMaxX(r)-5:r.origin.x+5;
    NSBezierPath* vine=[NSBezierPath bezierPath];
    [vine moveToPoint:NSMakePoint(x,r.origin.y+14)];
    [vine curveToPoint:NSMakePoint(x,NSMaxY(r)-14)
      controlPoint1:NSMakePoint(x+(side?-3:3),r.origin.y+r.size.height*.33)
      controlPoint2:NSMakePoint(x+(side?3:-3),r.origin.y+r.size.height*.66)];
    [AZSKIN(kFiligreeDim) setStroke];vine.lineWidth=1.5;[vine stroke];
  }
  box(NSMakeRect(r.origin.x+15,r.origin.y+5,r.size.width-30,1),AZSKIN(kFiligree),nil,0);
}
- (void)drawControl:(const aztec::Control&)c {
  using namespace aztec;NSRect r=c.rect;double v=std::clamp(owner->value(c.id),0.,1.);
  NSString* title=[NSString stringWithUTF8String:c.label.c_str()];
  if(c.kind==PanMode){const char* modes[]={"MANUAL","ALTERNATE","RANDOM"};int mode=int(std::round(v*2.));for(int i=0;i<3;++i){NSRect b=NSMakeRect(r.origin.x+i*r.size.width/3.,r.origin.y,r.size.width/3.-3,r.size.height);box(b,mode==i?AZSKIN(kAccentGlow):dark(),mode==i?green():muted(),4);label([NSString stringWithUTF8String:modes[i]],NSInsetRect(b,2,6),9,mode==i?onText():cream(),true);}return;}
  if(c.kind==Pan){label(@"L",NSMakeRect(r.origin.x,r.origin.y,16,16),10,cream());label(@"C",NSMakeRect(NSMidX(r)-8,r.origin.y,16,16),10,cream(),true);label(@"R",NSMakeRect(NSMaxX(r)-16,r.origin.y,16,16),10,cream());box(NSMakeRect(r.origin.x+4,r.origin.y+23,r.size.width-8,3),muted(),nil,1);box(NSMakeRect(r.origin.x+v*(r.size.width-8),r.origin.y+18,8,13),green(),nil,3);return;}
  if(c.kind==Knob){
    label(title,NSMakeRect(r.origin.x,r.origin.y,r.size.width,16),11,cream(),true);
    const double cx=NSMidX(r),cy=r.origin.y+43.,radius=24.;
    box(NSMakeRect(cx-radius-2,cy-radius+2,radius*2+4,radius*2+4),AZSKIN(kWell),nil,radius+2);
    NSBezierPath* cap=[NSBezierPath bezierPathWithOvalInRect:NSMakeRect(cx-radius,cy-radius,radius*2,radius*2)];
    // Knob metal + ring groove resolve against the live palette so every
    // theme (and the persisted one) recolours the caps too.
    skin::Rgb capTopS=skin::scale(AZCOL(kPanelRaised),1.15),capBotS=AZCOL(kWell);
    NSColor* capTop=rgb(capTopS.r/255.,capTopS.g/255.,capTopS.b/255.);
    NSColor* capBot=rgb(capBotS.r/255.,capBotS.g/255.,capBotS.b/255.);
    NSGradient* metal=[[NSGradient alloc] initWithStartingColor:capTop endingColor:capBot];
    [metal drawInBezierPath:cap angle:90.];[AZSKIN(kFiligreeDim) setStroke];cap.lineWidth=1.;[cap stroke];
    arc(cx,cy,radius-3,155,335,AZSKIN(kBorderDark),1.);
    skin::Rgb grooveS=skin::mix(AZCOL(kBorderDark),AZCOL(kViolet),.5);
    arc(cx,cy,radius+4,135,405,rgb(grooveS.r/255.,grooveS.g/255.,grooveS.b/255.),3.);
    for(int j=0;j<21;++j){double a=(135.+270.*j/20.)*qg::tau/360.;box(NSMakeRect(cx+33*std::cos(a)-1.5,cy+33*std::sin(a)-1.5,3,3),j/20.<=v?green():AZSKIN(kAccentTrack),nil,1.5);}
    double zero=bipolar(c.id)?270.:135.;double angle=135.+270.*v;
    arc(cx,cy,radius+4,std::min(zero,angle),std::max(zero,angle),green(),3.5);
    double a=angle*qg::tau/360.;NSBezierPath* pointer=[NSBezierPath bezierPath];
    [pointer moveToPoint:NSMakePoint(cx+8*std::cos(a),cy+8*std::sin(a))];
    [pointer lineToPoint:NSMakePoint(cx+22*std::cos(a),cy+22*std::sin(a))];
    [cream() setStroke];pointer.lineWidth=3.;pointer.lineCapStyle=NSRoundLineCapStyle;[pointer stroke];
    box(NSMakeRect(cx+28*std::cos(a)-2.5,cy+28*std::sin(a)-2.5,5,5),cream(),nil,2.5);
    box(NSMakeRect(r.origin.x+4,r.origin.y+74,r.size.width-8,19),dark(),AZSKIN(kHairline),4);
    label(owner->display(c.id,v),NSMakeRect(r.origin.x+5,r.origin.y+76,r.size.width-10,16),12,AZSKIN(kReadout),true);
  }else if(c.kind==Slider){
    label(title,NSMakeRect(r.origin.x,r.origin.y,r.size.width,13),9,muted());
    double yy=r.origin.y+20.,xx=r.origin.x+4.,width=r.size.width-8.;
    box(NSMakeRect(xx,yy,width,4),AZSKIN(kHairline),nil,2);
    double start=bipolar(c.id)?.5:0.;
    box(NSMakeRect(xx+std::min(v,start)*width,yy,std::max(1.,std::abs(v-start)*width),4),green(),nil,2);
    [self art:NSMakeRect(453,636,24,32) in:NSMakeRect(xx+v*width-6,yy-7,12,16) opacity:1.];
    if(r.size.height>=33)label(owner->display(c.id,v),NSMakeRect(r.origin.x,r.origin.y+28,r.size.width,12),10,cream(),true);
    else label(owner->display(c.id,v),NSMakeRect(r.origin.x+65,r.origin.y,r.size.width-65,13),9,cream(),true);
  }else if(c.kind==Select){
    box(r,dark(),AZSKIN(kBorderDark),6);
    bool stacked=r.size.height>=38;
    if(stacked)label(title,NSMakeRect(r.origin.x+8,r.origin.y+4,r.size.width-24,11),8,muted());
    double shown=v;if(c.id==lfoID(selectedLfo,lWave)&&owner->value(kModWaveRnd0+selectedLfo)>0.)shown=owner->value(kUiModWave0+selectedLfo);
    NSString* value=owner->display(c.id,shown,c.id>=kRandomSteps0&&c.id<kDensityFlow);
    label(value,NSMakeRect(r.origin.x+8,r.origin.y+(stacked?18:9),r.size.width-29,18),11,cream());
    label(@"▾",NSMakeRect(NSMaxX(r)-21,r.origin.y+(stacked?17:8),16,18),12,green());
  }else if(c.kind==Toggle){
    bool on=v>=.5;if(c.id>=kGrainEnabled&&c.id<=kRepeatEnabled)title=on?@"ON":@"OFF";box(r,on?AZSKIN(kAccentGlow):AZSKIN(kPanelRaised),on?green():AZSKIN(kFiligreeDim),6);
    if(on)box(NSMakeRect(r.origin.x+6,NSMidY(r)-6,12,12),AZSKIN(kAccentBright),nil,6);
    box(NSMakeRect(r.origin.x+9,NSMidY(r)-3,6,6),on?green():muted(),nil,3);
    label(title,NSMakeRect(r.origin.x+20,r.origin.y+(r.size.height-14)/2.,r.size.width-25,16),10,on?onText():muted(),true);
  }else{
    bool on=v>=.5;box(r,on?AZSKIN(kAccentGlow):AZSKIN(kPanelRaised),on?green():AZSKIN(kFiligreeDim),5);
    int step=c.id>=kReverbStep0?int(c.id-kReverbStep0)+1:c.id>=kGlitchStep0?int(c.id-kGlitchStep0)+1:int(c.id-kStep0)+1;
    if(step==1+int(std::round(owner->value(c.id>=kReverbStep0?kUiReverb:kUiStep)*15.)))box(NSInsetRect(r,1,1),[NSColor clearColor],cream(),4);
    [self art:NSMakeRect(734,456,54,48) in:NSMakeRect(NSMidX(r)-10,r.origin.y+1,20,18) opacity:(on?1.:.35)];
    label([NSString stringWithFormat:@"%d",step],NSMakeRect(r.origin.x,NSMaxY(r)-15,r.size.width,14),10,on?onText():muted(),true);
  }
}
- (void)drawRect:(NSRect)dirty {
  (void)dirty;[dark() setFill];NSRectFill(self.bounds);if(!owner)return;[self layoutControls];
  [NSGraphicsContext saveGraphicsState];NSAffineTransform* t=[NSAffineTransform transform];
  [t scaleXBy:self.bounds.size.width/aztec::canvasW yBy:self.bounds.size.height/aztec::canvasH];[t concat];
  [self panel:NSMakeRect(8,8,1304,73)];
  label(@"GRAINS",NSMakeRect(29,23,165,44),31,AZSKIN(kTitle));label(@"DOSAGE",NSMakeRect(196,23,194,44),31,AZSKIN(kTitle));
  box(NSMakeRect(420,22,462,30),dark(),green(),5);label(@"<",NSMakeRect(420,30,24,18),13,cream(),true);label(@">",NSMakeRect(858,30,24,18),13,cream(),true);label(presetName,NSMakeRect(448,30,406,18),12,cream(),true);
  box(NSMakeRect(420,56,227,28),AZSKIN(kPanelRaised),AZSKIN(kFiligree),5);label(@"LOAD",NSMakeRect(420,64,227,16),10,cream(),true);
  box(NSMakeRect(653,56,229,28),AZSKIN(kPanelRaised),AZSKIN(kFiligree),5);label(@"SAVE",NSMakeRect(653,64,229,16),10,cream(),true);
  for(int slot=0;slot<3;++slot){
    int stage=[self stage:slot];double x=slotX[slot];
    [self panel:NSMakeRect(x,98,416,404)];
    
    label([NSString stringWithFormat:@"↔  %d   %s",slot+1,names[stage]],NSMakeRect(x+15,110,210,25),14,muted());
    box(NSMakeRect(x+316,106,87,28),AZSKIN(kPanelRaised),AZSKIN(kFiligree),6);
    label(@"RANDOM",NSMakeRect(x+319,114,81,18),10,cream(),true);
    bool active=stage==0?(owner->value(aztec::kGrainEnabled)>.5&&owner->value(aztec::kGrainMix)>.001):owner->value(stage==1?aztec::kUiGlitch:aztec::kUiRepeat)>.5;
    for(int j=0;j<12;++j)box(NSMakeRect(x+19+j*32,486,23,4),active?green():AZSKIN(kAccentTrack),nil,2);
    if(stage==0){
      box(NSMakeRect(x+208,248,196,147),AZSKIN(kPanelInset),AZSKIN(kFiligreeDim),8);
      label(@"MASTER OPTIONS",NSMakeRect(x+220,252,174,17),10,muted(),true);
      if(owner->value(aztec::kPanMode)>=.25)label(owner->value(aztec::kPanMode)<.75?@"PAN: ALTERNATE L / R":@"PAN: RANDOM L / R",NSMakeRect(x+170,405,228,16),10,green(),true);
      NSRect screen=NSMakeRect(x+14,432,388,48);box(screen,dark(),AZSKIN(kHairline),4);
      double left=owner->value(aztec::kUiGrainStart),right=owner->value(aztec::kUiGrainEnd),head=owner->value(aztec::kUiGrainHead);bool active=owner->value(aztec::kUiGrainActive)>.5;
      double wavePeak=.02;for(int b=0;b<128;++b)wavePeak=std::max(wavePeak,owner->value(aztec::kUiWave0+b));
      for(int b=0;b<128;++b){double u=(b+.5)/128.,h=std::max(1.,owner->value(aztec::kUiWave0+b)/wavePeak*30.);bool lit=active&&(b+1.)/128.>=left&&b/128.<=right;box(NSMakeRect(x+18+b*3.,446+(30.-h)*.5,2,h),lit?green():AZSKIN(kFaint),nil,0);}
      if(active)box(NSMakeRect(x+18+head*384.,446,1,30),cream(),nil,0);
      label([NSString stringWithFormat:@"GRAIN FOLLOW   ·   %.1f s",owner->value(aztec::kUiWaveSeconds)*16.],NSMakeRect(x+22,433,360,11),9,muted());
    }
    if(dragSlot>=0&&dropSlot==slot)box(NSMakeRect(x+2,100,412,300),[NSColor clearColor],green(),11);
    if(stage==1){label(@"RANDOM TRIGGER",NSMakeRect(x+20,261,350,20),12,muted());
      label(@"CHANCE = probability at each interval",NSMakeRect(x+20,354,375,24),10,muted());
      label(@"SLICE = cut length  /  burst = up to 2 cuts",NSMakeRect(x+20,380,375,24),10,muted());}
    if(stage==2){
      label([NSString stringWithFormat:@"16 STEPS  ·  SELECTED %02d",selectedRepeat+1],NSMakeRect(x+20,249,365,16),10,muted());
      for(int i=0;i<16;++i){
        NSRect r=NSMakeRect(x+19+(i%8)*48,271+(i/8)*39,43,32);bool enabled=owner->value(aztec::kRepeatStep0+i)>=.5;
        const bool selected=i==selectedRepeat;const double press=selected?1.:0.;
        const double a=r.origin.x,b=r.origin.y;
        // Raised cap, dark lower edge and a narrow top highlight, inside the hit area.
        box(NSMakeRect(a,b+3,43,29),AZSKIN(kWell),AZSKIN(kBorderDark),5);
        box(NSMakeRect(a,b+press,43,28),enabled?AZSKIN(kAccentGlow):AZSKIN(kPanelRaised),selected?cream():(enabled?AZSKIN(kFiligreeDim):AZSKIN(kFiligreeDim)),5);
        box(NSMakeRect(a+4,b+2+press,35,1),enabled?AZSKIN(kFiligree):AZSKIN(kFiligreeDim),nil,0);
        box(NSMakeRect(a+2,b+5+press,1,19),AZSKIN(kFiligreeDim),nil,0);
        box(NSMakeRect(a+3,b+26+press,37,1),AZSKIN(kWell),nil,0);
        label([NSString stringWithFormat:@"%d",i+1],NSMakeRect(a+3,b+2+press,37,13),11,enabled?onText():muted(),true,11);
        label(owner->display(aztec::kRepeatRate0+i,owner->value(aztec::kRepeatRate0+i),false),NSMakeRect(a+3,b+16+press,37,10),9,enabled?onText():muted(),true,9);
        if(i==int(std::round(owner->value(aztec::kUiStep)*15.)))box(NSMakeRect(a+5,b+30,33,2),green(),nil,1);
      }
    }
  }
  for(int i=0;i<2;++i)label(@"›",NSMakeRect(433+i*436,222,17,30),24,green(),true);
  [self panel:NSMakeRect(16,516,836,236)];
  label(@"MODULATION",NSMakeRect(32,533,129,21),13,cream());
  for(int i=0;i<4;++i){
    NSRect tab=NSMakeRect(171+i*133,528,120,28);bool selected=i==selectedLfo;
    box(tab,selected?AZSKIN(kPanelRaised):AZSKIN(kPanel),selected?AZSKIN(kFiligree):AZSKIN(kFiligreeDim),6);
    label([NSString stringWithFormat:@"Mod %d  %@",i+1,owner->value(aztec::lfoID(i,aztec::lEnabled))>=.5?@"●":@"○"],NSMakeRect(tab.origin.x+7,tab.origin.y+7,106,18),11,selected?cream():muted(),true);
  }
  NSRect scope=NSMakeRect(32,613,220,64);box(scope,dark(),AZSKIN(kHairline),6);
  double cycle=std::round(owner->value(aztec::kUiLfoCycle0+selectedLfo)*4294967295.-2147483648.);
  int64_t epoch=int64_t(std::round(owner->value(aztec::kUiLfoEpoch0+selectedLfo)*4294967295.-2147483648.));
  qg::Lfo preview;preview.prepare(0x13579BDFULL+uint64_t(selectedLfo)*104729+(owner->value(aztec::lfoID(selectedLfo,aztec::lReset))>=.5?uint64_t(epoch)*0x9e3779b97f4a7c15ULL:0));
  qg::LfoSettings shape;shape.enabled=true;shape.beats=1.;
  shape.wave=int(std::round(owner->value(owner->value(aztec::kModWaveRnd0+selectedLfo)>0.?aztec::kUiModWave0+selectedLfo:aztec::lfoID(selectedLfo,aztec::lWave))*129.));
  shape.randomSteps=1+int(std::round(owner->value(aztec::kRandomSteps0+selectedLfo)*63.));
  shape.depth=owner->value(aztec::lfoID(selectedLfo,aztec::lDepth));shape.phase=0.;
  shape.glide=.01+.99*owner->value(aztec::lfoID(selectedLfo,aztec::lGlide));
  NSBezierPath* wave=[NSBezierPath bezierPath];for(int i=0;i<=210;++i){double v=preview.process(shape,cycle+double(i)/210.,48000.,false,0);NSPoint pt=NSMakePoint(37+i,640-v*21.);if(i==0)[wave moveToPoint:pt];else[wave lineToPoint:pt];}
  [green() setStroke];wave.lineWidth=1.6;[wave stroke];label(@"WAVE PREVIEW",NSMakeRect(42,664,190,11),8,muted());
  double cursor=37.+210.*owner->value(aztec::kUiLfoPhase0+selectedLfo);box(NSMakeRect(cursor,617,2,44),cream(),nil,0);
  [self panel:NSMakeRect(864,516,440,236)];
  label(@"XY MORPH",NSMakeRect(880,533,270,21),13,cream());
  NSRect pad=NSMakeRect(880,566,168,166);box(pad,dark(),AZSKIN(kHairline),8);
  NSImage* nebula=sprites[@"nebula"];if(nebula)[nebula drawInRect:pad fromRect:NSMakeRect(0,0,nebula.size.width,nebula.size.height) operation:NSCompositingOperationSourceOver fraction:1. respectFlipped:YES hints:nil];
  NSBezierPath* geometry=[NSBezierPath bezierPath];
  for(int i=0;i<8;++i){double a=i*qg::tau/8.;NSPoint pt=NSMakePoint(NSMidX(pad)+72*std::cos(a),NSMidY(pad)+71*std::sin(a));for(int j=i+1;j<8;++j){double b=j*qg::tau/8.;[geometry moveToPoint:pt];[geometry lineToPoint:NSMakePoint(NSMidX(pad)+72*std::cos(b),NSMidY(pad)+71*std::sin(b))];}}
  [[AZSKIN(kViolet) colorWithAlphaComponent:.48] setStroke];geometry.lineWidth=.6;[geometry stroke];
  double px=pad.origin.x+owner->value(aztec::kXYX)*pad.size.width,py=NSMaxY(pad)-owner->value(aztec::kXYY)*pad.size.height;
  NSBezierPath* cross=[NSBezierPath bezierPath];[cross moveToPoint:NSMakePoint(px,pad.origin.y)];[cross lineToPoint:NSMakePoint(px,NSMaxY(pad))];[cross moveToPoint:NSMakePoint(pad.origin.x,py)];[cross lineToPoint:NSMakePoint(NSMaxX(pad),py)];[[AZSKIN(kAccent) colorWithAlphaComponent:.25] setStroke];cross.lineWidth=1.;[cross stroke];
  box(NSMakeRect(px-5,py-5,10,10),green(),cream(),5);
  [self panel:NSMakeRect(16,766,1288,108)];label(@"RESLICE",NSMakeRect(32,781,108,24),14,muted());
  box(NSMakeRect(1052,778,216,28),AZSKIN(kAccentGlow),green(),5);label(@"RANDOM ONCE",NSMakeRect(1052,782,216,20),12,onText(),true);
  for(int i=0;i<16;++i){double x=32+i*78.;bool on=owner->value(aztec::kResliceStep0+i)>.5;box(NSMakeRect(x,823,70,40),on?AZSKIN(kAccentGlow):dark(),selectedReslice==i?cream():muted(),5);label([NSString stringWithFormat:@"%02d → %02d",i+1,1+int(std::round(owner->value(owner->value(aztec::kResliceRndOn)>.5?aztec::kUiResliceSource0+i:aztec::kResliceIndex0+i)*15.))],NSMakeRect(x,827,70,18),10,on?onText():muted(),true);if(i==int(std::round(owner->value(aztec::kUiResliceStep)*15.)))box(NSMakeRect(x+5,858,60,2),owner->value(aztec::kUiResliceActive)>.5?green():cream(),nil,1);}
  [self panel:NSMakeRect(16,886,1288,108)];label(@"GATER",NSMakeRect(32,901,104,24),14,muted());
  label(@"CLICK: WET / OFF     SHIFT-CLICK: LATCH RELEASE",NSMakeRect(760,876,500,16),9,muted(),true);
  for(int i=0;i<16;++i){double x=32+i*78.;int state=owner->value(aztec::kGaterState0+i)>=.25?1:0;if(owner->value(aztec::kGaterEnabled)>.5&&owner->value(aztec::kGaterStepRnd)>.5)state=owner->value(aztec::kUiGaterState0+i)>=.5?1:0;bool release=owner->value(aztec::kGaterRelease0+i)>=.5;double length=owner->value(aztec::kGaterLengthRnd)>.5?owner->value(aztec::kUiGaterLength0+i):.05+.95*owner->value(aztec::kGaterLength0+i);double sustain=owner->value(aztec::kGaterSustain0+i);NSColor* color=release?AZSKIN(kRelease):state?green():muted();
    box(NSMakeRect(x,943,70,40),dark(),selectedGate==i?cream():color,5);
    label([NSString stringWithFormat:@"%02d  %@",i+1,release?@"REL":state?@"WET":owner->value(aztec::kGaterLatch)>.5?@"HOLD":@"OFF"],NSMakeRect(x+2,944,66,17),10,color,true);
    box(NSMakeRect(x+5,965,60,4),AZSKIN(kBorderDark),nil,1);box(NSMakeRect(x+5,965,60*length,4),color,nil,1);
    box(NSMakeRect(x+5,973,60,4),AZSKIN(kBorderDark),nil,1);box(NSMakeRect(x+5,973,60*sustain,4),AZSKIN(kViolet),nil,1);
    // Tie badge removed from the Gater UI.
    if(owner->value(aztec::kGaterEnabled)>.5&&i==int(std::round(owner->value(aztec::kUiGaterStep)*15.)))box(NSMakeRect(x+5,981,60,2),cream(),nil,1);
  }
  // Tie wrap-around badge removed (Tie feature dropped).
  [self panel:NSMakeRect(16,1006,540,136)];
  label(@"FILTER",NSMakeRect(32,1017,100,22),13,cream());
  [self panel:NSMakeRect(568,1006,736,136)];
  label(@"REVERB",NSMakeRect(584,1017,100,22),13,cream());
  if(owner->value(aztec::kReverbSource)>=.5){box(NSMakeRect(592,1110,8,8),owner->value(aztec::kUiReverbGate)>.5?green():muted(),nil,4);label(@"RANDOM IMPULSE  ·  50% CHANCE  ·  TAIL CONTINUES",NSMakeRect(614,1102,650,28),11,cream());}

  [self panel:NSMakeRect(16,1154,1288,128)];
  label(@"FILTER SEQUENCER",NSMakeRect(32,1173,180,22),13,cream());
  if(owner->value(aztec::kFilterSeqMode)>.5)label(@"SAMPLE & GLIDE — smooth random cutoff",NSMakeRect(32,1230,704,24),14,green(),true);
  else for(int i=0;i<32;++i){double x=32+i*22.;double v=qg::filterPattern(int(std::round(owner->value(aztec::kFilterSeqPattern)*63.)),i);bool active=owner->value(aztec::kFilterSeqOn)>.5&&i==int(std::round(owner->value(aztec::kUiFilterSeqStep)*31.));box(NSMakeRect(x,1223,18,37),dark(),active?green():muted(),3);box(NSMakeRect(x+3,1255-25*(v+1)*.5,12,3+25*(v+1)*.5),active?green():AZSKIN(kAccentTrack),nil,1);}
  [self panel:NSMakeRect(16,1294,1288,56)];
  label(@"MASTER",NSMakeRect(32,1312,78,22),13,cream());
  label(@"OUTPUT",NSMakeRect(994,1304,106,16),10,cream());
  double level=owner->value(aztec::kUiLevel);
  for(int j=0;j<24;++j)box(NSMakeRect(994+j*4,1326,2,12),level>j/24.?(j>20?AZSKIN(kHot):AZSKIN(kMeter)):AZSKIN(kMeterOff),nil,1);
  for(const auto& c:controls)[self drawControl:c];
  box(NSMakeRect(982,1312,144,26),dark(),AZSKIN(kFiligreeDim),5);label([NSString stringWithFormat:@"SKIN: %@ ▾",[NSString stringWithUTF8String:aztec::theme::skinLabel().c_str()]],NSMakeRect(990,1318,128,15),10,cream(),true);
  box(NSMakeRect(1140,1312,144,26),dark(),AZSKIN(kFiligreeDim),5);label(@"UI SIZE ▾",NSMakeRect(1148,1318,128,15),10,cream(),true);
  [NSGraphicsContext restoreGraphicsState];
}
- (void)chooseSkin:(NSMenuItem*)item {
  if(item.tag==-2){aztec::theme::loadSkinFile(aztec::theme::defaultPath());}
  else if(item.tag==-1){ // Original GUI: Astral Green, drop any persisted skin.
    aztec::theme::applySkin(0,{});
    std::remove(aztec::theme::defaultPath().c_str());
  }
  else{aztec::theme::applySkin(int(item.tag),{});aztec::theme::saveSkinFile(aztec::theme::defaultPath());}
  [self setNeedsDisplay:YES];
}
- (void)skinMenu:(NSEvent*)event {
  NSMenu* menu=[[NSMenu alloc] initWithTitle:@"Skin"];
  for(int i=0;i<int(aztec::theme::themes().size());++i){NSString* title=[NSString stringWithUTF8String:aztec::theme::themes()[size_t(i)].title];NSMenuItem* item=[[NSMenuItem alloc] initWithTitle:title action:@selector(chooseSkin:) keyEquivalent:@""];item.target=self;item.tag=i;if(i==aztec::theme::currentTheme())item.state=NSControlStateValueOn;[menu addItem:item];}
  [menu addItem:[NSMenuItem separatorItem]];
  NSMenuItem* orig=[[NSMenuItem alloc] initWithTitle:@"ORIGINAL (ASTRAL)" action:@selector(chooseSkin:) keyEquivalent:@""];orig.target=self;orig.tag=-1;if(aztec::theme::currentTheme()==0&&!aztec::theme::customized())orig.state=NSControlStateValueOn;[menu addItem:orig];
  NSMenuItem* reset=[[NSMenuItem alloc] initWithTitle:@"Reload skin.txt" action:@selector(chooseSkin:) keyEquivalent:@""];reset.target=self;reset.tag=-2;[menu addItem:reset];
  [NSMenu popUpContextMenu:menu withEvent:event forView:self];
}
- (void)choose:(NSMenuItem*)item {
  if(!owner)return;auto id=aztec::ParamID([item.representedObject unsignedIntValue]);int n=owner->info(id).stepCount;
  owner->edit(id,n?double(item.tag)/n:0.);[self setNeedsDisplay:YES];
}
- (void)chooseZoom:(NSMenuItem*)item {if(owner)owner->zoom(double(item.tag)/100.);}
- (NSURL*)presetFolder {
  NSString* path=[NSHomeDirectory() stringByAppendingPathComponent:@"Library/Audio/Presets/GrainsDosage"];
  NSURL* folder=[NSURL fileURLWithPath:path isDirectory:YES];NSError* error=nil;
  if(![[NSFileManager defaultManager] createDirectoryAtURL:folder withIntermediateDirectories:YES attributes:nil error:&error]){NSAlert* alert=[[NSAlert alloc] init];alert.messageText=@"Cannot create the preset folder";alert.informativeText=error.localizedDescription;[alert runModal];return nil;}
  for(int i=0;i<aztec::factoryPresetCount;++i){NSString* name=[[NSString stringWithUTF8String:aztec::factoryNames[i]] stringByAppendingPathExtension:@"gdspreset"];NSURL* url=[folder URLByAppendingPathComponent:name];
    if(![[NSFileManager defaultManager] fileExistsAtPath:url.path]){auto p=aztec::factoryPreset(i);auto bytes=aztec::encodePreset([&](int id){return p[id];});NSData* data=[NSData dataWithBytes:bytes.data() length:bytes.size()];
      if(![data writeToURL:url options:NSDataWritingWithoutOverwriting error:&error]&&![[NSFileManager defaultManager] fileExistsAtPath:url.path]){NSAlert* alert=[[NSAlert alloc] init];alert.messageText=@"Cannot install the factory presets";alert.informativeText=error.localizedDescription;[alert runModal];return nil;}}
  }
  return folder;
}
- (BOOL)loadPresetURL:(NSURL*)url {
  NSNumber* size=nil;[url getResourceValue:&size forKey:NSURLFileSizeKey error:nil];if(!size||size.unsignedLongLongValue>65536)return NO;
  NSData* data=[NSData dataWithContentsOfURL:url];std::array<double,aztec::kCount> p{};
  if(!data||!data.length||!aztec::decodePreset(std::string((const char*)data.bytes,data.length),p))return NO;
  for(int id=0;id<aztec::kCount;++id)if(aztec::presetParameter(id))owner->edit(id,p[id]);
  presetName=[[url lastPathComponent] stringByDeletingPathExtension];[self setNeedsDisplay:YES];return YES;
}
- (void)choosePreset:(NSMenuItem*)item {
  NSURL* url=item.representedObject;if(item.tag==-1){[[NSWorkspace sharedWorkspace] openURL:url];return;}
  if(![self loadPresetURL:url]){NSAlert* alert=[[NSAlert alloc] init];alert.messageText=@"Cannot load this preset";alert.informativeText=@"Invalid or incompatible preset. No settings changed.";[alert runModal];}
}
- (void)presetMenu:(NSEvent*)event {
  NSURL* folder=[self presetFolder];if(!folder)return;
  NSArray<NSURL*>* files=[[NSFileManager defaultManager] contentsOfDirectoryAtURL:folder includingPropertiesForKeys:nil options:NSDirectoryEnumerationSkipsHiddenFiles error:nil];
  files=[files sortedArrayUsingComparator:^NSComparisonResult(NSURL* a,NSURL* b){return [a.lastPathComponent localizedStandardCompare:b.lastPathComponent];}];
  NSMenu* menu=[[NSMenu alloc] initWithTitle:@"Presets"];
  for(NSURL* url in files)if([url.pathExtension.lowercaseString isEqualToString:@"gdspreset"]){NSMenuItem* item=[[NSMenuItem alloc] initWithTitle:[url.lastPathComponent stringByDeletingPathExtension] action:@selector(choosePreset:) keyEquivalent:@""];item.target=self;item.representedObject=url;[menu addItem:item];}
  [menu addItem:[NSMenuItem separatorItem]];NSMenuItem* open=[[NSMenuItem alloc] initWithTitle:@"Open Preset Folder…" action:@selector(choosePreset:) keyEquivalent:@""];open.target=self;open.tag=-1;open.representedObject=folder;[menu addItem:open];[NSMenu popUpContextMenu:menu withEvent:event forView:self];
}
- (void)preset:(BOOL)save {
  if(!owner)return;NSError* error=nil;bool ok=false;
  if(save){NSSavePanel* panel=[NSSavePanel savePanel];panel.allowedFileTypes=@[@"gdspreset"];panel.nameFieldStringValue=@"GrainsDosage.gdspreset";panel.directoryURL=[self presetFolder];
    if([panel runModal]!=NSModalResponseOK)return;
    auto text=aztec::encodePreset([&](int id){return owner->value(id);});NSData* data=[NSData dataWithBytes:text.data() length:text.size()];ok=[data writeToURL:panel.URL options:NSDataWritingAtomic error:&error];if(ok)presetName=[panel.URL.lastPathComponent stringByDeletingPathExtension];
  }else{NSOpenPanel* panel=[NSOpenPanel openPanel];panel.allowedFileTypes=@[@"gdspreset"];panel.allowsMultipleSelection=NO;panel.canChooseDirectories=NO;panel.directoryURL=[self presetFolder];
    if([panel runModal]!=NSModalResponseOK)return;
    ok=[self loadPresetURL:panel.URL];
  }
  if(!ok){NSAlert* alert=[[NSAlert alloc] init];alert.messageText=save?@"Preset could not be saved":@"Preset could not be loaded";alert.informativeText=error?error.localizedDescription:@"Invalid or incompatible GrainsDosage preset. No settings changed.";[alert runModal];}
  [self setNeedsDisplay:YES];
}
// PRESET < / > buttons: cycle through the sorted preset folder (factory + user files).
- (void)stepPreset:(int)direction {
  if(!owner)return;NSURL* folder=[self presetFolder];if(!folder)return;
  const char* rep=[folder fileSystemRepresentation];if(!rep)return;
  std::wstring directory;for(const char* s=rep;*s;++s)directory+=wchar_t(static_cast<unsigned char>(*s));// UTF-8 bytes as wchar units; nextPresetFile decodes via dirent
  // Build the current-name wstring first: inside a message-send expression the
  // selector keyword `direction:` resolves to the ObjC selector namespace, not
  // the method's int parameter, so passing `direction` inline fails to compile.
  std::wstring current;const char* cur=[presetName UTF8String];if(cur)for(const char* s=cur;*s;++s)current+=wchar_t(static_cast<unsigned char>(*s));
  const int dir=direction;
  auto name=aztec::nextPresetFile(directory,dir,current);
  if(name.empty())return;
  std::string utf8name;for(wchar_t c:name)utf8name+=static_cast<char>(c);
  NSString* file=[NSString stringWithUTF8String:utf8name.c_str()];
  NSURL* url=[folder URLByAppendingPathComponent:file];
  if(![self loadPresetURL:url]){NSAlert* alert=[[NSAlert alloc] init];alert.messageText=@"Cannot load this preset";alert.informativeText=@"Invalid or incompatible preset. No settings changed.";[alert runModal];}
}
- (void)mouseDown:(NSEvent*)event {
  if(!owner)return;[self.window makeFirstResponder:self];NSPoint p=[self logical:event];[self layoutControls];
  if(NSPointInRect(p,NSMakeRect(420,22,24,30))){[self stepPreset:-1];return;}// PRESET <
  if(NSPointInRect(p,NSMakeRect(858,22,24,30))){[self stepPreset:1];return;}// PRESET >
  if(NSPointInRect(p,NSMakeRect(444,22,414,30))){[self presetMenu:event];return;}
  if(NSPointInRect(p,NSMakeRect(982,1312,144,26))){[self skinMenu:event];return;}
  if(NSPointInRect(p,NSMakeRect(420,56,227,28))){[self preset:NO];return;}// LOAD
  if(NSPointInRect(p,NSMakeRect(653,56,229,28))){[self preset:YES];return;}// SAVE
  if(NSPointInRect(p,NSMakeRect(1140,1312,144,26))){
    NSMenu* menu=[[NSMenu alloc] initWithTitle:@"UI size"];
    for(int percent:{50,60,70,75,80,90,100}){NSMenuItem* item=[[NSMenuItem alloc] initWithTitle:[NSString stringWithFormat:@"%d%%",percent] action:@selector(chooseZoom:) keyEquivalent:@""];item.target=self;item.tag=percent;[menu addItem:item];}
    [NSMenu popUpContextMenu:menu withEvent:event forView:self];return;
  }
  if(NSPointInRect(p,NSMakeRect(1052,778,216,28))){aztec::randomizeReslice(randomSeed,[&](aztec::ParamID id){return owner->value(id);},[&](aztec::ParamID id,double v){owner->edit(id,v);});[self setNeedsDisplay:YES];return;}
  for(int i=0;i<16;++i)if(NSPointInRect(p,NSMakeRect(32+i*78,823,70,40))){selectedReslice=i;if(event.clickCount>=2)owner->edit(aztec::kResliceStep0+i,owner->value(aztec::kResliceStep0+i)>.5?0.:1.);[self setNeedsDisplay:YES];return;}
  for(int i=0;i<16;++i)if(NSPointInRect(p,NSMakeRect(32+i*78,943,70,40))){selectedGate=i;if(event.modifierFlags & NSEventModifierFlagShift){double next=owner->value(aztec::kGaterRelease0+i)>.5?0.:1.;owner->edit(aztec::kGaterRelease0+i,next);if(next>.5)owner->edit(aztec::kGaterState0+i,0.);}else{owner->edit(aztec::kGaterRelease0+i,0.);owner->edit(aztec::kGaterState0+i,owner->value(aztec::kGaterState0+i)>=.25?0.:1.);}[self setNeedsDisplay:YES];return;}
  for(int i=0;i<3;++i)if(NSPointInRect(p,NSMakeRect(slotX[i]+316,106,87,28))){aztec::randomizeModule([self stage:i],selectedRepeat,randomSeed,[&](aztec::ParamID id,double value){owner->edit(id,value);});[self setNeedsDisplay:YES];return;}
  for(int i=0;i<3;++i)if(NSPointInRect(p,NSMakeRect(slotX[i]+1,99,226,42))){dragSlot=dropSlot=i;[self setNeedsDisplay:YES];return;}
  double repeatX=slotX[[self slot:2]];
  for(int i=0;i<16;++i)if(NSPointInRect(p,NSMakeRect(repeatX+19+(i%8)*48,271+(i/8)*39,43,32))){
    selectedRepeat=i;if(event.clickCount>=2){auto id=aztec::ParamID(aztec::kRepeatStep0+i);owner->edit(id,owner->value(id)>=.5?0.:1.);}[self setNeedsDisplay:YES];return;
  }
  for(int i=0;i<4;++i)if(NSPointInRect(p,NSMakeRect(171+i*133,528,120,28))){selectedLfo=i;[self setNeedsDisplay:YES];return;}
  if(NSPointInRect(p,NSMakeRect(880,566,168,166))){
    dragXY=true;owner->begin(aztec::kXYX);owner->begin(aztec::kXYY);owner->change(aztec::kXYX,(p.x-880)/168.);owner->change(aztec::kXYY,1.-(p.y-566)/166.);[self setNeedsDisplay:YES];return;
  }
  for(const auto& c:controls)if(NSPointInRect(p,c.rect)){
    if(c.kind==aztec::PanMode)owner->edit(c.id,std::clamp(int((p.x-c.rect.origin.x)/(c.rect.size.width/3.)),0,2)/2.);
    else if(c.kind==aztec::Pad||c.kind==aztec::Toggle)owner->edit(c.id,owner->value(c.id)>=.5?0.:1.);
    else if(event.clickCount>=2)owner->edit(c.id,owner->info(c.id).defaultNormalizedValue);
    else if(c.kind==aztec::Select){
      NSMenu* menu=[[NSMenu alloc] initWithTitle:@"Select"];NSMenu* destination=menu;int n=owner->info(c.id).stepCount;
      for(int i=0;i<=n;++i){
        if(c.id==aztec::lfoID(selectedLfo,aztec::lWave)){
          if(i<128&&i%16==0){NSString* title=[NSString stringWithUTF8String:aztec::waveFamilies[i/16]];NSMenuItem* group=[[NSMenuItem alloc] initWithTitle:title action:nullptr keyEquivalent:@""];destination=[[NSMenu alloc] initWithTitle:title];group.submenu=destination;[menu addItem:group];}
          else if(i==128){[menu addItem:[NSMenuItem separatorItem]];destination=menu;}
        }
        NSMenuItem* item=[[NSMenuItem alloc] initWithTitle:owner->display(c.id,double(i)/std::max(1,n),false) action:@selector(choose:) keyEquivalent:@""];
        item.target=self;item.tag=i;item.representedObject=@(c.id);item.state=int(std::round(owner->value(c.id)*n))==i?NSControlStateValueOn:NSControlStateValueOff;[destination addItem:item];
      }[NSMenu popUpContextMenu:menu withEvent:event forView:self];
    }else{dragID=int(c.id);dragKind=c.kind;dragRect=c.rect;origin=p;starting=owner->value(c.id);owner->begin(c.id);}
    [self setNeedsDisplay:YES];return;
  }
}
- (void)mouseDragged:(NSEvent*)event {
  if(!owner)return;NSPoint p=[self logical:event];
  if(dragSlot>=0){dropSlot=-1;for(int i=0;i<3;++i)if(NSPointInRect(p,NSMakeRect(slotX[i],98,416,304)))dropSlot=i;[self setNeedsDisplay:YES];return;}
  if(dragXY){owner->change(aztec::kXYX,(p.x-880)/168.);owner->change(aztec::kXYY,1.-(p.y-566)/166.);[self setNeedsDisplay:YES];return;}
  if(dragID<0)return;double delta=(dragKind==aztec::Slider||dragKind==aztec::Pan)?(p.x-origin.x)/dragRect.size.width:(origin.y-p.y)/180.;
  if(event.modifierFlags&NSEventModifierFlagShift)delta*=.1;
  starting=std::clamp(starting+delta,0.,1.);origin=p;
  owner->change(aztec::ParamID(dragID),starting);[self setNeedsDisplay:YES];
}
- (void)mouseUp:(NSEvent*)event {
  (void)event;if(!owner)return;
  if(dragSlot>=0&&dropSlot>=0&&dragSlot!=dropSlot){std::array<int,3> order{};for(int i=0;i<3;++i)order[i]=[self stage:i];std::swap(order[dragSlot],order[dropSlot]);for(int i=0;i<6;++i)if(order==aztec::moduleOrders[i]){owner->edit(aztec::kModuleOrder,double(i)/6.);break;}}
  if(dragID>=0)owner->end(aztec::ParamID(dragID));if(dragXY){owner->end(aztec::kXYX);owner->end(aztec::kXYY);}
  dragID=dragSlot=dropSlot=-1;dragXY=false;[self setNeedsDisplay:YES];
}
- (void)scrollWheel:(NSEvent*)event {
  if(!owner)return;NSPoint p=[self logical:event];[self layoutControls];
  for(const auto& c:controls)if(NSPointInRect(p,c.rect)&&(c.kind==aztec::Knob||c.kind==aztec::Slider)){
    int n=owner->info(c.id).stepCount;double step=n?1./n:.01;if(event.modifierFlags&NSEventModifierFlagShift)step*=.1;
    if(event.scrollingDeltaY!=0.)owner->edit(c.id,owner->value(c.id)+(event.scrollingDeltaY>0?step:-step));[self setNeedsDisplay:YES];return;
  }
}
@end
namespace aztec {
Editor::Editor(EditController* c):controller_(c){controller_->addRef();rect=ViewRect(0,0,792,816);(void)qg::waveBank();}
Editor::~Editor(){removed();controller_->release();}
NSString* Editor::display(ParamID id,double v,bool units) const {
  String128 text{};controller_->getParamStringByValue(id,v,text);
  auto convert=[](const TChar* p)->NSString*{size_t n=0;while(n<128&&p[n])++n;return [[NSString alloc] initWithCharacters:reinterpret_cast<const unichar*>(p) length:n];};
  NSString* value=convert(text);ParameterInfo param=info(id);
  if(!(param.flags&ParameterInfo::kIsList)&&[value rangeOfString:@"."].location!=NSNotFound){
    // Keep compact precise readouts instead of RangeParameter's many trailing zeros.
    double number=value.doubleValue;value=[NSString stringWithFormat:@"%.2f",number];
    while([value hasSuffix:@"0"])value=[value substringToIndex:value.length-1];
    if([value hasSuffix:@"."])value=[value substringToIndex:value.length-1];
  }
  if(units&&param.units[0]){NSString* unit=convert(param.units);if([unit isEqualToString:@"% cycle"])unit=@"%";value=[value stringByAppendingFormat:@" %@",unit];}return value;
}
tresult Editor::isPlatformTypeSupported(FIDString type){return type&&std::strcmp(type,kPlatformTypeNSView)==0?kResultTrue:kResultFalse;}
tresult Editor::attached(void* parent,FIDString type){
  if(!parent||isPlatformTypeSupported(type)!=kResultTrue||surface_)return kResultFalse;
  auto result=CPluginView::attached(parent,type);if(result!=kResultOk)return result;
  static bool skinRestored=false;if(!skinRestored){skinRestored=true;aztec::theme::loadSkinFile(aztec::theme::defaultPath());}
  surface_=[[GrainsSurface alloc] initWithFrame:NSMakeRect(0,0,rect.getWidth(),rect.getHeight())];surface_->owner=this;
  [(__bridge NSView*)parent addSubview:surface_];[surface_ setNeedsDisplay:YES];return kResultOk;
}
tresult Editor::removed(){if(surface_){[surface_ stop];surface_->owner=nullptr;[surface_ removeFromSuperview];surface_=nil;}return CPluginView::removed();}
tresult Editor::onSize(ViewRect* size){if(!size)return kInvalidArgument;auto result=CPluginView::onSize(size);if(surface_){[surface_ setFrameSize:NSMakeSize(size->getWidth(),size->getHeight())];[surface_ setNeedsDisplay:YES];}return result;}
tresult Editor::checkSizeConstraint(ViewRect* size){if(!size)return kInvalidArgument;double scale=std::clamp(double(size->getWidth())/canvasW,.5,1.);size->right=size->left+int32(std::round(canvasW*scale));size->bottom=size->top+int32(std::round(canvasH*scale));return kResultTrue;}
void Editor::zoom(double scale){if(plugFrame){ViewRect next(0,0,int32(canvasW*scale),int32(canvasH*scale));plugFrame->resizeView(this,&next);}}
IPlugView* createEditor(EditController* controller){return new Editor(controller);}
}
