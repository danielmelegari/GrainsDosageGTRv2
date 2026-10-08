#include "src/mockup_ui.h"
#include "src/factory_presets.h"
#include <cassert>
#include <iostream>
#include <set>
using namespace aztec;
int main(){int count=0;for(int route=0;route<120;++route)for(int page=0;page<3;++page)for(int mode=0;mode<16;++mode){auto p=initialParameters();p[kRoutingOrder]=(route+1)/120.;p[kPanMode]=mode&1;p[kReverbSource]=(mode>>1)&1;p[kReverbModel]=(mode>>2)&1;p[kFilterModel]=(mode&1)?7/18.:0;p[kFilterSeqMode]=(mode>>3)&1;auto value=[&](ParamID id){return p[id];};mockup::RackState s;s.page=page;
 auto controls=mockup::controls(value,0,mode%4,mode%16,mode%16,mode%16,s);std::set<ParamID> ids;int pads=0;
 for(const auto& c:controls){assert(c.r.x>=54&&c.r.x+c.r.w<=1028&&c.r.w>0&&c.r.h>0);assert(c.id<kCount&&!isMonitor(c.id)&&ids.insert(c.id).second);assert(c.r.y>=0&&c.r.y+c.r.h<=mockup::contentBottom(page));if(c.kind==mockup::Pad)++pads;}
 assert(pads==(page==1&&p[kReverbSource]<.5?16:0));
 for(size_t i=0;i<controls.size();++i)for(size_t j=i+1;j<controls.size();++j){auto a=controls[i].r,b=controls[j].r;if(a.x<b.x+b.w&&b.x<a.x+a.w&&a.y<b.y+b.h&&b.y<a.y+a.h){std::cerr<<controls[i].label<<" ("<<controls[i].id<<") overlaps "<<controls[j].label<<" ("<<controls[j].id<<")\n";return 1;}}
 for(double offset:{0.,mockup::maxScroll(s)}){s.scroll[page]=offset;auto scrolled=mockup::controls(value,0,mode%4,mode%16,mode%16,mode%16,s);for(size_t i=0;i<controls.size();++i){bool header=controls[i].id==kInputDeclick||controls[i].id==kDeclickSensitivity;assert(scrolled[i].r.y==controls[i].r.y-(header?0:offset));}if(page==0){auto chain=mockup::routeChain(value);for(int i=0;i<5;++i){auto r=mockup::moduleRect(chain[i],value,s);assert(r.x==54&&r.w==974);if(i)assert(r.y>mockup::moduleRect(chain[i-1],value,s).y);if(r.y>=mockup::bodyTop&&r.y+35<mockup::height)assert(mockup::hitRoute(r.x+100,r.y+30,value,s)==i);}}}
 ++count;}
 mockup::RackState s;mockup::scrollBy(s,1e6);assert(s.offset()==mockup::maxScroll(s));s.page=2;assert(s.offset()==0);mockup::scrollBy(s,-1e6);assert(s.offset()==0);assert(mockup::xy.w==mockup::xy.h);
 std::cout<<"PASS: "<<count<<" page/routing variants, control preservation, non-overlap, fixed header, scroll bounds and square Morph\n";
}
