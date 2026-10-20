#include "src/editor_refresh.h"
#include "src/host_edit_queue.h"
#include <cassert>
#include <iostream>
int main(){
 aztec::HostEditQueue edits;
 int notifications=0;double finalValue=-1.;
 auto send=[&](uint32_t id,double v){assert(id==7);++notifications;finalValue=v;};
 for(int i=0;i<1000;++i){edits.queue(7,i/1000.);edits.flush(i*.001,send);}
 assert(notifications>=58&&notifications<=60);
 edits.flush(.999,send,true);assert(finalValue==.999); // flush before endEdit
 int count=notifications;edits.flush(2,send,true);assert(notifications==count);
 edits.clear();edits.queue(10,.2);edits.queue(11,.8);int xy=0;
 edits.flush(0,[&](uint32_t id,double v){assert((id==10&&v==.2)||(id==11&&v==.8));++xy;},true);assert(xy==2);
 // A new gesture starts immediately, without the previous gesture's deadline.
 edits.clear();edits.queue(7,.1);edits.flush(0,send);assert(finalValue==.1);
 aztec::EditorRefresh refresh;
 assert(!refresh.interactionDue(0));
 // Simulate a 1000 Hz mouse. Bound repaint requests without dropping the
 // last request when motion stops between frame ticks.
 int frames=0;
 for(int i=0;i<1000;++i){refresh.request();frames+=refresh.interactionDue(i*.001);}
 assert(frames>=58&&frames<=60);
 assert(refresh.interactionDue(1.1));assert(!refresh.interactionDue(1.2));
 // Polling may resume after a long drag without a backlog of catch-up frames.
 assert(refresh.backgroundDue(0));assert(!refresh.backgroundDue(.001));
 assert(refresh.backgroundDue(30));assert(!refresh.backgroundDue(30.001));
 refresh.request();assert(refresh.interactionDue(40));assert(!refresh.interactionDue(40));
 std::cout<<"PASS: 1000 Hz gestures coalesce to <=60 paints/s; final update retained; no idle catch-up\n";
}
