#include "src/preset_folders.h"
#include "src/preset_browser.h"
#include <cassert>
#include <fstream>
#include <chrono>
#include <iostream>
using namespace aztec;
int main(){
 auto root=std::string("preset-folder-test-")+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
#ifdef _WIN32
 assert(CreateDirectoryW(presetFolders::wide(root).c_str(),nullptr));
#else
 assert(mkdir(root.c_str(),0700)==0);
#endif
 for(auto name:{"","..","../escape","a/b","a\\b","CON","COM1.txt","a:","bad.","bad "})assert(!presetFolders::validName(name));
 assert(presetFolders::validName("Ambient textures"));assert(presetFolders::validName("échos"));
 assert(presetFolders::create(root,"","Ambient"));assert(!presetFolders::create(root,"","Ambient"));
 assert(presetFolders::create(root,"Ambient","Slow"));assert(presetFolders::create(root,"","Empty"));
 assert(!presetFolders::create(root,"../escape","No"));assert(!presetFolders::create(root,"","Modules"));
 std::ofstream(root+"/One.gdspreset")<<"original";std::ofstream(root+"/Ambient/One.gdspreset")<<"existing";
 assert(!presetFolders::move(root,"One.gdspreset","Ambient/One.gdspreset"));
 std::string data;std::ifstream(root+"/One.gdspreset")>>data;assert(data=="original");std::ifstream(root+"/Ambient/One.gdspreset")>>data;assert(data=="existing");
 assert(presetFolders::move(root,"One.gdspreset","Ambient/Slow/One.gdspreset"));
 assert(presetFolders::move(root,"Ambient","Atmospheres"));
 assert(!presetFolders::move(root,"Atmospheres","Empty"));assert(!presetFolders::move(root,"Atmospheres","Modules"));
 presetFolders::Listing listing;presetFolders::scan(root,listing);assert(listing.folders.size()==3&&listing.files.size()==2);
 assert(std::find(listing.files.begin(),listing.files.end(),"Atmospheres/Slow/One.gdspreset")!=listing.files.end());
 PresetBrowser b;b.show({{"Atmospheres/Slow/One.gdspreset","One","Folder: Atmospheres/Slow"}},listing.folders);b.selectFolder("Empty");assert(b.selectedFolder()=="Empty"&&b.filtered().empty());
 assert(b.click(700,265)==-4);assert(b.click(800,265)==-5);
 b.selectFolder("Atmospheres/Slow");assert(b.click(870,330)==-6&&b.moveIndex==0);
 b.favorites={"Atmospheres/Slow/One.gdspreset","Atmospheres2/Another.gdspreset"};b.moved("Atmospheres","Clouds");assert(b.favorites.count("Clouds/Slow/One.gdspreset")&&b.favorites.count("Atmospheres2/Another.gdspreset"));
 PresetBrowser reopened;reopened.readFavorites(b.encodedFavorites());assert(reopened.favorites==b.favorites);
#ifndef _WIN32
 assert(symlink("Atmospheres",(root+"/Alias").c_str())==0);assert(!presetFolders::create(root,"Alias","No"));assert(!presetFolders::move(root,"Alias","Other"));listing={};presetFolders::scan(root,listing);assert(listing.folders.size()==3);unlink((root+"/Alias").c_str());
#endif
 for(const auto& file:listing.files)std::remove((root+"/"+file).c_str());
 for(auto i=listing.folders.rbegin();i!=listing.folders.rend();++i){
#ifdef _WIN32
 assert(RemoveDirectoryW(presetFolders::wide(root+"/"+*i).c_str()));
#else
 assert(rmdir((root+"/"+*i).c_str())==0);
#endif
 }
#ifdef _WIN32
 assert(RemoveDirectoryW(presetFolders::wide(root).c_str()));
#else
 assert(rmdir(root.c_str())==0);
#endif
 std::cout<<"Folder creation, nesting, rename, no-overwrite moves, favorite remapping and alias exclusion passed\n";
}
