#pragma once
#include <string>
#include <vector>
#include <algorithm>
#include <cerrno>
#include <cstring>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstdio>
#ifdef __linux__
#include <sys/syscall.h>
#include <fcntl.h>
#endif
#endif
namespace aztec::presetFolders {
struct Listing {std::vector<std::string> folders,files;};
inline bool validName(const std::string& n){
 if(n.empty()||n.size()>120||n.front()=='.'||n.back()=='.'||n.back()==' ')return false;
 for(unsigned char c:n)if(c<32||std::string("/\\:*?\"<>|").find(char(c))!=std::string::npos)return false;
 std::string stem=n.substr(0,n.find('.'));for(char& c:stem)if(c>='a'&&c<='z')c-=32;
 if(stem=="CON"||stem=="PRN"||stem=="AUX"||stem=="NUL")return false;
 if(stem.size()==4&&(stem.substr(0,3)=="COM"||stem.substr(0,3)=="LPT")&&stem[3]>='1'&&stem[3]<='9')return false;
 return true;
}
inline bool safeRelative(const std::string& path){if(path.empty())return true;size_t start=0;while(start<path.size()){auto end=path.find('/',start);auto part=path.substr(start,end==std::string::npos?end:end-start);if(part.empty()||part=="."||part==".."||part.find('\\')!=std::string::npos)return false;if(end==std::string::npos)return true;start=end+1;}return false;}
inline std::string parent(const std::string& path){auto i=path.rfind('/');return i==std::string::npos?"":path.substr(0,i);}
inline std::string remap(const std::string& key,const std::string& from,const std::string& to){return key==from?to:key.rfind(from+"/",0)==0?to+key.substr(from.size()):key;}
inline std::string leaf(const std::string& path){return path.substr(path.rfind('/')==std::string::npos?0:path.rfind('/')+1);}
inline std::string join(const std::string& a,const std::string& b){return a.empty()?b:b.empty()?a:a+"/"+b;}
#ifdef _WIN32
inline std::wstring wide(const std::string& s){int n=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,s.data(),int(s.size()),nullptr,0);std::wstring out(n,L'\0');if(n)MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,s.data(),int(s.size()),out.data(),n);return out;}
inline std::string utf8(const std::wstring& s){int n=WideCharToMultiByte(CP_UTF8,0,s.data(),int(s.size()),nullptr,0,nullptr,nullptr);std::string out(n,'\0');if(n)WideCharToMultiByte(CP_UTF8,0,s.data(),int(s.size()),out.data(),n,nullptr,nullptr);return out;}
#endif
// Do not follow aliases/symlinks or Windows junctions while organizing presets.
inline bool directory(const std::string& path){
#ifdef _WIN32
 auto a=GetFileAttributesW(wide(path).c_str());return a!=INVALID_FILE_ATTRIBUTES&&(a&FILE_ATTRIBUTE_DIRECTORY)&&!(a&FILE_ATTRIBUTE_REPARSE_POINT);
#else
 struct stat st{};return lstat(path.c_str(),&st)==0&&S_ISDIR(st.st_mode);
#endif
}
inline bool inside(const std::string& root,const std::string& relative){if(!safeRelative(relative)||!directory(root))return false;std::string path=root;size_t i=0;while(i<relative.size()){auto end=relative.find('/',i);path=join(path,relative.substr(i,end==std::string::npos?end:end-i));if(!directory(path))return false;if(end==std::string::npos)break;i=end+1;}return true;}
inline void scan(const std::string& root,Listing& out,const std::string& relative="",int depth=0){
 if(depth>8||!inside(root,relative))return;
 std::vector<std::pair<std::string,bool>> entries;
#ifdef _WIN32
 WIN32_FIND_DATAW d{};HANDLE h=FindFirstFileW(wide(join(join(root,relative),"*")).c_str(),&d);if(h==INVALID_HANDLE_VALUE)return;
 do{if(d.dwFileAttributes&FILE_ATTRIBUTE_REPARSE_POINT)continue;entries.push_back({utf8(d.cFileName),bool(d.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)});}while(FindNextFileW(h,&d));FindClose(h);
#else
 DIR* d=opendir(join(root,relative).c_str());if(!d)return;while(auto* e=readdir(d)){std::string name=e->d_name;struct stat st{};if(lstat(join(join(root,relative),name).c_str(),&st)||(!S_ISDIR(st.st_mode)&&!S_ISREG(st.st_mode)))continue;entries.push_back({name,S_ISDIR(st.st_mode)});}closedir(d);
#endif
 std::sort(entries.begin(),entries.end());
 for(const auto& entry:entries){const auto& name=entry.first;if(name.empty()||name.front()=='.'||(relative.empty()&&name=="Modules"))continue;auto path=join(relative,name);
  if(entry.second){out.folders.push_back(path);scan(root,out,path,depth+1);}else{auto ext=name.substr(name.size()>10?name.size()-10:0);for(char& c:ext)if(c>='A'&&c<='Z')c+=32;if(ext==".gdspreset")out.files.push_back(path);}
 }
}
inline bool create(const std::string& root,const std::string& parentPath,const std::string& name){
 if(std::count(parentPath.begin(),parentPath.end(),'/')>=7||!validName(name)||!inside(root,parentPath)||(parentPath.empty()&&name=="Modules"))return false;auto path=join(join(root,parentPath),name);
#ifdef _WIN32
 return CreateDirectoryW(wide(path).c_str(),nullptr)!=0;
#else
 return mkdir(path.c_str(),0755)==0;
#endif
}
inline bool move(const std::string& root,const std::string& from,const std::string& to){
 if(to=="Modules"||from=="Modules"||from.empty()||to.empty()||!safeRelative(from)||!safeRelative(to)||!inside(root,parent(from))||!inside(root,parent(to)))return false;
 auto a=join(root,from),b=join(root,to);
#ifdef _WIN32
 auto attributes=GetFileAttributesW(wide(a).c_str());if(attributes==INVALID_FILE_ATTRIBUTES||(attributes&FILE_ATTRIBUTE_REPARSE_POINT))return false;
 return MoveFileW(wide(a).c_str(),wide(b).c_str())!=0;
#else
 struct stat st{};if(lstat(a.c_str(),&st)||S_ISLNK(st.st_mode))return false;
#ifdef __APPLE__
 return renamex_np(a.c_str(),b.c_str(),RENAME_EXCL)==0;
#else
 return syscall(SYS_renameat2,AT_FDCWD,a.c_str(),AT_FDCWD,b.c_str(),1)==0;
#endif
#endif
}
}
