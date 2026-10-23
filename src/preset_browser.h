#pragma once
#include "mockup_ui.h"
#include "preset_catalog.h"
#include "preset_folders.h"
#include <set>
#include <sstream>
#include <iomanip>
namespace aztec {
// UI-only state. No directory access, preset decoding or persistence during paint.
struct PresetBrowser {
  struct Entry {std::string key,name,category;};
  bool open=false;
  int category=0,first=0,categoryFirst=0,moveIndex=-1;
  std::vector<std::string> folders;
  std::vector<Entry> entries;
  std::vector<std::string> categories;
  std::set<std::string> favorites;
  static constexpr int rows=14;
  static constexpr mockup::Rect panel{120,235,843,650},close{911,251,34,34};
  void show(std::vector<Entry> list,std::vector<std::string> paths={}){
    folders=std::move(paths);
    entries=std::move(list);categories={"All presets","Favorites"};
    for(const auto& e:entries)if(std::find(categories.begin()+2,categories.end(),e.category)==categories.end())categories.push_back(e.category);
    for(const auto& path:folders){auto label="Folder: "+path;if(std::find(categories.begin(),categories.end(),label)==categories.end())categories.push_back(label);}
    category=first=categoryFirst=0;open=true;
  }
  std::string selectedFolder()const{if(category>=2&&categories[category].rfind("Folder: ",0)==0)return categories[category].substr(8);return {};}
  void selectFolder(const std::string& path){auto i=std::find(categories.begin(),categories.end(),"Folder: "+path);if(i!=categories.end()){category=int(i-categories.begin());categoryFirst=std::max(0,category-rows+1);first=0;}}
  void moved(const std::string& from,const std::string& to){std::set<std::string> updated;for(const auto& key:favorites){if(key==from)updated.insert(to);else if(key.rfind(from+"/",0)==0)updated.insert(to+key.substr(from.size()));else updated.insert(key);}favorites=std::move(updated);}
  static std::string categoryFor(const std::string& name){auto i=installedFactoryIndex(name);return i.first<0?"User presets":std::string(presetBankNames[i.first])+" / "+bankCategoryName(i.first,i.second/16);}
  std::vector<int> filtered()const{std::vector<int> out;for(size_t i=0;i<entries.size();++i)if(category==0||(category==1?favorites.count(entries[i].key)>0:entries[i].category==categories[category]))out.push_back(int(i));return out;}
  void scroll(int amount,bool left=false){if(left)categoryFirst=std::clamp(categoryFirst+amount,0,std::max(0,int(categories.size())-rows));else first=std::clamp(first+amount,0,std::max(0,int(filtered().size())-rows));}
  std::string encodedFavorites()const{std::ostringstream out;for(const auto& key:favorites)out<<std::quoted(key)<<'\n';return out.str();}
  void readFavorites(const std::string& text){favorites.clear();std::istringstream in(text);std::string key;while(in>>std::quoted(key))favorites.insert(key);}
  // Negative actions: -1 none, -2 favorite, -3 reveal, -4 create, -5 rename, -6 move.
  int click(double x,double y){
    if(close.contains(x,y)){open=false;return -1;}
    if(mockup::Rect{640,251,125,34}.contains(x,y))return -4;
    if(mockup::Rect{777,251,125,34}.contains(x,y)&&!selectedFolder().empty())return -5;
    if(mockup::Rect{140,840,210,28}.contains(x,y))return -3;
    if(y>=314&&y<314+rows*35){int row=int((y-314)/35);
      if(x>=140&&x<375&&categoryFirst+row<int(categories.size())){category=categoryFirst+row;first=0;return -1;}
      auto visible=filtered();int n=first+row;
      if(x>=395&&x<944&&n<int(visible.size())){int index=visible[n];if(x>=899){const auto& key=entries[index].key;if(!favorites.erase(key))favorites.insert(key);scroll(0);return -2;}if(x>=855){if(entries[index].key!="default"){moveIndex=index;return -6;}return -1;}return index;}
    }
    if(y>=810&&y<838){if(x>=140&&x<375)scroll(x<257?-rows:rows,true);else if(x>=395&&x<944)scroll(x<669?-rows:rows);}
    return -1;
  }
  void draw(mockup::Painter& p,const std::string& selected)const{
    using mockup::Rect;using skin::Rgb;
    p.text("PRESETS",{145,250,400,36},22,{230,237,239},false);
    p.box({640,251,125,34},{44,70,74},{56,92,96},5);p.text("+ New folder",{640,251,125,34},12,{203,235,225},true);
    if(!selectedFolder().empty()){p.box({777,251,125,34},{44,70,74},{56,92,96},5);p.text("Rename folder",{777,251,125,34},12,{203,235,225},true);}
    p.text("×",close,25,{230,237,239},true);
    p.text("CATEGORIES",{145,291,220,20},11,{143,170,175},false);
    auto visible=filtered();
    p.text(std::to_string(visible.size())+" presets",{400,291,490,20},11,{143,170,175},false);
    p.line(383,299,383,836,{75,91,96},1);
    for(int r=0;r<rows;++r){double y=314+r*35;
      int c=categoryFirst+r;if(c<int(categories.size())){if(c==category)p.box({140,y,233,32},{44,70,74},{44,70,74},5);p.text(categories[c],{150,y+4,214,25},13,c==category?Rgb{150,244,216}:Rgb{203,213,215},false);}
      int n=first+r;if(n>=int(visible.size()))continue;const auto& e=entries[visible[n]];
      if(e.key==selected)p.box({395,y,549,32},{42,62,69},{56,92,96},5);
      p.text(e.name,{408,y+4,438,25},14,{224,232,234},false);
      if(e.key!="default")p.text("…",{855,y,42,32},20,{173,202,207},true);
      // Vector heart avoids platform font fallback and makes the whole cell clickable.
      std::vector<std::pair<double,double>> heart;
      for(int i=0;i<=40;++i){double t=i*6.283185307179586/40.;double sx=std::sin(t);heart.push_back({921+sx*sx*sx*11,y+17-(13*std::cos(t)-5*std::cos(2*t)-2*std::cos(3*t)-std::cos(4*t))*.7});}
      if(favorites.count(e.key))p.polygon(heart,{248,112,150});else p.polyline(heart,{152,168,174},1.5);
    }
    if(visible.empty())p.text(category==1?"Click a heart to add a favorite.":"No presets in this category.",{409,360,480,35},15,{170,190,194},false);
    if(categories.size()>rows)p.text("‹ Previous       Next ›",{145,810,224,28},12,{173,202,207},true);
    if(visible.size()>rows)p.text("‹ Previous     "+std::to_string(first+1)+"–"+std::to_string(std::min(first+rows,int(visible.size())))+"     Next ›",{400,810,540,28},12,{173,202,207},true);
    p.text("Open preset folder…",{145,840,210,28},12,{173,202,207},false);
  }
};
}
