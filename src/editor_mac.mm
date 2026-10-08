#import <Cocoa/Cocoa.h>
#include "editor.h"
// Header order matters: every non-aztec dependency (qg DSP helpers in
// filter_sequencer.h, the skin palette in skin_spec/skin_theme, the per-module
// PNG registry in gui/modules_loader.h) is included at GLOBAL scope here. The
// aztec::Editor block below must stay free of such includes: including them
// inside namespace aztec silently re-opens nested namespaces (aztec::qg,
// aztec::skin, aztec::gui) and breaks every qualified reference later on.
#include "parameters.h"
#include "modulation.h"
#include "skin_spec.h"
#include "skin_theme.h"
#include "mockup_ui.h"
#include "gui/modules_loader.h"
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
constexpr double canvasW=mockup::width,canvasH=mockup::height;
enum Kind {Knob,Slider,Toggle,Select,Pad,Pan,PanMode,VSlider};
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
    // Momentary triggers (RANDOM ALL): pulse the value locally so the button
    // lights immediately; performEdit carries it to the processor, which
    // consumes the request and mirrors 0 back on the next audio block.
    controller_->setParamNormalized(id,v);controller_->performEdit(id,v);
    // GrainsSurface is only forward-declared at this point, so no message
    // sends to surface_ are allowed here. setNeedsDisplay: lives on NSView,
    // so up-casting the pointer lets us request a repaint (immediate button
    // feedback for momentary triggers like RANDOM ALL) without the full
    // @interface.
    if(id==kRandomAll&&surface_)dispatch_async(dispatch_get_main_queue(),^{[(NSView*)surface_ setNeedsDisplay:YES];});
  }
  void edit(ParamID id,double v) {begin(id);change(id,v);end(id);}
  void selectTab(int tab){controller_->setParamNormalized(kUiTab,tabToValue(tab));}
  void zoom(double scale);
};
}  // namespace aztec (closed before the Objective-C section; ObjC classes and
   // categories may only live at global scope. Re-opened further down for the
   // Editor member-function definitions.)

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
// arc() helper removed along with the knob modulation/value arcs.
static bool bipolar(aztec::ParamID id) {
  using namespace aztec;
  return (id>=kExtraRoutes0&&id<kGlitchMove)||id==kPitch||id==kTranspose||id==kXAmount||id==kYAmount||
    (id>=kRepeatPitch0&&id<kStretchOn)||
    (id>=kLfo0&&id<kLegacyCount&&int(id-kLfo0)%lStride>=lRoute0);
}
static const char* names[3]={"GRANULIZER","PRESLICER","BEAT REPEATER"};
static constexpr double slotX[3]={16.,452.,888.};
// Tab strip: FIVE buttons on top, one card per module — GRANULIZER /
// PRESLICER / BEAT REPEATER / RESLICE / GATER. There are no solo buttons and
// no stacked artefact tabs: each card owns the full-width panel area from
// extreme left to extreme right (x=16..1304, y=98..502), and switching cards
// flips instantly between the five processors to morph between artefacts.
// Modulation / Morph / Filter / Reverb live under the cards and stay visible.
static const char* tabNames[5]={"GRANULIZER","PRESLICER","BEAT REPEATER","RESLICE","GATER"};
// Tab strip geometry (canvas coordinates): five equal buttons in one row at
// y=84, directly under the header panel (which ends at y=81) and above the
// module cards (which start at y=98). Five 250 px buttons with 3 px gaps =
// 1286 px starting at x=17 -> right edge 1303, inside the 1320 canvas. Shared
// by the Cocoa paint/hit-test pass and mirrored by the Win32 editor.
inline void tabRectAt(int i,double& x,double& y,double& w,double& h){
  auto r=aztec::mockup::tabRect(i);x=r.x;y=r.y;w=r.w;h=r.h;
}

@interface GrainsSurface:NSView {
@public
  aztec::Editor* owner;
@private
  NSImage* artwork;
  // Per-module PNG artwork layers (Granulizer.png, PreSlicer.png, ...). One
  // independent visual layer per module, resolved from the replaceable skin
  // folder: <skinDir>/modules/<name>.png (+ @2x/ retina sibling). When a
  // layer is present it replaces that module's vector panel() so the PNG can
  // be edited/swapped without touching code; missing layers fall back to the
  // current vector-on-background rendering, pixel-for-pixel as before.
  NSMutableArray<NSImage*>* moduleLayers;
  NSImage* rackBackplate;
  NSImage* rackAtlas;
  NSImage* staticScene;
  NSString* scenePreset;
  std::array<double,aztec::kCount> sceneValues;
  std::array<int,5> sceneSelection;
  int animationFrames;
  NSUInteger staticBuilds;
  bool fullRedrawForInspection;
  NSMutableDictionary<NSString*,NSAttributedString*>* textCache;
  NSImage* knobFace;   // user-supplied knobOK.png face (assets/sprites/knob.png)
  NSString* skinDir;   // runtime skin folder: <bundle Resources>/GrainsDosage-skin
                       // (or ./GrainsDosage-skin next to the binary). Drop your
                       // own PNGs there to reskin without recompiling.
  NSMutableDictionary<NSString*,NSImage*>* sprites;
  NSString* presetName;
  std::vector<aztec::Control> controls;
  std::array<double,aztec::kCount> cached;
  int selectedGate,selectedReslice;
  int selectedLfo,selectedRepeat,dragID,dragSlot,dropSlot;
  bool dragXY,routeMoved;
  aztec::mockup::WaveVisual waveVisual;
  aztec::mockup::Motion motion;
  std::array<int,5> routingAtDrag;
  aztec::Kind dragKind;
  NSRect dragRect;
  NSPoint origin;
  double starting;
  NSTimer* timer;
  uint32_t randomSeed;
}
- (BOOL)skinLoaded;
- (NSUInteger)staticBuilds;
- (void)setFullRedrawForInspection:(BOOL)enabled;
- (void)preset:(BOOL)save;
- (void)stepPreset:(int)direction;
- (NSURL*)presetFolder;
- (BOOL)loadPresetURL:(NSURL*)url;
- (void)presetMenu:(NSEvent*)event;
- (void)skinMenu:(NSEvent*)event;
- (void)chooseSkin:(NSMenuItem*)item;
- (void)choosePreset:(NSMenuItem*)item;
- (BOOL)controlsFit;
- (NSInteger)moduleIndexForRect:(NSRect)r;
- (int)hitTab:(NSPoint)p;
- (double)pxHitCard;
- (double)pyHitCard;
- (void)drawTabs;
- (void)stop;
- (void)tick:(NSTimer*)tick;
- (void)choose:(NSMenuItem*)item;
- (void)chooseZoom:(NSMenuItem*)item;
@end
@implementation GrainsSurface
- (instancetype)initWithFrame:(NSRect)frame {
  self=[super initWithFrame:frame];if(self){
    presetName=@"PRESETS ▾";sceneValues.fill(-1.);sceneSelection.fill(-1);animationFrames=0;staticBuilds=0;textCache=[NSMutableDictionary dictionary];
    // Tab strip (Option B): the Cocoa editor owns its own selection state for
    // the per-step step-sequencer widgets (mirrors the Win32 editor). The tab
    // itself lives in kUiTab. These must be initialised before any layout is
    // built - otherwise every dependent row (STEP LENGTH / SUSTAIN / SOURCE
    // SLICE / DESTINATION) reads garbage indices and controls land off-canvas.
    selectedGate=selectedReslice=0;
    randomSeed=arc4random()|1;selectedLfo=selectedRepeat=0;dragID=dragSlot=dropSlot=-1;dragXY=routeMoved=false;cached.fill(-1.);
    // ---- Runtime skin folder -------------------------------------------------
    // Everything graphic is resolved from one replaceable folder so swapping a
    // single PNG reskins the plugin with no rebuild. Search order per image:
    //   1. <bundle Resources>/GrainsDosage-skin/<name>.png  (installed drop-in)
    //   2. ./GrainsDosage-skin/<name>.png                   (dev layout, repo root)
    //   3. bundled resource / factory fallback              (default skin)
    // On first run the factory PNGs are auto-copied into the writable app-support
    // copy of the folder, so users always have real files to edit/replace.
    auto exists=[](NSString* p){return p&&[[NSFileManager defaultManager] fileExistsAtPath:p];};
    {NSString* res=[[NSBundle bundleForClass:[GrainsSurface class]] resourcePath];
     skinDir=res?[res stringByAppendingPathComponent:@"GrainsDosage-skin"]:@"GrainsDosage-skin";
     if(!exists(skinDir)){
       NSString* local=@"GrainsDosage-skin";
       if(exists(local))skinDir=local;}}
    auto skinFile=[&](NSString* name){
      NSString* f=[skinDir stringByAppendingFormat:@"/%@.png",name];
      return exists(f)?f:nil;};
    // Seed a writable copy of the skin folder (Application Support) once.
    auto seedSkin=[&](NSString* name,NSString* fallback){
      NSString* dst=[skinDir stringByAppendingFormat:@"/%@.png",name];
      if(exists(dst)||!exists(fallback))return;
      NSString* dir=[dst stringByDeletingLastPathComponent];
      [[NSFileManager defaultManager] createDirectoryAtPath:dir
                                withIntermediateDirectories:YES attributes:nil error:nil];
      [[NSFileManager defaultManager] copyPath:fallback toPath:dst handler:nil];};
    // Artwork background: skin folder first, bundled PNG as fallback.
    {NSString* bg=skinFile(@"background");
     if(!bg){NSString* path=[[NSBundle bundleForClass:[GrainsSurface class]] pathForResource:@"GrainsDosage-skin" ofType:@"png"];
       if(!path)path=@"assets/GrainsDosage-skin.png";
       if(!exists(path))path=@"GrainsDosage/assets/GrainsDosage-skin.png";
       if(exists(path)){bg=path;seedSkin(@"background",path);}}
     artwork=bg?[[NSImage alloc] initWithContentsOfFile:bg]:nil;
     if(artwork)artwork.size=NSMakeSize(2048,1520);}
    // ---- Per-module PNG artwork layers ---------------------------------------
    // Granulizer.png / PreSlicer.png / BeatRepeater.png / Modulation.png /
    // Morph.png / Reslice.png / Gater.png / Filter.png / Reverb.png /
    // FilterSeq.png / MasterOut.png — one independent visual layer per module.
    // Search order per layer (same replaceable-folder contract as background):
    //   1. <skinDir>/modules/<name>.png   (+ @2x/ retina sibling rep)
    //   2. repo assets/modules/<name>.png (dev layout fallback, seeds the folder)
    // Absent layer -> nil entry -> panel() vector fallback keeps the current
    // look pixel-for-pixel, so shipping without the PNGs changes nothing.
    moduleLayers=[NSMutableArray arrayWithCapacity:aztec::gui::kModuleImages.size()];
    {auto exists2=[](NSString* p){return p&&[[NSFileManager defaultManager] fileExistsAtPath:p];};
     for(size_t i=0;i<aztec::gui::kModuleImages.size();++i){
      const auto& img=aztec::gui::kModuleImages[i];
      NSString* name=[NSString stringWithUTF8String:img.name];
      NSImage* layer=nil;
      NSString* runtimeFile=[skinDir stringByAppendingFormat:@"/modules/%@.png",name];
      NSString* runtimeRetina=[skinDir stringByAppendingFormat:@"/modules/@2x/%@.png",name];
      if(exists(runtimeFile)){
        layer=[[NSImage alloc] initWithContentsOfFile:runtimeFile];
        if(exists2(runtimeRetina)){
          NSImage* hi=[[NSImage alloc] initWithContentsOfFile:runtimeRetina];
          if(hi.representations.count)[layer addRepresentation:hi.representations.firstObject];}}
      if(!layer||!layer.representations.count){
        // Dev layout: read straight from the repo artwork and seed it into the
        // writable skin folder so users have a real file to edit per module.
        NSString* file=[@("assets/modules/") stringByAppendingString:name];
        file=[file stringByAppendingPathExtension:@"png"];
        if(!exists(file))file=[@"GrainsDosage/assets/modules/" stringByAppendingFormat:@"%@.png",name];
        NSString* hiFile=exists(file)?[file stringByReplacingOccurrencesOfString:@"/modules/" withString:@"/modules/@2x/"]:nil;
        if(exists(file)){
          layer=[[NSImage alloc] initWithContentsOfFile:file];
          if(exists2(hiFile)){
            NSImage* hi=[[NSImage alloc] initWithContentsOfFile:hiFile];
            if(hi.representations.count)[layer addRepresentation:hi.representations.firstObject];}
          NSString* dst=[skinDir stringByAppendingFormat:@"/modules/%@.png",name];
          if(!exists(dst)){
            [[NSFileManager defaultManager] createDirectoryAtPath:[dst stringByDeletingLastPathComponent]
                                  withIntermediateDirectories:YES attributes:nil error:nil];
            [[NSFileManager defaultManager] copyPath:file toPath:dst handler:nil];}}}
      if(layer)layer.size=NSMakeSize(img.width,img.height);  // canvas units
      [moduleLayers addObject:layer?:[NSNull null]];}}
    sprites=[NSMutableDictionary dictionary];
    NSArray<NSString*>* spriteNames=@[@"knob",@"slider",@"step",@"header-left",@"header-right",@"xy-nebula"];
    NSArray<NSString*>* spriteKeys=@[@"385,260,76,76",@"453,636,24,32",@"734,456,54,48",@"1850,12,136,104",@"1726,15,95,104",@"nebula"];
    for(NSUInteger i=0;i<spriteNames.count;++i){
      NSImage* sprite=nil;
      NSString* runtimeFile=skinFile(spriteNames[i]);
      if(runtimeFile)sprite=[[NSImage alloc] initWithContentsOfFile:runtimeFile];
      if(!sprite||!sprite.representations.count){
        NSString* file=[[NSBundle bundleForClass:[GrainsSurface class]] pathForResource:spriteNames[i] ofType:@"png"];
        if(!file)file=[@"assets/sprites/" stringByAppendingFormat:@"%@.png",spriteNames[i]];
        if(!exists(file))file=[@"GrainsDosage/" stringByAppendingString:file];
        if(exists(file)){
          sprite=[[NSImage alloc] initWithContentsOfFile:file];
          seedSkin(spriteNames[i],file);}}
      if(sprite)sprites[spriteKeys[i]]=sprite;}
    // Use the supplied transparent face. A distinct name avoids stale skins.
    NSString* face=[[NSBundle bundleForClass:[GrainsSurface class]] pathForResource:@"mockup-knob" ofType:@"png"];
    if(!exists(face))face=@"assets/mockup/knob.png";
    knobFace=[[NSImage alloc] initWithContentsOfFile:face];
    NSString* back=[[NSBundle bundleForClass:[GrainsSurface class]] pathForResource:@"approved-backplate" ofType:@"png"];
    if(!exists(back))back=@"assets/approved-rack/backplate.png";
    rackBackplate=[[NSImage alloc] initWithContentsOfFile:back];
    NSString* atlas=[[NSBundle bundleForClass:[GrainsSurface class]] pathForResource:@"approved-controls" ofType:@"png"];
    if(!exists(atlas))atlas=@"assets/approved-rack/controls.png";
    rackAtlas=[[NSImage alloc] initWithContentsOfFile:atlas];
    self.toolTip=@"Drag knobs vertically; Shift gives fine control. Double-click resets. Drag the module tabs to change audio order.";
    timer=[NSTimer timerWithTimeInterval:1./30. target:self selector:@selector(tick:) userInfo:nil repeats:YES];
    timer.tolerance=.003;
    [[NSRunLoop mainRunLoop] addTimer:timer forMode:NSRunLoopCommonModes];
  }return self;
}
- (BOOL)isFlipped{return YES;}
- (BOOL)isOpaque{return YES;}
- (BOOL)acceptsFirstResponder{return YES;}
- (NSUInteger)staticBuilds{return staticBuilds;}
- (void)setFullRedrawForInspection:(BOOL)enabled{fullRedrawForInspection=enabled;if(!enabled)staticScene=nil;}
- (BOOL)skinLoaded{return rackBackplate!=nil&&rackAtlas!=nil;}
- (void)stop {
  if(owner&&dragID>=0)owner->end(aztec::ParamID(dragID));
  if(owner&&dragXY){owner->end(aztec::kXYX);owner->end(aztec::kXYY);}
  dragID=dragSlot=dropSlot=-1;dragXY=false;[timer invalidate];timer=nil;
}
- (void)tick:(NSTimer*)tick {
  (void)tick;if(!owner||self.hiddenOrHasHiddenAncestor||!self.window||!(self.window.occlusionState&NSWindowOcclusionStateVisible))return;
  bool changed=false;
  for(int id=0;id<aztec::kCount;++id){double v=owner->value(id);if(v!=cached[id]){cached[id]=v;changed=true;}}
  if(changed)animationFrames=60;
  if(animationFrames>0||dragSlot>=0){
    --animationFrames;
    if([self tab]==0)waveVisual.update([&](aztec::ParamID id){return cached[id];});
    [self setNeedsDisplay:YES];
  }
}
- (NSPoint)logical:(NSEvent*)event {
  NSPoint p=[self convertPoint:event.locationInWindow fromView:nil];
  aztec::mockup::Viewport viewport(self.bounds.size.width,self.bounds.size.height);auto logical=viewport.logical(p.x,p.y);return NSMakePoint(logical.first,logical.second);
}
- (int)order {return std::clamp(int(std::round(owner->value(aztec::kModuleOrder)*6.)),0,6);}
- (int)tab {return aztec::tabFromValue(owner->value(aztec::kUiTab));}
// The tab strip is the single source of truth for the top-row cards: the
// active card selects which module owns the full-width panel area. Audio
// routing keeps following kModuleOrder (the AUDIO ORDER control), untouched.
- (void)chooseTab:(int)tab {
  if(!owner||tab<0||tab>=aztec::tabCount)return;
  owner->selectTab(tab);
  [self setNeedsDisplay:YES];
}
- (int)stage:(int)slot {int order=[self order];return aztec::moduleOrders[order>=6?0:order][slot];}
- (int)slot:(int)stage {for(int i=0;i<3;++i)if([self stage:i]==stage)return i;return 0;}
- (void)add:(aztec::ParamID)id x:(double)x y:(double)y w:(double)w h:(double)h kind:(aztec::Kind)kind label:(const char*)name {
  controls.push_back({id,NSMakeRect(x,y,w,h),kind,name});
}
- (void)layoutControls {
  using namespace aztec;controls.clear();
  const int currentTab=[self tab];   // shared layout (.inl) reads this
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
- (int)hitTab:(NSPoint)p {
  // Tab strip hit test (see tabRectAt for geometry). Returns -1 when the point
  // is outside every tab button so callers fall through to the legacy handlers.
  for(int i=0;i<5;++i){double x,y,w,h;tabRectAt(i,x,y,w,h);
    if(NSPointInRect(p,NSMakeRect(x,y,w,h)))return i;}
  return -1;
}
- (double)pxHitCard { double x,y,w,h;aztec::tabSlotRect([self tab],0,x,y,w,h);return x;}
- (double)pyHitCard { double x,y,w,h;aztec::tabSlotRect([self tab],0,x,y,w,h);return y;}
- (void)drawTabs {
  const int active=[self tab];
  for(int i=0;i<5;++i){
    double x,y,w,h;tabRectAt(i,x,y,w,h);NSRect r=NSMakeRect(x,y,w,h);
    bool on=i==active;
    box(r,on?AZSKIN(kAccentGlow):AZSKIN(kPanelRaised),on?green():AZSKIN(kFiligreeDim),4);
    label(@(tabNames[i]),NSMakeRect(x,y+1,w,h-2),9,on?onText():cream(),true);
  }
}
- (NSInteger)moduleIndexForRect:(NSRect)r {
  // Match a panel() rect against the kModuleImages canvas positions. The three
  // top-row slots share one module identity (Granulizer/PreSlicer/BeatRepeater
  // swap by drag order), so slot rects always resolve to their index 0/1/2.
  for(size_t i=0;i<aztec::gui::kModuleImages.size();++i){
    const auto& img=aztec::gui::kModuleImages[i];
    if(std::abs(r.origin.x-img.canvasX)<1&&std::abs(r.origin.y-img.canvasY)<1&&
       std::abs(r.size.width-img.width)<1&&std::abs(r.size.height-img.height)<1)
      return NSInteger(i);}
  return -1;
}
- (void)panel:(NSRect)r {
  // Per-module PNG artwork layer wins when present: the panel becomes an
  // independent visual layer that can be edited/swapped without a rebuild.
  // All controls keep drawing on top at their exact current positions.
  NSInteger moduleIndex=[self moduleIndexForRect:r];
  if(moduleIndex>=0&&moduleIndex<(NSInteger)moduleLayers.count){
    id entry=moduleLayers[(NSUInteger)moduleIndex];
    if(entry!=(id)[NSNull null]){
      NSImage* layer=(NSImage*)entry;
      [NSGraphicsContext currentContext].imageInterpolation=NSImageInterpolationHigh;
      [layer drawInRect:r fromRect:NSZeroRect operation:NSCompositingOperationSourceOver
                fraction:1. respectFlipped:YES hints:nil];
      return;}}
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
- (void)drawRect:(NSRect)dirty {
  (void)dirty;[dark() setFill];NSRectFill(self.bounds);if(!owner)return;[self layoutControls];
  [NSGraphicsContext saveGraphicsState];NSAffineTransform* transform=[NSAffineTransform transform];
  aztec::mockup::Viewport viewport(self.bounds.size.width,self.bounds.size.height);
  [transform translateXBy:viewport.x yBy:viewport.y];[transform scaleXBy:viewport.scale yBy:viewport.scale];[transform concat];
  using namespace aztec;
  mockup::Painter painter;
  painter.image=[&](int asset,mockup::Rect r,mockup::Rect source){
    NSImage* image=asset==mockup::Backplate?rackBackplate:rackAtlas;if(!image)return;
    [image drawInRect:NSMakeRect(r.x,r.y,r.w,r.h) fromRect:NSMakeRect(source.x,image.size.height-source.y-source.h,source.w,source.h) operation:NSCompositingOperationSourceOver fraction:1. respectFlipped:YES hints:@{NSImageHintInterpolation:@(NSImageInterpolationHigh)}];
  };
  painter.box=[](mockup::Rect r,skin::Rgb fill,skin::Rgb edge,double radius){box(NSMakeRect(r.x,r.y,r.w,r.h),C(fill),C(edge),radius);};
  painter.text=[&](const std::string& text,mockup::Rect r,double size,skin::Rgb color,bool center){
    NSString* str=[NSString stringWithUTF8String:text.c_str()];
    NSString* key=[NSString stringWithFormat:@"%@|%.2f|%d,%d,%d|%d",str,size,color.r,color.g,color.b,center];
    NSAttributedString* cachedText=textCache[key];
    if(!cachedText){
      NSMutableParagraphStyle* style=[[NSMutableParagraphStyle alloc] init];style.alignment=center?NSTextAlignmentCenter:NSTextAlignmentLeft;style.lineBreakMode=NSLineBreakByTruncatingTail;
      NSFont* font=[NSFont fontWithName:@"HelveticaNeue-Bold" size:size];if(!font)font=[NSFont systemFontOfSize:size weight:NSFontWeightHeavy];
      NSShadow* shadow=[[NSShadow alloc] init];shadow.shadowColor=rgb(.04,.03,.08,.8);shadow.shadowOffset=NSMakeSize(0,-1);shadow.shadowBlurRadius=1;
      cachedText=[[NSAttributedString alloc] initWithString:str attributes:@{NSFontAttributeName:font,NSForegroundColorAttributeName:C(color),NSParagraphStyleAttributeName:style,NSShadowAttributeName:shadow}];
      if(textCache.count>512)[textCache removeAllObjects];textCache[key]=cachedText;
    }
    [cachedText drawInRect:NSMakeRect(r.x,r.y+(r.h-size*1.2)/2,r.w,size*1.4)];
  };
  painter.line=[](double x,double y,double xx,double yy,skin::Rgb col,double width){NSBezierPath* path=[NSBezierPath bezierPath];[path moveToPoint:NSMakePoint(x,y)];[path lineToPoint:NSMakePoint(xx,yy)];[C(col) setStroke];path.lineWidth=width;[path stroke];};
  painter.polyline=[](const std::vector<std::pair<double,double>>& pts,skin::Rgb col,double width){if(pts.empty())return;NSBezierPath* path=[NSBezierPath bezierPath];[path moveToPoint:NSMakePoint(pts[0].first,pts[0].second)];for(size_t i=1;i<pts.size();++i)[path lineToPoint:NSMakePoint(pts[i].first,pts[i].second)];[C(col) setStroke];path.lineWidth=width;[path stroke];};
  painter.polygon=[](const std::vector<std::pair<double,double>>& pts,skin::Rgb col){if(pts.empty())return;NSBezierPath* path=[NSBezierPath bezierPath];[path moveToPoint:NSMakePoint(pts[0].first,pts[0].second)];for(size_t i=1;i<pts.size();++i)[path lineToPoint:NSMakePoint(pts[i].first,pts[i].second)];[path closePath];[C(col) setFill];[path fill];};
  painter.knob=[&](mockup::Rect r,double value){
    if(!knobFace){box(NSMakeRect(r.x,r.y,r.w,r.h),rgb(.7,.7,.7),cream(),r.w/2);return;}
    CGImageRef face=[knobFace CGImageForProposedRect:nullptr context:nil hints:nil];
    CGContextRef ctx=[NSGraphicsContext currentContext].CGContext;CGContextSaveGState(ctx);
    CGContextTranslateCTM(ctx,r.x+r.w/2,r.y+r.h/2);CGContextRotateCTM(ctx,(270*value-135)*::qg::tau/360.);
    CGContextScaleCTM(ctx,1,-1);CGContextSetInterpolationQuality(ctx,kCGInterpolationHigh);
    CGContextDrawImage(ctx,CGRectMake(-r.w/2,-r.h/2,r.w,r.h),face);CGContextRestoreGState(ctx);
  };
  bool rebuild=!staticScene||![scenePreset isEqualToString:presetName];
  std::array<int,5> selection{{[self tab],selectedLfo,selectedRepeat,selectedGate,selectedReslice}};
  if(selection!=sceneSelection){sceneSelection=selection;rebuild=true;}
  for(int id=0;id<kCount;++id)if(!isMonitor(id)){double v=owner->value(id);if(sceneValues[id]!=v){sceneValues[id]=v;rebuild=true;}}
  const int activeWave=kUiModWave0+selectedLfo;
  if(owner->value(kModWaveRnd0+selectedLfo)>0&&sceneValues[activeWave]!=owner->value(activeWave)){sceneValues[activeWave]=owner->value(activeWave);rebuild=true;}
  auto render=[&](mockup::RenderPass pass){mockup::render(painter,[&](ParamID id){return owner->value(id);},[&](ParamID id,double v){return std::string([owner->display(id,v) UTF8String]);},std::string([presetName UTF8String]),[self tab],selectedLfo,selectedRepeat,selectedGate,selectedReslice,&waveVisual,dragSlot,dropSlot,&motion,pass);};
  if(fullRedrawForInspection){render(mockup::RenderPass::All);}else{
  if(rebuild){
    staticScene=[[NSImage alloc] initWithSize:NSMakeSize(canvasW,canvasH)];
    [staticScene lockFocusFlipped:YES];render(mockup::RenderPass::Static);[staticScene unlockFocus];
    scenePreset=[presetName copy];++staticBuilds;
  }
  [staticScene drawInRect:NSMakeRect(0,0,canvasW,canvasH) fromRect:NSZeroRect operation:NSCompositingOperationCopy fraction:1. respectFlipped:YES hints:nil];
  render(mockup::RenderPass::Dynamic);
  }
  [NSGraphicsContext restoreGraphicsState];
}
- (void)chooseSkin:(NSMenuItem*)item {
  staticScene=nil;
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
  files=[files sortedArrayUsingComparator:^NSComparisonResult(NSURL* a,NSURL* b){int ca=aztec::presetCategory(std::string(a.lastPathComponent.stringByDeletingPathExtension.UTF8String)),cb=aztec::presetCategory(std::string(b.lastPathComponent.stringByDeletingPathExtension.UTF8String));if(ca!=cb)return ca<cb?NSOrderedAscending:NSOrderedDescending;return [a.lastPathComponent localizedStandardCompare:b.lastPathComponent];}];
  NSMenu* menu=[[NSMenu alloc] initWithTitle:@"Presets"];
  for(int category=0;category<aztec::factoryCategoryCount+2;++category){
    NSMenu* group=[[NSMenu alloc] initWithTitle:[NSString stringWithUTF8String:aztec::factoryCategories[category]]];
    for(NSURL* url in files)if([url.pathExtension.lowercaseString isEqualToString:@"gdspreset"]){NSString* name=url.lastPathComponent.stringByDeletingPathExtension;
      if(aztec::presetCategory(std::string(name.UTF8String))!=category)continue;
      NSMenuItem* item=[[NSMenuItem alloc] initWithTitle:name action:@selector(choosePreset:) keyEquivalent:@""];item.target=self;item.representedObject=url;item.state=[name isEqualToString:presetName]?NSControlStateValueOn:NSControlStateValueOff;[group addItem:item];}
    if(group.numberOfItems){NSMenuItem* parent=[[NSMenuItem alloc] initWithTitle:group.title action:nullptr keyEquivalent:@""];parent.submenu=group;[menu addItem:parent];}
  }
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
  using namespace aztec;
  auto hit=[&](mockup::Rect r){return r.contains(p.x,p.y);};
  if(hit(mockup::previous)){[self stepPreset:-1];return;}
  if(hit(mockup::next)){[self stepPreset:1];return;}
  if(hit(mockup::preset)){[self presetMenu:event];return;}
  if(hit(mockup::load)){[self preset:NO];return;}
  if(hit(mockup::save)){[self preset:YES];return;}
  if(hit(mockup::skinMenu)){[self skinMenu:event];return;}
  if(hit(mockup::zoomMenu)){
    NSMenu* menu=[[NSMenu alloc] initWithTitle:@"UI size"];
    for(int percent:{50,60,70,75,80,90,100}){NSMenuItem* item=[[NSMenuItem alloc] initWithTitle:[NSString stringWithFormat:@"%d%%",percent] action:@selector(chooseZoom:) keyEquivalent:@""];item.target=self;item.tag=percent;[menu addItem:item];}
    [NSMenu popUpContextMenu:menu withEvent:event forView:self];return;
  }
  int led=mockup::hitLed(p.x,p.y);if(led>=0){int stage=mockup::routeChain([&](ParamID id){return owner->value(id);})[led];auto id=mockup::stageEnabled[stage];owner->edit(id,owner->value(id)>=.5?0.:1.);[self setNeedsDisplay:YES];return;}
  int route=mockup::hitRoute(p.x,p.y);if(route>=0){dragSlot=route;dropSlot=-1;routeMoved=false;origin=p;routingAtDrag=mockup::routeChain([&](ParamID id){return owner->value(id);});[self chooseTab:routingAtDrag[route]];[self setNeedsDisplay:YES];return;}
  int lfoHit=mockup::hitLfo(p.x,p.y);if(lfoHit>=0){selectedLfo=lfoHit;[self setNeedsDisplay:YES];return;}
  if(hit(mockup::randomRect([self tab]))){
    int t=[self tab];if(t<3)randomizeModule(t,selectedRepeat,randomSeed,[&](ParamID id){return owner->value(id);},[&](ParamID id,double v){owner->edit(id,v);});
    else if(t==3)randomizeReslice(randomSeed,[&](ParamID id){return owner->value(id);},[&](ParamID id,double v){owner->edit(id,v);});
    else {for(int i=0;i<16;++i){randomSeed=randomSeed*1664525u+1013904223u;owner->edit(kGaterState0+i,(randomSeed>>31)?1.:0.);}}
    [self setNeedsDisplay:YES];return;
  }
  int step=mockup::hitStep(p.x,p.y,[self tab]);if([self tab]>=2&&step>=0){
    if([self tab]==2){selectedRepeat=step;if(event.clickCount>=2)owner->edit(kRepeatStep0+step,owner->value(kRepeatStep0+step)>.5?0.:1.);}
    if([self tab]==3){selectedReslice=step;if(event.clickCount>=2)owner->edit(kResliceStep0+step,owner->value(kResliceStep0+step)>.5?0.:1.);}
    if([self tab]==4){selectedGate=step;if(event.modifierFlags&NSEventModifierFlagShift){double v=owner->value(kGaterRelease0+step)>.5?0.:1.;owner->edit(kGaterRelease0+step,v);if(v>.5)owner->edit(kGaterState0+step,0.);}else{owner->edit(kGaterRelease0+step,0.);owner->edit(kGaterState0+step,owner->value(kGaterState0+step)>.25?0.:1.);}}
    [self setNeedsDisplay:YES];return;
  }
  if(hit(mockup::xy)){dragXY=true;owner->begin(kXYX);owner->begin(kXYY);owner->change(kXYX,(p.x-mockup::xy.x-12)/(mockup::xy.w-24));owner->change(kXYY,1-(p.y-mockup::xy.y-12)/(mockup::xy.h-24));[self setNeedsDisplay:YES];return;}
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
  if(dragSlot>=0){if(std::hypot(p.x-origin.x,p.y-origin.y)>8)routeMoved=true;if(routeMoved)dropSlot=aztec::mockup::routeInsertion(p.x,p.y);[self setNeedsDisplay:YES];return;}
  if(dragXY){owner->change(aztec::kXYX,(p.x-aztec::mockup::xy.x-12)/(aztec::mockup::xy.w-24));owner->change(aztec::kXYY,1.-(p.y-aztec::mockup::xy.y-12)/(aztec::mockup::xy.h-24));[self setNeedsDisplay:YES];return;}
  if(dragID<0)return;double delta=(dragKind==aztec::Slider||dragKind==aztec::Pan)?(p.x-origin.x)/dragRect.size.width:(origin.y-p.y)/(dragKind==aztec::VSlider?std::max(1.,dragRect.size.height-42):180.);
  if(event.modifierFlags&NSEventModifierFlagShift)delta*=.1;
  starting=std::clamp(starting+delta,0.,1.);origin=p;
  owner->change(aztec::ParamID(dragID),starting);[self setNeedsDisplay:YES];
}
- (void)mouseUp:(NSEvent*)event {
  (void)event;if(!owner)return;
  if(dragSlot>=0&&dropSlot>=0&&dropSlot!=dragSlot&&dropSlot!=dragSlot+1)owner->edit(aztec::kRoutingOrder,aztec::mockup::moveRoute(routingAtDrag,dragSlot,dropSlot));
  if(dragID>=0)owner->end(aztec::ParamID(dragID));if(dragXY){owner->end(aztec::kXYX);owner->end(aztec::kXYY);}
  dragID=dragSlot=dropSlot=-1;dragXY=routeMoved=false;[self setNeedsDisplay:YES];
}
- (void)scrollWheel:(NSEvent*)event {
  if(!owner)return;NSPoint p=[self logical:event];[self layoutControls];
  for(const auto& c:controls)if(NSPointInRect(p,c.rect)&&(c.kind==aztec::Knob||c.kind==aztec::Slider||c.kind==aztec::VSlider)){
    int n=owner->info(c.id).stepCount;double step=n?1./n:.01;if(event.modifierFlags&NSEventModifierFlagShift)step*=.1;
    if(event.scrollingDeltaY!=0.)owner->edit(c.id,owner->value(c.id)+(event.scrollingDeltaY>0?step:-step));[self setNeedsDisplay:YES];return;
  }
}
@end
namespace aztec {
// No "using namespace qg" here: inside this namespace an unqualified qg would
// find aztec::qg first (shadowing), so global qg names must stay ::qg-qualified.
Editor::Editor(EditController* c):controller_(c){controller_->addRef();rect=ViewRect(0,0,int(mockup::width*.70),int(mockup::height*.70));(void)::qg::waveBank();}
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

