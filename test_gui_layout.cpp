#include "src/parameters.h"
#include <iostream>
#include <vector>
#include <string>
struct R{double x,y,w,h;int id;std::string label;};
using namespace aztec;
int main(){
 const double slotX[]={16,452,888};
 for(int currentTab=0;currentTab<5;++currentTab)for(int order=0;order<6;++order)for(int mode=0;mode<16;++mode)for(int model=0;model<19;++model)for(int step=0;step<16;++step){
 int perms[6][3]={{0,1,2},{0,2,1},{1,0,2},{1,2,0},{2,0,1},{2,1,0}};
 auto slot=[&](int i){return perms[order][i];};
 int selectedLfo=step%4,selectedRepeat=step,selectedGate=step,selectedReslice=step;
 auto value=[&](int id){if(id==kFilterModel)return model/18.; if(id==kPanMode)return double(mode&1); if(id==kReverbModel)return double((mode>>1)&1); if(id==kReverbSource)return double((mode>>2)&1); if(id==kFilterSeqMode)return double((mode>>3)&1); if(id==kModuleOrder)return order/5.; return 0.;};
 std::vector<R> rects;
 #define ADD(id,x,y,w,h,kind,label) do {double a=(x),b=(y),c=(w),d=(h);if(c<=0||d<=0||a<0||b<0||a+c>1632||b+d>1518){std::cerr<<label<<" out of bounds "<<a<<","<<b<<"\n";return 1;}if(int(id)<0||int(id)>=kCount||isMonitor(int(id))){std::cerr<<"Invalid editable parameter "<<int(id)<<"\n";return 1;}rects.push_back({a,b,c,d,int(id),label});}while(0)
 #include "src/editor_layout_win.inl"
 #undef ADD
 for(size_t i=0;i<rects.size();++i)for(size_t j=i+1;j<rects.size();++j){auto a=rects[i],b=rects[j];if(a.id==b.id){std::cerr<<"Duplicate control parameter\n";return 1;}if(a.x<b.x+b.w&&b.x<a.x+a.w&&a.y<b.y+b.h&&b.y<a.y+a.h){std::cerr<<a.label<<" overlaps "<<b.label<<"\n";return 1;}}
 }
 std::cout<<"PASS: 145920 layouts; bounds, positive geometry, non-overlap and unique editable parameter IDs\n";
}

