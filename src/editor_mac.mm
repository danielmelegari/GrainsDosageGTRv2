#import <Cocoa/Cocoa.h>
#include "editor.h"
#include "editor_refresh.h"
#include "host_edit_queue.h"
#include "license.h"
#include "value_entry.h"
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
#include "module_preset.h"
#include "preset_banks.h"
#include "preset_catalog.h"
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
  std::shared_ptr<EditorState> editorState_;
  __strong GrainsSurface* surface_=nil;
  HostEditQueue hostEdits_;
public:
  explicit Editor(EditController* c,std::shared_ptr<EditorState> state);
  void rememberPresetName(NSString* name){editorState_->presetName=name.UTF8String?name.UTF8String:"";}
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
  void flushHostEdits(bool force=false){hostEdits_.flush(EditorRefresh::now(),[&](uint32_t id,double v){controller_->performEdit(id,v);},force);}
  void end(ParamID id) {flushHostEdits(true);hostEdits_.clear();controller_->endEdit(id);}
  void dragChange(ParamID id,double v){
    v=std::clamp(v,0.,1.);int n=info(id).stepCount;if(n>0)v=std::round(v*n)/n;
    if(value(id)==v)return;
    controller_->setParamNormalized(id,v);hostEdits_.queue(id,v);flushHostEdits();
  }
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
  bool parse(ParamID id,const std::string& text,double& v){return parseParameterEntry(controller_,id,text,v);}
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
    NSColor* top=[fill blendedColorWithFraction:.08 ofColor:C(aztec::skin::Rgb{95,102,114})];
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
  NSImage* rackCables;
  NSImage* rackAtlas;
  NSImage* rackLogo;
  NSString* licenseLabel;
  NSMutableDictionary<NSNumber*,NSImage*>* editableModuleSkins;
  NSImage* staticScene;
  aztec::mockup::RackState rackState;
  int scenePage;double sceneScroll;
  bool dragScroll;double scrollOrigin;NSPoint routePointer;
  NSMutableDictionary<NSString*,id>* rasterSlices;
  NSString* scenePreset;
  std::array<double,aztec::kCount> sceneValues;
  std::array<int,5> sceneSelection;
  int animationFrames;
  NSUInteger staticBuilds;
  NSUInteger controlPatches;
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
  std::array<int,4> routingAtDrag;
  aztec::Kind dragKind;
  NSRect dragRect;
  NSPoint origin;
  double starting;
  NSTimer* timer;
  aztec::EditorRefresh refresh;
  uint32_t randomSeed;
}
- (BOOL)skinLoaded;
- (NSUInteger)staticBuilds;
- (NSUInteger)controlPatches;
- (void)setFullRedrawForInspection:(BOOL)enabled;
- (void)preset:(BOOL)save;
- (void)stepPreset:(int)direction;
- (void)restorePresetName:(NSString*)name;
- (NSURL*)presetFolder;
- (BOOL)loadPresetURL:(NSURL*)url;
- (void)presetMenu:(NSEvent*)event;
- (void)skinMenu:(NSEvent*)event;
- (void)chooseSkin:(NSMenuItem*)item;
- (void)choosePreset:(NSMenuItem*)item;
- (BOOL)controlsFit;
- (int)page;
- (double)pageScroll;
- (void)setPageScroll:(double)value;
- (NSInteger)moduleIndexForRect:(NSRect)r;
- (void)stop;
- (void)tick:(NSTimer*)tick;
- (void)choose:(NSMenuItem*)item;
- (void)chooseZoom:(NSMenuItem*)item;
@end
@implementation GrainsSurface
- (void)restorePresetName:(NSString*)name {presetName=[name copy];}
- (instancetype)initWithFrame:(NSRect)frame {
  self=[super initWithFrame:frame];if(self){
    scenePage=-1;sceneScroll=-1;dragScroll=false;scrollOrigin=0;presetName=@"PRESETS ▾";sceneValues.fill(-1.);sceneSelection.fill(-1);animationFrames=0;staticBuilds=0;textCache=[NSMutableDictionary dictionary];rasterSlices=[NSMutableDictionary dictionary];
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
    NSString* cables=[[NSBundle bundleForClass:[GrainsSurface class]] pathForResource:@"approved-cables" ofType:@"png"];
    if(!exists(cables))cables=@"assets/approved-rack/cables.png";
    rackCables=[[NSImage alloc] initWithContentsOfFile:cables];
    NSString* back=[[NSBundle bundleForClass:[GrainsSurface class]] pathForResource:@"approved-backplate" ofType:@"png"];
    if(!exists(back))back=@"assets/approved-rack/backplate.png";
    rackBackplate=[[NSImage alloc] initWithContentsOfFile:back];
    NSString* atlas=[[NSBundle bundleForClass:[GrainsSurface class]] pathForResource:@"approved-controls" ofType:@"png"];
    if(!exists(atlas))atlas=@"assets/approved-rack/controls.png";
    rackAtlas=[[NSImage alloc] initWithContentsOfFile:atlas];
    NSString* logo=[[NSBundle bundleForClass:[GrainsSurface class]] pathForResource:@"approved-logo" ofType:@"png"];
    if(!exists(logo))logo=@"assets/approved-rack/logo.png";
    rackLogo=[[NSImage alloc] initWithContentsOfFile:logo];
    editableModuleSkins=[NSMutableDictionary dictionary];
    NSString* moduleFolder=[NSHomeDirectory() stringByAppendingPathComponent:@"Library/Application Support/GrainsDosage/Skins/Modules"];
    [[NSFileManager defaultManager] createDirectoryAtPath:moduleFolder withIntermediateDirectories:YES attributes:nil error:nil];
    for(int stage:{0,2,3,4,aztec::mockup::inputOptionsBackground}){
      const char* names[]={"granulizer","retired","beatrepeater","reslice","gater","input-options"};
      NSString* name=[NSString stringWithUTF8String:names[stage]];
      NSString* file=[moduleFolder stringByAppendingPathComponent:[name stringByAppendingString:@".png"]];
      NSString* bundled=[[NSBundle bundleForClass:[GrainsSurface class]] pathForResource:name ofType:@"png"];
      if(!bundled)bundled=[@"assets/module-skins" stringByAppendingPathComponent:[name stringByAppendingString:@".png"]];
      NSImage* image=[[NSImage alloc] initWithContentsOfFile:file];
      if(!image)image=[[NSImage alloc] initWithContentsOfFile:bundled];
      if(image)editableModuleSkins[@(stage)]=image;
    }
    self.toolTip=@"Drag knobs vertically; Double-click knobs/faders to reset; double-click a value readout to type; Shift-click restores its default. Scroll each page. Drag module headers vertically to change audio order. Option-scroll edits a knob.";
    timer=[NSTimer timerWithTimeInterval:1./60. target:self selector:@selector(tick:) userInfo:nil repeats:YES];
    timer.tolerance=.003;
    [[NSRunLoop mainRunLoop] addTimer:timer forMode:NSRunLoopCommonModes];
  }return self;
}
- (BOOL)isFlipped{return YES;}
- (BOOL)isOpaque{return YES;}
- (BOOL)acceptsFirstResponder{return YES;}
- (NSUInteger)staticBuilds{return staticBuilds;}
- (NSUInteger)controlPatches{return controlPatches;}
- (void)setFullRedrawForInspection:(BOOL)enabled{fullRedrawForInspection=enabled;if(!enabled)staticScene=nil;}
- (BOOL)skinLoaded{return rackCables!=nil&&rackLogo!=nil&&rackBackplate!=nil&&rackAtlas!=nil&&editableModuleSkins.count==5;}
- (void)stop {
  if(owner&&dragID>=0)owner->end(aztec::ParamID(dragID));
  if(owner&&dragXY){owner->end(aztec::kXYX);owner->end(aztec::kXYY);}
  dragID=dragSlot=dropSlot=-1;dragXY=false;[timer invalidate];timer=nil;
}
- (void)tick:(NSTimer*)tick {
  (void)tick;if(owner)owner->flushHostEdits();if(!owner||self.hiddenOrHasHiddenAncestor||!self.window||!(self.window.occlusionState&NSWindowOcclusionStateVisible))return;
  const double now=aztec::EditorRefresh::now();
  if(refresh.interactionDue(now))[self setNeedsDisplay:YES];
  if(dragID>=0||dragXY)return;
  if(!refresh.backgroundDue(now,1./15.))return;
  bool changed=false;
  NSString* currentLicense=[NSString stringWithUTF8String:aztec::license::status().c_str()];
  if(![licenseLabel isEqual:currentLicense]){licenseLabel=currentLicense;changed=true;}
  for(int id=0;id<aztec::kCount;++id){double v=owner->value(id);if(v!=cached[id]){cached[id]=v;if(!aztec::isMonitor(id)||aztec::mockup::monitorVisible(id,[&](aztec::ParamID p){return owner->value(p);},rackState,selectedLfo))changed=true;}}
  if(dragSlot>=0&&routeMoved){double delta=routePointer.y<aztec::mockup::bodyTop+45?-18:routePointer.y>aztec::mockup::height-45?18:0;if(delta){aztec::mockup::scrollBy(rackState,delta);dropSlot=aztec::mockup::routeInsertion(routePointer.x,routePointer.y,[&](aztec::ParamID id){return owner->value(id);},rackState);staticScene=nil;}}
  if(changed)animationFrames=12;
  if(animationFrames>0||dragSlot>=0){
    --animationFrames;
    if(rackState.page==0&&aztec::mockup::visible(aztec::mockup::waveRect([&](aztec::ParamID id){return owner->value(id);},rackState)))waveVisual.update([&](aztec::ParamID id){return cached[id];});
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
- (int)page{return rackState.page;}
- (double)pageScroll{return rackState.offset();}
- (void)setPageScroll:(double)value{rackState.scroll[rackState.page]=std::clamp(value,0.,aztec::mockup::maxScroll(rackState));staticScene=nil;[self setNeedsDisplay:YES];}
- (void)layoutControls {
  using namespace aztec;controls.clear();
  for(const auto& c:mockup::controls([&](ParamID id){return owner->value(id);},[self tab],selectedLfo,selectedRepeat,selectedGate,selectedReslice,rackState))controls.push_back({c.id,NSMakeRect(c.r.x,c.r.y,c.r.w,c.r.h),Kind(c.kind),c.label});
}
- (BOOL)controlsFit {
  [self layoutControls];for(const auto& c:controls)if(c.rect.size.width<=0||c.rect.size.height<=0||c.rect.origin.x<0||NSMaxX(c.rect)>aztec::canvasW)return NO;return YES;
}
- (void)art:(NSRect)source in:(NSRect)dest opacity:(double)opacity {
  NSString* key=[NSString stringWithFormat:@"%.0f,%.0f,%.0f,%.0f",source.origin.x,source.origin.y,source.size.width,source.size.height];NSImage* sprite=sprites[key];
  if(sprite){[NSGraphicsContext currentContext].imageInterpolation=NSImageInterpolationHigh;[sprite drawInRect:dest fromRect:NSMakeRect(0,0,sprite.size.width,sprite.size.height) operation:NSCompositingOperationSourceOver fraction:opacity respectFlipped:YES hints:nil];return;}
  if(!artwork)return;source.origin.y=artwork.size.height-source.origin.y-source.size.height;
  [artwork drawInRect:dest fromRect:source operation:NSCompositingOperationSourceOver fraction:opacity respectFlipped:YES hints:nil];
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
  painter.moduleBackground=[&](int stage,mockup::Rect r){
    NSImage* image=editableModuleSkins[@(stage)];if(!image)return;
    [image drawInRect:NSMakeRect(r.x,r.y,r.w,r.h) fromRect:NSZeroRect operation:NSCompositingOperationSourceOver fraction:1. respectFlipped:YES hints:nil];
    [[NSColor colorWithCalibratedWhite:0 alpha:stage==mockup::inputOptionsBackground?0.:stage==3?.45:.30] setFill];NSRectFillUsingOperation(NSMakeRect(r.x,r.y,r.w,r.h),NSCompositingOperationSourceOver);
  };
  painter.image=[&](int asset,mockup::Rect r,mockup::Rect source){
    NSString* key=[NSString stringWithFormat:@"%d:%.3f,%.3f,%.3f,%.3f",asset,source.x,source.y,source.w,source.h];
    CGImageRef slice=(__bridge CGImageRef)rasterSlices[key];
    if(!slice){
      NSImage* image=asset==mockup::Cables?rackCables:asset==mockup::Logo?rackLogo:asset==mockup::Atlas?rackAtlas:rackBackplate;if(!image)return;
      CGImageRef pixels=[image CGImageForProposedRect:nullptr context:nil hints:nil];
      if(!pixels)return;
      if(asset==mockup::Cables)source={0,0,double(CGImageGetWidth(pixels)),double(CGImageGetHeight(pixels))};
      CGImageRef cropped=CGImageCreateWithImageInRect(pixels,CGRectMake(source.x,source.y,source.w,source.h));if(!cropped)return;
      rasterSlices[key]=CFBridgingRelease(cropped);slice=(__bridge CGImageRef)rasterSlices[key];
    }
    CGContextRef context=[NSGraphicsContext currentContext].CGContext;CGContextSaveGState(context);
    CGContextTranslateCTM(context,r.x,r.y+r.h);CGContextScaleCTM(context,1,-1);
    if(asset==mockup::RoundScrew){CGContextAddEllipseInRect(context,CGRectMake(0,0,r.w,r.h));CGContextClip(context);}
    CGContextSetInterpolationQuality(context,kCGInterpolationHigh);
    CGContextDrawImage(context,CGRectMake(0,0,r.w,r.h),slice);CGContextRestoreGState(context);
  };
  painter.box=[](mockup::Rect r,skin::Rgb fill,skin::Rgb edge,double radius){box(NSMakeRect(r.x,r.y,r.w,r.h),C(fill),C(edge),radius);};
  painter.text=[&](const std::string& text,mockup::Rect r,double size,skin::Rgb color,bool center){
    NSString* str=[NSString stringWithUTF8String:text.c_str()];
    NSFont* fitFont=[NSFont fontWithName:@"HelveticaNeue-Medium" size:size];if(!fitFont)fitFont=[NSFont boldSystemFontOfSize:size];
    double measured=[str sizeWithAttributes:@{NSFontAttributeName:fitFont}].width;
    if(measured>r.w&&measured>0)size*=r.w/measured;
    NSString* key=[NSString stringWithFormat:@"%@|%.3f|%d,%d,%d|%d",str,size,color.r,color.g,color.b,center];
    NSAttributedString* cachedText=textCache[key];
    if(!cachedText){
      NSMutableParagraphStyle* style=[[NSMutableParagraphStyle alloc] init];style.alignment=center?NSTextAlignmentCenter:NSTextAlignmentLeft;style.lineBreakMode=NSLineBreakByTruncatingTail;
      NSFont* font=[NSFont fontWithName:@"HelveticaNeue-Medium" size:size];if(!font)font=[NSFont systemFontOfSize:size weight:NSFontWeightMedium];
      cachedText=[[NSAttributedString alloc] initWithString:str attributes:@{NSFontAttributeName:font,NSForegroundColorAttributeName:C(color),NSParagraphStyleAttributeName:style}];
      if(textCache.count>512)[textCache removeAllObjects];textCache[key]=cachedText;
    }
    [cachedText drawInRect:NSMakeRect(r.x,r.y+(r.h-size*1.2)/2,r.w,size*1.4)];
  };
  painter.line=[](double x,double y,double xx,double yy,skin::Rgb col,double width){NSBezierPath* path=[NSBezierPath bezierPath];[path moveToPoint:NSMakePoint(x,y)];[path lineToPoint:NSMakePoint(xx,yy)];[C(col) setStroke];path.lineWidth=width;[path stroke];};
  painter.polyline=[](const std::vector<std::pair<double,double>>& pts,skin::Rgb col,double width){if(pts.empty())return;NSBezierPath* path=[NSBezierPath bezierPath];[path moveToPoint:NSMakePoint(pts[0].first,pts[0].second)];for(size_t i=1;i<pts.size();++i)[path lineToPoint:NSMakePoint(pts[i].first,pts[i].second)];[C(col) setStroke];path.lineWidth=width;path.lineCapStyle=NSLineCapStyleRound;path.lineJoinStyle=NSLineJoinStyleRound;[path stroke];};
  painter.polygon=[](const std::vector<std::pair<double,double>>& pts,skin::Rgb col){if(pts.empty())return;NSBezierPath* path=[NSBezierPath bezierPath];[path moveToPoint:NSMakePoint(pts[0].first,pts[0].second)];for(size_t i=1;i<pts.size();++i)[path lineToPoint:NSMakePoint(pts[i].first,pts[i].second)];[path closePath];[C(col) setFill];[path fill];};
  painter.knob=[&](mockup::Rect r,double value){
    if(!knobFace){box(NSMakeRect(r.x,r.y,r.w,r.h),rgb(.7,.7,.7),cream(),r.w/2);return;}
    CGImageRef face=[knobFace CGImageForProposedRect:nullptr context:nil hints:nil];
    CGContextRef ctx=[NSGraphicsContext currentContext].CGContext;CGContextSaveGState(ctx);
    CGContextTranslateCTM(ctx,r.x+r.w/2,r.y+r.h/2);CGContextRotateCTM(ctx,(270*value-135)*::qg::tau/360.);
    CGContextScaleCTM(ctx,1,-1);CGContextSetInterpolationQuality(ctx,kCGInterpolationHigh);
    CGContextDrawImage(ctx,CGRectMake(-r.w/2,-r.h/2,r.w,r.h),face);CGContextRestoreGState(ctx);
  };
  painter.clip=[](mockup::Rect r){CGContextRef c=[NSGraphicsContext currentContext].CGContext;CGContextSaveGState(c);CGContextClipToRect(c,CGRectMake(r.x,r.y,r.w,r.h));};
  painter.unclip=[](){CGContextRestoreGState([NSGraphicsContext currentContext].CGContext);};
  bool rebuild=!staticScene||![scenePreset isEqualToString:presetName]||scenePage!=rackState.page||sceneScroll!=rackState.offset();scenePage=rackState.page;sceneScroll=rackState.offset();
  std::vector<ParamID> changedControls;
  std::array<int,5> selection{{[self tab],selectedLfo,selectedRepeat,selectedGate,selectedReslice}};
  if(selection!=sceneSelection){sceneSelection=selection;rebuild=true;}
  for(int id=0;id<kCount;++id)if(!isMonitor(id)){double v=owner->value(id);if(sceneValues[id]!=v){sceneValues[id]=v;changedControls.push_back(ParamID(id));if(id==lfoID(selectedLfo,lWave))rebuild=true;}}
  const int activeWave=modParam(kUiModWave0,selectedLfo);
  if(modWaveSelection([&](ParamID id){return owner->value(id);},selectedLfo)>0&&sceneValues[activeWave]!=owner->value(activeWave)){sceneValues[activeWave]=owner->value(activeWave);rebuild=true;}
  std::vector<mockup::Control> patches;
  if(!rebuild&&!changedControls.empty()){
    auto list=mockup::controls([&](ParamID id){return owner->value(id);},[self tab],selectedLfo,selectedRepeat,selectedGate,selectedReslice,rackState);
    for(auto id:changedControls){auto found=std::find_if(list.begin(),list.end(),[&](const mockup::Control& c){return c.id==id;});
      if(mockup::structural(id)){rebuild=true;break;}
      if(found!=list.end()&&found->kind!=mockup::Pad&&(found->id==kInputDeclick||found->id==kDeclickSensitivity||mockup::visible(found->r)))patches.push_back(*found);
    }
  }
  auto render=[&](mockup::RenderPass pass){mockup::render(painter,[&](ParamID id){return owner->value(id);},[&](ParamID id,double v){return std::string([owner->display(id,v) UTF8String]);},std::string([presetName UTF8String]),[self tab],selectedLfo,selectedRepeat,selectedGate,selectedReslice,&waveVisual,dragSlot,dropSlot,&motion,pass,rackState);};
  if(fullRedrawForInspection||dragSlot>=0||motion.routing){staticScene=nil;render(mockup::RenderPass::All);}else{
  if(rebuild){
    staticScene=[[NSImage alloc] initWithSize:NSMakeSize(canvasW,canvasH)];
    [staticScene lockFocusFlipped:YES];render(mockup::RenderPass::Static);[staticScene unlockFocus];
    scenePreset=[presetName copy];++staticBuilds;
  }else if(!patches.empty()){
    [staticScene lockFocusFlipped:YES];
    for(const auto& control:patches){
      CGContextRef context=[NSGraphicsContext currentContext].CGContext;CGContextSaveGState(context);
      CGContextClipToRect(context,CGRectMake(control.r.x,control.r.y,control.r.w,control.r.h));
      if(control.id!=kInputDeclick&&control.id!=kDeclickSensitivity)CGContextClipToRect(context,CGRectMake(51,mockup::bodyTop,982,mockup::height-mockup::bodyTop));
      mockup::restoreControlBackground(painter,control,[&](ParamID id){return owner->value(id);},rackState);
      mockup::renderControl(painter,control,[&](ParamID id){return owner->value(id);},[&](ParamID id,double v){return std::string([owner->display(id,v) UTF8String]);},selectedLfo);
      CGContextRestoreGState(context);++controlPatches;
    }
    [staticScene unlockFocus];
  }
  [staticScene drawInRect:NSMakeRect(0,0,canvasW,canvasH) fromRect:NSZeroRect operation:NSCompositingOperationCopy fraction:1. respectFlipped:YES hints:nil];
  render(mockup::RenderPass::Dynamic);
  const auto licenseText=aztec::license::status();
  if(!licenseText.empty()){
    mockup::Rect licenseButton{450,88,220,25};mockup::pill(painter,licenseButton);
    painter.text(licenseText+(aztec::license::activationRequired?"  ·  ACTIVATE":""),licenseButton,11,{230,233,237},true);
  }
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
  return folder;
}
- (BOOL)loadPresetURL:(NSURL*)url {
  NSNumber* size=nil;[url getResourceValue:&size forKey:NSURLFileSizeKey error:nil];if(!size||size.unsignedLongLongValue>65536)return NO;
  NSData* data=[NSData dataWithContentsOfURL:url];std::array<double,aztec::kCount> p{};
  if(!data||!data.length||!aztec::decodePreset(std::string((const char*)data.bytes,data.length),p))return NO;
  for(int id=0;id<aztec::kCount;++id)if(aztec::presetParameter(id))owner->edit(id,p[id]);
  presetName=[[url lastPathComponent] stringByDeletingPathExtension];owner->rememberPresetName(presetName);[self setNeedsDisplay:YES];return YES;
}
- (void)defaultPreset {auto p=aztec::initialParameters();for(int id=0;id<aztec::kCount;++id)if(aztec::presetParameter(id))owner->edit(id,p[id]);presetName=@"Default";owner->rememberPresetName(presetName);[self setNeedsDisplay:YES];}
- (NSArray<NSURL*>*)savedPresetFiles:(NSURL*)folder {
  NSArray<NSURL*>* files=[[NSFileManager defaultManager] contentsOfDirectoryAtURL:folder includingPropertiesForKeys:nil options:NSDirectoryEnumerationSkipsHiddenFiles error:nil];NSMutableArray<NSURL*>* saved=[NSMutableArray array];
  for(NSURL* url in files){if(![url.pathExtension.lowercaseString isEqualToString:@"gdspreset"])continue;std::string stem(url.lastPathComponent.stringByDeletingPathExtension.UTF8String);BOOL hide=NO;
    if(aztec::installedFactoryIndex(stem).first>=0){NSNumber* size=nil;[url getResourceValue:&size forKey:NSURLFileSizeKey error:nil];if(size&&size.unsignedLongLongValue<=65536){NSData* bytes=[NSData dataWithContentsOfURL:url];if(bytes)hide=aztec::untouchedFactoryPreset(stem,std::string((const char*)bytes.bytes,bytes.length));}}
    if(!hide)[saved addObject:url];
  }
  return [saved sortedArrayUsingComparator:^NSComparisonResult(NSURL* a,NSURL* b){return [a.lastPathComponent localizedStandardCompare:b.lastPathComponent];}];
}
- (void)choosePreset:(NSMenuItem*)item {
  if(item.tag==-2){[self defaultPreset];return;}NSURL* url=item.representedObject;if(item.tag==-1){[[NSWorkspace sharedWorkspace] openURL:url];return;}
  if(![self loadPresetURL:url]){NSAlert* alert=[[NSAlert alloc] init];alert.messageText=@"Cannot load this preset";alert.informativeText=@"Invalid or incompatible preset. No settings changed.";[alert runModal];}
}
- (void)presetMenu:(NSEvent*)event {
  NSURL* folder=[self presetFolder];if(!folder)return;NSMenu* menu=[[NSMenu alloc] initWithTitle:@"Presets"];
  NSMenuItem* def=[[NSMenuItem alloc] initWithTitle:@"Default" action:@selector(choosePreset:) keyEquivalent:@""];def.target=self;def.tag=-2;[menu addItem:def];
  NSMenu* saved=[[NSMenu alloc] initWithTitle:@"Saved Presets"];
  for(NSURL* url in [self savedPresetFiles:folder]){NSString* name=url.lastPathComponent.stringByDeletingPathExtension;NSMenuItem* item=[[NSMenuItem alloc] initWithTitle:name action:@selector(choosePreset:) keyEquivalent:@""];item.target=self;item.representedObject=url;item.state=[name isEqualToString:presetName]?NSControlStateValueOn:NSControlStateValueOff;[saved addItem:item];}
  if(saved.numberOfItems){NSMenuItem* group=[[NSMenuItem alloc] initWithTitle:@"Saved Presets" action:nullptr keyEquivalent:@""];group.submenu=saved;[menu addItem:group];}
  [menu addItem:[NSMenuItem separatorItem]];NSMenuItem* open=[[NSMenuItem alloc] initWithTitle:@"Open Preset Folder…" action:@selector(choosePreset:) keyEquivalent:@""];open.target=self;open.tag=-1;open.representedObject=folder;[menu addItem:open];[NSMenu popUpContextMenu:menu withEvent:event forView:self];
}
- (NSURL*)modulePresetFolder:(int)module {
  NSURL* root=[self presetFolder];if(!root)return nil;
  NSURL* folder=[[root URLByAppendingPathComponent:@"Modules" isDirectory:YES] URLByAppendingPathComponent:[NSString stringWithUTF8String:aztec::mockup::moduleNames[module]] isDirectory:YES];NSError* error=nil;
  if(![[NSFileManager defaultManager] createDirectoryAtURL:folder withIntermediateDirectories:YES attributes:nil error:&error]){NSAlert* alert=[[NSAlert alloc] init];alert.messageText=@"Cannot create the module preset folder";alert.informativeText=error.localizedDescription;[alert runModal];return nil;}return folder;
}
- (BOOL)loadModulePresetURL:(NSURL*)url module:(int)module {
  NSData* data=[NSData dataWithContentsOfURL:url];std::array<double,aztec::kCount> values{};
  if(!owner||!data||!data.length||!aztec::decodeModulePreset(std::string((const char*)data.bytes,data.length),module,values))return NO;
  for(int id=0;id<aztec::kCount;++id)if(aztec::modulePresetParameter(module,id))owner->edit(id,values[id]);[self setNeedsDisplay:YES];return YES;
}
- (void)chooseModulePresetFile:(NSMenuItem*)item {
  if(![self loadModulePresetURL:item.representedObject module:(int)item.tag]){NSAlert* alert=[[NSAlert alloc] init];alert.messageText=@"Cannot load this module preset";alert.informativeText=@"Invalid or incompatible module preset. No settings changed.";[alert runModal];}
}
- (void)preset:(BOOL)save { [self preset:save module:-1]; }
- (void)chooseModulePreset:(NSMenuItem*)item {
  if(!owner)return;int module=[item.representedObject intValue];
  if(item.tag==3){aztec::resetModulePreset(module,[&](aztec::ParamID id){return owner->info(id).defaultNormalizedValue;},[&](aztec::ParamID id,double v){owner->edit(id,v);});[self setNeedsDisplay:YES];}
  else if(item.tag==4){NSURL* folder=[self modulePresetFolder:module];if(folder)[[NSWorkspace sharedWorkspace] openURL:folder];}
  else [self preset:item.tag==1 module:module];
}
- (void)modulePresetMenu:(int)module event:(NSEvent*)event {
  NSURL* folder=[self modulePresetFolder:module];if(!folder)return;
  NSMenu* menu=[[NSMenu alloc] initWithTitle:@"Module presets"];
  NSMenuItem* defaults=[[NSMenuItem alloc] initWithTitle:@"Default" action:@selector(chooseModulePreset:) keyEquivalent:@""];defaults.target=self;defaults.tag=3;defaults.representedObject=@(module);[menu addItem:defaults];
  NSArray<NSURL*>* files=[[NSFileManager defaultManager] contentsOfDirectoryAtURL:folder includingPropertiesForKeys:@[NSURLIsRegularFileKey] options:NSDirectoryEnumerationSkipsHiddenFiles error:nil];
  files=[files sortedArrayUsingComparator:^NSComparisonResult(NSURL* a,NSURL* b){return [a.lastPathComponent localizedStandardCompare:b.lastPathComponent];}];
  for(NSURL* url in files){NSNumber* regular=nil;[url getResourceValue:&regular forKey:NSURLIsRegularFileKey error:nil];if(!regular.boolValue||![url.pathExtension.lowercaseString isEqualToString:@"gdmodule"])continue;
    NSMenuItem* item=[[NSMenuItem alloc] initWithTitle:url.lastPathComponent.stringByDeletingPathExtension action:@selector(chooseModulePresetFile:) keyEquivalent:@""];item.target=self;item.tag=module;item.representedObject=url;[menu addItem:item];}
  [menu addItem:[NSMenuItem separatorItem]];
  for(int i=1;i<=2;++i){NSMenuItem* item=[[NSMenuItem alloc] initWithTitle:i==1?@"Save module preset…":@"Load module preset…" action:@selector(chooseModulePreset:) keyEquivalent:@""];item.target=self;item.tag=i;item.representedObject=@(module);[menu addItem:item];}
  NSMenuItem* open=[[NSMenuItem alloc] initWithTitle:@"Open module preset folder…" action:@selector(chooseModulePreset:) keyEquivalent:@""];open.target=self;open.tag=4;open.representedObject=@(module);[menu addItem:open];
  [NSMenu popUpContextMenu:menu withEvent:event forView:self];
}
- (void)preset:(BOOL)save module:(int)module {
  if(!owner)return;NSURL* folder=module<0?[self presetFolder]:[self modulePresetFolder:module];if(!folder)return;NSError* error=nil;bool ok=false;
  if(save){NSSavePanel* panel=[NSSavePanel savePanel];panel.allowedFileTypes=module<0?@[@"gdspreset"]:@[@"gdmodule"];panel.nameFieldStringValue=module<0?@"GrainsDosage.gdspreset":[[NSString stringWithUTF8String:aztec::mockup::moduleNames[module]] stringByAppendingPathExtension:@"gdmodule"];panel.directoryURL=folder;
    if([panel runModal]!=NSModalResponseOK)return;
    auto get=[&](int id){return owner->value(id);};auto text=module<0?aztec::encodePreset(get):aztec::encodeModulePreset(module,get);NSData* data=[NSData dataWithBytes:text.data() length:text.size()];ok=[data writeToURL:panel.URL options:NSDataWritingAtomic error:&error];if(ok&&module<0){presetName=[panel.URL.lastPathComponent stringByDeletingPathExtension];owner->rememberPresetName(presetName);}
  }else{NSOpenPanel* panel=[NSOpenPanel openPanel];panel.allowedFileTypes=module<0?@[@"gdspreset"]:@[@"gdmodule"];panel.allowsMultipleSelection=NO;panel.canChooseDirectories=NO;panel.directoryURL=folder;
    if([panel runModal]!=NSModalResponseOK)return;
    if(module<0)ok=[self loadPresetURL:panel.URL];else ok=[self loadModulePresetURL:panel.URL module:module];
  }
  if(!ok){NSAlert* alert=[[NSAlert alloc] init];alert.messageText=save?@"Preset could not be saved":@"Preset could not be loaded";alert.informativeText=error?error.localizedDescription:@"Invalid or incompatible GrainsDosage preset. No settings changed.";[alert runModal];}
  [self setNeedsDisplay:YES];
}
// Cycle Default and user-saved files without modifying anything on disk.
- (void)stepPreset:(int)direction {
  if(!owner)return;NSURL* folder=[self presetFolder];if(!folder)return;NSArray<NSURL*>* files=[self savedPresetFiles:folder];int current=-1;
  for(NSUInteger i=0;i<files.count;++i)if([files[i].lastPathComponent.stringByDeletingPathExtension isEqualToString:presetName])current=int(i);
  int next=aztec::adjacentPreset(current,direction,int(files.count));if(next<0)[self defaultPreset];else [self loadPresetURL:files[NSUInteger(next)]];
}
- (void)mouseDown:(NSEvent*)event {
  if(!owner)return;[self.window makeFirstResponder:self];NSPoint p=[self logical:event];[self layoutControls];
  using namespace aztec;
  auto hit=[&](mockup::Rect r){return r.contains(p.x,p.y);};
  if(license::activationRequired&&!license::status().empty()&&hit({450,88,220,25})){
    NSAlert* alert=[[NSAlert alloc] init];alert.messageText=@"GrainsDosage — Offline activation";
    alert.informativeText=@"Send this complete Device Code to the publisher for a signed 14-day trial or permanent activation. Permanent licenses support up to 3 computers. Paste the full GDS2 code from your TXT file. No internet connection is needed.";
    const auto code=license::deviceCode();
    NSView* accessory=[[NSView alloc] initWithFrame:NSMakeRect(0,0,520,130)];
    NSTextField* device=[[NSTextField alloc] initWithFrame:NSMakeRect(0,88,520,40)];device.editable=NO;device.selectable=YES;device.stringValue=code.empty()?@"Device code unavailable":[NSString stringWithUTF8String:code.c_str()];[accessory addSubview:device];
    NSTextField* field=[[NSTextField alloc] initWithFrame:NSMakeRect(0,0,520,76)];field.placeholderString=@"Paste GDS2 activation code for this Mac…";[accessory addSubview:field];alert.accessoryView=accessory;
    [alert addButtonWithTitle:@"Activate"];[alert addButtonWithTitle:@"Cancel"];[alert addButtonWithTitle:@"Copy Device Code"];
    for(;;){auto response=[alert runModal];if(response==NSAlertThirdButtonReturn){if(!code.empty()){[[NSPasteboard generalPasteboard] clearContents];[[NSPasteboard generalPasteboard] setString:device.stringValue forType:NSPasteboardTypeString];}continue;}
     if(response==NSAlertFirstButtonReturn){std::string error;if(!license::activate(field.stringValue.UTF8String,error)){NSAlert* problem=[[NSAlert alloc] init];problem.messageText=@"Activation failed";problem.informativeText=[NSString stringWithUTF8String:error.c_str()];[problem runModal];}}break;}
    [self setNeedsDisplay:YES];return;
  }
  if(hit(mockup::previous)){[self stepPreset:-1];return;}
  if(hit(mockup::next)){[self stepPreset:1];return;}
  if(hit(mockup::preset)){[self presetMenu:event];return;}
  if(hit(mockup::load)){[self preset:NO];return;}
  if(hit(mockup::save)){[self preset:YES];return;}
  if(hit(mockup::randomAll)){randomizeSound(randomSeed,[&](ParamID id){return owner->value(id);},[&](ParamID id,double v){owner->edit(id,v);});staticScene=nil;motion.ready=false;[self layoutControls];[self setNeedsDisplay:YES];return;}
  if(rackState.page==1&&p.y>=mockup::bodyTop&&hit(rackState.position(mockup::zoomMenu))){
    NSMenu* menu=[[NSMenu alloc] initWithTitle:@"UI size"];
    for(int percent:{50,60,70,75,80,90,100}){NSMenuItem* item=[[NSMenuItem alloc] initWithTitle:[NSString stringWithFormat:@"%d%%",percent] action:@selector(chooseZoom:) keyEquivalent:@""];item.target=self;item.tag=percent;[menu addItem:item];}
    [NSMenu popUpContextMenu:menu withEvent:event forView:self];return;
  }
  auto value=[&](ParamID id){return owner->value(id);};
  int page=mockup::hitPage(p.x,p.y);if(page>=0){rackState.page=page;staticScene=nil;motion.ready=false;[self setNeedsDisplay:YES];return;}
  if(mockup::maxScroll(rackState)>0&&hit(mockup::scrollbar())){dragScroll=true;origin=p;scrollOrigin=rackState.offset();return;}
  if(rackState.page==0&&p.y>=mockup::bodyTop)for(int st:{0,2,3,4})if(hit(mockup::modulePresetRect(st,value,rackState))){[self modulePresetMenu:st event:event];return;}
  if(mockup::toggleResliceAdvance(p.x,p.y,value,rackState)){staticScene=nil;motion.ready=false;[self layoutControls];[self setNeedsDisplay:YES];return;}
  int route=mockup::hitRoute(p.x,p.y,value,rackState);if(route>=0){dragSlot=route;dropSlot=-1;routeMoved=false;origin=routePointer=p;routingAtDrag=mockup::routeChain(value);[self chooseTab:routingAtDrag[route]];[self setNeedsDisplay:YES];return;}
  int lfoHit=mockup::hitLfo(p.x,p.y,rackState);if(lfoHit>=0){selectedLfo=lfoHit;[self setNeedsDisplay:YES];return;}
  if(rackState.page==0&&p.y>=mockup::bodyTop&&hit(mockup::reslicePhraseRect(value,rackState))){reseedReslice(randomSeed,value,[&](ParamID id,double v){owner->edit(id,v);});[self setNeedsDisplay:YES];return;}
  if(rackState.page==0&&p.y>=mockup::bodyTop)for(int t:{0,2,3,4})if(hit(mockup::randomRect(t,value,rackState))){
    if(t<3)randomizeModule(t,selectedRepeat,randomSeed,value,[&](ParamID id,double v){owner->edit(id,v);});else if(t==3)randomizeReslice(randomSeed,value,[&](ParamID id,double v){owner->edit(id,v);});else for(int i=0;i<16;++i){randomSeed=randomSeed*1664525u+1013904223u;owner->edit(kGaterState0+i,(randomSeed>>31)?1.:0.);}
    [self setNeedsDisplay:YES];return;
  }
  int st=-1,step=mockup::hitStep(p.x,p.y,value,rackState,st);if(step>=0){
    ParamID id=st==2?kRepeatStep0+step:st==3?kResliceStep0+step:kGaterState0+step;
    if(event.modifierFlags&NSEventModifierFlagShift){owner->edit(id,owner->info(id).defaultNormalizedValue);[self setNeedsDisplay:YES];return;}
    if(event.clickCount>=2){[self editValue:id];return;}
    if(st==2){selectedRepeat=step;if(event.clickCount>=2)owner->edit(kRepeatStep0+step,value(kRepeatStep0+step)>.5?0.:1.);}
    if(st==3){selectedReslice=step;if(event.clickCount>=2)owner->edit(kResliceStep0+step,value(kResliceStep0+step)>.5?0.:1.);}
    if(st==4){selectedGate=step;if(event.modifierFlags&NSEventModifierFlagOption){double v=value(kGaterRelease0+step)>.5?0.:1.;owner->edit(kGaterRelease0+step,v);if(v>.5)owner->edit(kGaterState0+step,0.);}else{owner->edit(kGaterRelease0+step,0.);owner->edit(kGaterState0+step,value(kGaterState0+step)>.25?0.:1.);}}
    [self setNeedsDisplay:YES];return;
  }
  auto xy=rackState.position(mockup::xy);
  if(rackState.page==2&&p.y>=mockup::bodyTop&&hit(xy)){if(event.modifierFlags&NSEventModifierFlagShift){owner->edit(kXYX,owner->info(kXYX).defaultNormalizedValue);owner->edit(kXYY,owner->info(kXYY).defaultNormalizedValue);[self setNeedsDisplay:YES];return;}dragXY=true;owner->begin(kXYX);owner->begin(kXYY);auto v=mockup::xyValue(xy,p.x,p.y);owner->change(kXYX,v.first);owner->change(kXYY,v.second);[self setNeedsDisplay:YES];return;}
  for(const auto& c:controls)if((p.y>=mockup::bodyTop||c.id==kInputDeclick||c.id==kDeclickSensitivity)&&NSPointInRect(p,c.rect)){
    auto readout=mockup::valueRect({c.rect.origin.x,c.rect.origin.y,c.rect.size.width,c.rect.size.height},mockup::Kind(c.kind),c.id,c.label);
    if(event.modifierFlags&NSEventModifierFlagShift){owner->edit(c.id,owner->info(c.id).defaultNormalizedValue);[self setNeedsDisplay:YES];return;}
    if(event.clickCount>=2){if(!readout.contains(p.x,p.y)&&(c.kind==aztec::Knob||c.kind==aztec::Slider||c.kind==aztec::VSlider||c.kind==aztec::Pan)){owner->edit(c.id,owner->info(c.id).defaultNormalizedValue);[self setNeedsDisplay:YES];}else [self editValue:c.id];return;}
    if(readout.contains(p.x,p.y))return;
    if(c.kind==aztec::PanMode)owner->edit(c.id,std::clamp(int((p.x-c.rect.origin.x)/(c.rect.size.width/3.)),0,2)/2.);
    else if(c.kind==aztec::Pad||c.kind==aztec::Toggle)owner->edit(c.id,owner->value(c.id)>=.5?0.:1.);
    else if(c.kind==aztec::Select){
      NSMenu* menu=[[NSMenu alloc] initWithTitle:@"Select"];NSMenu* destination=menu;int n=owner->info(c.id).stepCount;
      for(int i=0;i<=n;++i){
        if(!aztec::availableMenuOption(c.id,i))continue;
        if(c.id==aztec::modParam(aztec::kLfoWaveChoice0,selectedLfo))if(const char* groupName=aztec::waveMenuGroup(i)){NSString* title=[NSString stringWithUTF8String:groupName];NSMenuItem* group=[[NSMenuItem alloc] initWithTitle:title action:nullptr keyEquivalent:@""];destination=[[NSMenu alloc] initWithTitle:title];group.submenu=destination;[menu addItem:group];}
        NSMenuItem* item=[[NSMenuItem alloc] initWithTitle:owner->display(c.id,double(i)/std::max(1,n),false) action:@selector(choose:) keyEquivalent:@""];
        item.target=self;item.tag=i;item.representedObject=@(c.id);item.state=int(std::round(owner->value(c.id)*n))==i?NSControlStateValueOn:NSControlStateValueOff;[destination addItem:item];
      }[NSMenu popUpContextMenu:menu withEvent:event forView:self];
    }else{dragID=int(c.id);dragKind=c.kind;dragRect=c.rect;origin=p;starting=owner->value(c.id);owner->begin(c.id);}
    [self setNeedsDisplay:YES];return;
  }
}
- (void)editValue:(aztec::ParamID)id {
 if(!owner)return;NSAlert* alert=[[NSAlert alloc] init];auto info=owner->info(id);
 alert.messageText=[NSString stringWithUTF8String:aztec::entryText(info.title).c_str()];
 alert.informativeText=@"Type a value in the displayed units, or the exact option name.";
 NSTextField* input=[[NSTextField alloc] initWithFrame:NSMakeRect(0,0,300,28)];input.stringValue=owner->display(id,owner->value(id));alert.accessoryView=input;
 [alert addButtonWithTitle:@"Apply"];[alert addButtonWithTitle:@"Cancel"];
 for(;;){[alert.window makeFirstResponder:input];[input selectText:nil];if([alert runModal]!=NSAlertFirstButtonReturn)return;double value=0;
  if(owner->parse(id,std::string(input.stringValue.UTF8String),value)){owner->edit(id,value);[self setNeedsDisplay:YES];return;}
  alert.informativeText=@"Invalid value. Enter a value within the control's range, using its displayed units or option name.";
 }
}
- (void)rightMouseDown:(NSEvent*)event {
 if(!owner)return;[self layoutControls];auto p=[self logical:event];
 for(const auto& c:controls)if((p.y>=aztec::mockup::bodyTop||c.id==aztec::kInputDeclick||c.id==aztec::kDeclickSensitivity)&&NSPointInRect(p,c.rect)){[self editValue:c.id];return;}
}
- (void)mouseDragged:(NSEvent*)event {
  if(!owner)return;NSPoint p=[self logical:event];
  if(dragScroll){auto bar=aztec::mockup::scrollbar(),thumb=aztec::mockup::scrollThumb(rackState);[self setPageScroll:scrollOrigin+(p.y-origin.y)*aztec::mockup::maxScroll(rackState)/std::max(1.,bar.h-thumb.h)];return;}
  if(dragSlot>=0){routePointer=p;if(std::hypot(p.x-origin.x,p.y-origin.y)>8)routeMoved=true;if(routeMoved)dropSlot=aztec::mockup::routeInsertion(p.x,p.y,[&](aztec::ParamID id){return owner->value(id);},rackState);[self setNeedsDisplay:YES];return;}
  if(dragXY){auto xy=rackState.position(aztec::mockup::xy);auto v=aztec::mockup::xyValue(xy,p.x,p.y);owner->dragChange(aztec::kXYX,v.first);owner->dragChange(aztec::kXYY,v.second);refresh.request();if(refresh.interactionDue(aztec::EditorRefresh::now()))[self setNeedsDisplay:YES];return;}
  if(dragID<0)return;double delta=(dragKind==aztec::Slider||dragKind==aztec::Pan)?(p.x-origin.x)/dragRect.size.width:(origin.y-p.y)/(dragKind==aztec::VSlider?std::max(1.,dragRect.size.height-42):180.);
  if(event.modifierFlags&NSEventModifierFlagShift)delta*=.1;
  starting=std::clamp(starting+delta,0.,1.);origin=p;
  owner->dragChange(aztec::ParamID(dragID),starting);refresh.request();if(refresh.interactionDue(aztec::EditorRefresh::now()))[self setNeedsDisplay:YES];
}
- (void)mouseUp:(NSEvent*)event {
  (void)event;if(!owner)return;
  if(dragSlot>=0&&dropSlot>=0&&dropSlot!=dragSlot&&dropSlot!=dragSlot+1)owner->edit(aztec::kRoutingOrder,aztec::mockup::moveRoute(routingAtDrag,dragSlot,dropSlot));
  if(dragID>=0)owner->end(aztec::ParamID(dragID));if(dragXY){owner->end(aztec::kXYX);owner->end(aztec::kXYY);}
  dragID=dragSlot=dropSlot=-1;dragXY=routeMoved=dragScroll=false;animationFrames=12;[self setNeedsDisplay:YES];
}
- (void)scrollWheel:(NSEvent*)event {
  if(!owner||dragID>=0||dragXY)return;NSPoint p=[self logical:event];
  if(event.modifierFlags&NSEventModifierFlagOption){[self layoutControls];for(const auto& c:controls)if(p.y>=aztec::mockup::bodyTop&&NSPointInRect(p,c.rect)&&(c.kind==aztec::Knob||c.kind==aztec::Slider||c.kind==aztec::VSlider)){int n=owner->info(c.id).stepCount;double step=n?1./n:.01;if(event.modifierFlags&NSEventModifierFlagShift)step*=.1;owner->edit(c.id,owner->value(c.id)+(event.scrollingDeltaY>0?step:-step));[self setNeedsDisplay:YES];return;}}
  double delta=event.scrollingDeltaY*(event.hasPreciseScrollingDeltas?1.:18.);[self setPageScroll:rackState.offset()-delta];
}

@end
namespace aztec {
// No "using namespace qg" here: inside this namespace an unqualified qg would
// find aztec::qg first (shadowing), so global qg names must stay ::qg-qualified.
Editor::Editor(EditController* c,std::shared_ptr<EditorState> state):controller_(c),editorState_(state?state:std::make_shared<EditorState>()){controller_->addRef();rect=ViewRect(0,0,int(mockup::width*.90),int(mockup::height*.90));(void)::qg::waveBank();}
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
  aztec::license::firstOpen();
  static bool skinRestored=false;if(!skinRestored){skinRestored=true;aztec::theme::loadSkinFile(aztec::theme::defaultPath());}
  surface_=[[GrainsSurface alloc] initWithFrame:NSMakeRect(0,0,rect.getWidth(),rect.getHeight())];surface_->owner=this;[surface_ restorePresetName:[NSString stringWithUTF8String:editorState_->presetName.c_str()]];
  [(__bridge NSView*)parent addSubview:surface_];[surface_ setNeedsDisplay:YES];zoom(.90);return kResultOk;
}
tresult Editor::removed(){if(surface_){[surface_ stop];surface_->owner=nullptr;[surface_ removeFromSuperview];surface_=nil;}return CPluginView::removed();}
tresult Editor::onSize(ViewRect* size){if(!size)return kInvalidArgument;auto result=CPluginView::onSize(size);if(surface_){[surface_ setFrameSize:NSMakeSize(size->getWidth(),size->getHeight())];[surface_ setNeedsDisplay:YES];}return result;}
tresult Editor::checkSizeConstraint(ViewRect* size){if(!size)return kInvalidArgument;double scale=std::clamp(double(size->getWidth())/canvasW,.5,1.);size->right=size->left+int32(std::round(canvasW*scale));size->bottom=size->top+int32(std::round(canvasH*scale));return kResultTrue;}
void Editor::zoom(double scale){if(plugFrame){ViewRect next(0,0,int32(canvasW*scale),int32(canvasH*scale));plugFrame->resizeView(this,&next);}}
IPlugView* createEditor(EditController* controller,std::shared_ptr<EditorState> state){return new Editor(controller,state);}
}

