// Contract of the upright uploaded knob face in the editors' y-down canvas.
#include <cmath>
#include <cassert>
#include <iostream>
int main(){
 const double pi=3.14159265358979323846;
 auto direction=[&](double v){double theta=(270*v-135)*pi/180;return std::pair<double,double>{std::sin(theta),-std::cos(theta)};};
 auto lo=direction(0),mid=direction(.5),hi=direction(1);
 assert(lo.first<0&&lo.second>0);assert(std::abs(mid.first)<1e-9&&mid.second<0);assert(hi.first>0&&hi.second>0);
 // Quarter positions must face left and right, not sweep backwards.
 assert(direction(.25).first<0);assert(direction(.75).first>0);
 std::cout<<"PASS: upright PNG sweeps clockwise from bottom-left through top to bottom-right\n";
}
