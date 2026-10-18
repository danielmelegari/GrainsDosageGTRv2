#include "src/trial_license.h"
#include <cassert>
#include <limits>
#include <iostream>

int main() {
  using namespace grains::trial;
  const Tick start = 130000000000000000ULL;
  State state{start, start};
  assert(advance(state, start + duration - 1));
  assert(!advance(state, start + duration));
  assert(!advance(state, start)); // Cannot revive expired persisted state.
  state = {start, start + 5 * second};
  assert(advance(state, start + 4 * second));
  assert(state.last == start + 5 * second);
  state = {start, start + 300 * second};
  assert(!advance(state, start));
  state = {start, start - 1};
  assert(!advance(state, start));
  State merged{};
  assert(merge({start, start+second}, {start, start+3*second}, merged));
  assert(merged.last == start+3*second);
  assert(!merge({start, start}, {start+1, start+1}, merged));
  assert(!advance(state, std::numeric_limits<Tick>::max()));
  auto text = std::string("GRAINS-TRIAL-1\n") + std::string(64, 'a') + "\n" +
    std::to_string(start) + "\n" + std::to_string(start+duration) + "\n" + std::string(512, 'b') + "\n";
  License license;
  assert(parseLicense(text, license));
  assert(license.start == start && license.end == start+duration);
  assert(license.signature.size() == 256);
  assert(!parseLicense(text + "junk", license));
  assert(!parseLicense(text.substr(0,text.size()-1), license));
  auto changed = text; changed[20] = 'z'; assert(!parseLicense(changed, license));
  changed = text; changed.back() = '\r'; assert(!parseLicense(changed, license));
  assert(!parseLicense(std::string(5000,'x'), license));
  auto permanent=std::string("GRAINS-PERMANENT-1\n")+std::string(64,'a')+"\n"+std::string(32,'c')+"\n3\n"+std::string(512,'b')+"\n";
  assert(parseLicense(permanent,license));assert(license.permanent&&license.licenseId==std::string(32,'c'));
  changed=permanent;changed[changed.find("\n3\n")+1]='4';assert(!parseLicense(changed,license));
  changed=permanent;changed[changed.find(std::string(32,'c'))]='z';assert(!parseLicense(changed,license));
  assert(parseLicense(text,license));assert(!license.permanent&&license.licenseId.empty());
  std::cout << "Trial policy and license parsing passed\n";
}
