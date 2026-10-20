#include "src/editor_refresh.h"
#include <cassert>
#include <iostream>
int main(){
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
