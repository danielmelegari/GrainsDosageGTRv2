#include "src/preset_browser.h"
#include <cassert>
#include <iostream>
int main(){using namespace aztec;
 PresetBrowser b;b.show({{"default","Default","Init"},{"a.gdspreset","A","User presets"},{"b.gdspreset","B","User presets"}});
 assert(b.open&&b.filtered().size()==3);
 assert(b.click(921,366)==-2);assert(b.favorites.count("a.gdspreset")); // heart only, no load
 assert(b.click(170,366)==-1);assert(b.category==1&&b.filtered()==std::vector<int>{1});
 assert(b.click(440,329)==1);assert(b.open); // audition without closing
 auto saved=b.encodedFavorites();PresetBrowser next;next.readFavorites(saved);assert(next.favorites==b.favorites);
 assert(b.click(921,329)==-2);assert(b.filtered().empty());assert(b.first==0);
 assert(b.click(20,20)==-1&&b.open);assert(b.click(927,267)==-1&&!b.open);
 std::vector<PresetBrowser::Entry> many;for(int i=0;i<50;++i)many.push_back({std::to_string(i),"Preset "+std::to_string(i),"Category "+std::to_string(i)});
 b.show(many);b.scroll(100);assert(b.first==36);assert(b.click(440,329)==36);b.scroll(-100);assert(b.first==0);
 b.scroll(100,true);assert(b.categoryFirst==38);b.click(170,329);assert(b.category==38&&b.first==0&&b.filtered()==std::vector<int>{36});
 b.favorites={"écho \"wide\".gdspreset","space and \\ slash"};next.readFavorites(b.encodedFavorites());assert(next.favorites==b.favorites);
 // Rendering never changes browser or favorite state, and every primitive stays finite.
 mockup::Painter p;int labels=0,hearts=0;
 p.box=[](auto r,auto,auto,double){assert(r.w>0&&r.h>0);};p.text=[&](auto&,auto,double,auto,bool){++labels;};p.line=[](double,double,double,double,auto,double){};
 p.polygon=[&](const auto& points,auto){++hearts;for(auto pt:points)assert(std::isfinite(pt.first)&&std::isfinite(pt.second));};p.polyline=[&](const auto& points,auto,double){++hearts;for(auto pt:points)assert(std::isfinite(pt.first)&&std::isfinite(pt.second));};
 auto before=b.encodedFavorites();b.draw(p,"Preset 36");assert(labels>0&&hearts==1&&before==b.encodedFavorites());
 std::cout<<"Preset browser filtering, hearts, persistence, paging and rendering passed\n";
}
