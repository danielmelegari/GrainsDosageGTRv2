#pragma once
#include "license.h"
#include <spawn.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <chrono>
#include <thread>
#include <sstream>
#ifdef __APPLE__
#include <crt_externs.h>
#else
extern char** environ;
#endif
namespace aztec::license::automatic {
// Worker-thread only. A stalled Security service cannot indefinitely hold up
// plugin unloading: kill and reap our helper after the bounded deadline.
inline State runHelper(const std::string& executable,bool create,int64_t now,int64_t minimum,int64_t& first,int64_t& last,
                       std::chrono::milliseconds timeout=std::chrono::milliseconds(3000)){
 if(executable.empty())return State::StorageError;
 int pipefd[2];if(pipe(pipefd)!=0)return State::StorageError;
 // Some DAWs close stdin/stdout. Keep our pipe clear of the stdio slots.
 for(int i=0;i<2;++i){
  if(pipefd[i]<3){int fd=fcntl(pipefd[i],F_DUPFD,3);if(fd<0){close(pipefd[0]);close(pipefd[1]);return State::StorageError;}close(pipefd[i]);pipefd[i]=fd;}
  fcntl(pipefd[i],F_SETFD,FD_CLOEXEC);
 }
 std::string values[]={executable,create?"1":"0",std::to_string(now),std::to_string(minimum),std::to_string(first),std::to_string(last)};
 char* argv[]={values[0].data(),values[1].data(),values[2].data(),values[3].data(),values[4].data(),values[5].data(),nullptr};
 posix_spawn_file_actions_t actions;
 if(posix_spawn_file_actions_init(&actions)!=0){close(pipefd[0]);close(pipefd[1]);return State::StorageError;}
 posix_spawn_file_actions_adddup2(&actions,pipefd[1],STDOUT_FILENO);
 posix_spawn_file_actions_addclose(&actions,pipefd[0]);
 posix_spawn_file_actions_addclose(&actions,pipefd[1]);
#ifdef __APPLE__
 char** environment=*_NSGetEnviron();
#else
 char** environment=environ;
#endif
 pid_t pid=0;int result=posix_spawn(&pid,executable.c_str(),&actions,nullptr,argv,environment);
 posix_spawn_file_actions_destroy(&actions);close(pipefd[1]);
 if(result!=0){close(pipefd[0]);return State::StorageError;}
 const auto deadline=std::chrono::steady_clock::now()+timeout;
 int status=0;bool finished=false;
 while(std::chrono::steady_clock::now()<deadline){
  auto waited=waitpid(pid,&status,WNOHANG);
  if(waited==pid){finished=true;break;}
  if(waited<0&&errno!=EINTR)break;
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
 }
 if(!finished){kill(pid,SIGKILL);while(waitpid(pid,&status,0)<0&&errno==EINTR){}close(pipefd[0]);return State::StorageError;}
 fcntl(pipefd[0],F_SETFL,O_NONBLOCK);
 char buffer[256]{};auto count=read(pipefd[0],buffer,sizeof(buffer)-1);close(pipefd[0]);
 if(!WIFEXITED(status)||WEXITSTATUS(status)!=0||count<=0)return State::StorageError;
 std::istringstream input(std::string(buffer,size_t(count)));int value=-1;int64_t a=0,b=0;std::string extra;
 if(!(input>>value>>a>>b)||(input>>extra))return State::StorageError;
 if(value==int(State::Ready)&&!create&&a==0&&b==0)return State::Ready;
 if(value!=int(State::Trial)&&value!=int(State::Expired)&&value!=int(State::ClockError))return State::StorageError;
 if(a<=0||b<a||(first&&first!=a))return State::StorageError;
 first=a;last=b;return State(value);
}
}
