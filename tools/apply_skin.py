#!/usr/bin/env python3
"""One-shot palette swap: route both native renderers through skin_spec.h.
Mechanical colour remap only (purple/taupe -> charcoal/silver/amber; greens kept)."""
from pathlib import Path
ROOT = Path(__file__).resolve().parents[1] / "src"

MAC = {
 "rgb(.48,.29,.59)":"C(aztec::skin::filigree)",
 "rgb(.26,.13,.37)":"C(aztec::skin::panelRaised)",
 "rgb(.67,.46,.96)":"C(aztec::skin::violet)",
 "rgb(.24,.13,.34)":"C(aztec::skin::greenDeep)",
 "rgb(.14,.12,.19)":"C(aztec::skin::borderDark)",
 "rgb(1.,.3,.18)":"C(aztec::skin::hot)",
 "rgb(.95,.35,.32)":"C(aztec::skin::release)",
 "rgb(.91,.94,.98)":"C(aztec::skin::cream)",
 "rgb(.72,.68,.81)":"C(aztec::skin::muted)",
 "rgb(.68,.91,.95)":"C(aztec::skin::readout)",
 "rgb(.65,.48,.78)":"C(aztec::skin::filigree)",
 "rgb(.64,.35,.79,.48)":"C(aztec::skin::violet,.48)",
 "rgb(.61,1.,.35)":"C(aztec::skin::green)",
 "rgb(.61,.46,.70)":"C(aztec::skin::filigree)",
 "rgb(.61,.38,.73)":"C(aztec::skin::title)",
 "rgb(.43,.66,.52)":"C(aztec::skin::filigreeDim)",
 "rgb(.43,.30,.49)":"C(aztec::skin::filigree)",
 "rgb(.40,.31,.47)":"C(aztec::skin::filigreeDim)",
 "rgb(.39,.27,.45)":"C(aztec::skin::filigree)",
 "rgb(.35,.23,.44)":"C(aztec::skin::filigreeDim)",
 "rgb(.35,.22,.45)":"C(aztec::skin::greenDim)",
 "rgb(.34,.28,.41)":"C(aztec::skin::filigreeDim)",
 "rgb(.32,.25,.40)":"C(aztec::skin::filigreeDim)",
 "rgb(.31,.20,.38)":"C(aztec::skin::filigreeDim)",
 "rgb(.31,.17,.43)":"C(aztec::skin::greenDeep)",
 "rgb(.29,.19,.38)":"C(aztec::skin::filigreeDim)",
 "rgb(.28,.27,.23)":"C(aztec::skin::filigreeDim)",
 "rgb(.28,.24,.35)":"C(aztec::skin::hairline)",
 "rgb(.28,.18,.37)":"C(aztec::skin::filigreeDim)",
 "rgb(.27,.21,.34)":"C(aztec::skin::filigreeDim)",
 "rgb(.25,.18,.32)":"C(aztec::skin::filigreeDim)",
 "rgb(.23,.18,.30)":"C(aztec::skin::borderDark)",
 "rgb(.23,.12,.33)":"C(aztec::skin::greenDeep)",
 "rgb(.22,.20,.30)":"C(aztec::skin::hairline)",
 "rgb(.17,.12,.22)":"C(aztec::skin::borderDark)",
 "rgb(.16,.14,.22)":"C(aztec::skin::greenDim)",
 "rgb(.12,.09,.18)":"C(aztec::skin::panel)",
 "rgb(.12,.075,.18)":"C(aztec::skin::panelRaised)",
 "rgb(.10,.16,.13)":"C(aztec::skin::meterOff)",
 "rgb(.10,.075,.14)":"C(aztec::skin::panelRaised)",
 "rgb(.075,.055,.11)":"C(aztec::skin::panelRaised)",
 "rgb(.075,.045,.115)":"C(aztec::skin::panelInset)",
 "rgb(.055,.042,.080)":"C(aztec::skin::panel)",
 "rgb(.025,.024,.044)":"C(aztec::skin::bgDeep)",
 "rgb(.025,.019,.036)":"C(aztec::skin::borderDark)",
 "rgb(.025,.017,.04)":"C(aztec::skin::well)",
 "rgb(.018,.013,.027)":"C(aztec::skin::well)",
}
# kept as-is (green/grey, on brand): .24,.48,.12 / .04,.28,.16 / .075,.20,.085 /
# .12,.19,.16 / .23,.27,.24 / .19,.26,.22 / .28,.33,.28 / .15,.94,.52,.25 /
# .36,.38,.39 / .31,.27,.36 / .20,.16,.26 / .24,.21,.28 / .045,.04,.065 / .01,.01,.02

WIN = {
 "RGB(170,117,245)":"C(aztec::skin::violet)",
 "RGB(66,33,94)":"C(aztec::skin::panelRaised)",
 "RGB(79,51,97)":"C(aztec::skin::filigreeDim)",
 "RGB(205,190,222)":"C(aztec::skin::muted)",
 "RGB(174,158,192)":"C(aztec::skin::muted)",
 "RGB(122,74,150)":"C(aztec::skin::filigree)",
 "RGB(89,59,112)":"C(aztec::skin::filigreeDim)",
 "RGB(61,33,87)":"C(aztec::skin::greenDeep)",
 "RGB(36,31,48)":"C(aztec::skin::borderDark)",
 "RGB(3,3,5)":"C(aztec::skin::well)",
 "RGB(14,11,20)":"C(aztec::skin::panel)",
 "RGB(99,69,115)":"C(aztec::skin::filigree)",
 "RGB(92,52,117)":"C(aztec::skin::violet)",
 "RGB(89,56,115)":"C(aztec::skin::greenDim)",
 "RGB(87,71,105)":"C(aztec::skin::filigreeDim)",
 "RGB(82,64,102)":"C(aztec::skin::filigreeDim)",
 "RGB(75,60,90)":"C(aztec::skin::filigreeDim)",
 "RGB(74,48,97)":"C(aztec::skin::filigreeDim)",
 "RGB(71,61,89)":"C(aztec::skin::hairline)",
 "RGB(69,54,87)":"C(aztec::skin::filigreeDim)",
 "RGB(62,54,73)":"C(aztec::skin::filigreeDim)",
 "RGB(6,6,11)":"C(aztec::skin::bgDeep)",
 "RGB(6,5,9)":"C(aztec::skin::borderDark)",
 "RGB(6,4,10)":"C(aztec::skin::well)",
 "RGB(59,46,77)":"C(aztec::skin::borderDark)",
 "RGB(59,31,84)":"C(aztec::skin::greenDeep)",
 "RGB(56,51,77)":"C(aztec::skin::hairline)",
 "RGB(5,3,7)":"C(aztec::skin::well)",
 "RGB(43,31,56)":"C(aztec::skin::borderDark)",
 "RGB(41,36,56)":"C(aztec::skin::greenDim)",
 "RGB(31,23,46)":"C(aztec::skin::panel)",
 "RGB(26,42,33)":"C(aztec::skin::meterOff)",
 "RGB(26,19,36)":"C(aztec::skin::panelRaised)",
 "RGB(255,80,40)":"C(aztec::skin::hot)",
 "RGB(242,89,82)":"C(aztec::skin::release)",
 "RGB(232,240,250)":"C(aztec::skin::cream)",
 "RGB(199,186,214)":"C(aztec::skin::muted)",
 "RGB(19,14,28)":"C(aztec::skin::panelRaised)",
 "RGB(19,11,29)":"C(aztec::skin::panelInset)",
 "RGB(173,232,242)":"C(aztec::skin::readout)",
 "RGB(156,97,186)":"C(aztec::skin::title)",
 "RGB(156,255,89)":"C(aztec::skin::green)",
 "RGB(156,117,179)":"C(aztec::skin::filigree)",
 "RGB(150,120,170)":"C(aztec::skin::greenDim)",
 "RGB(110,77,125)":"C(aztec::skin::filigree)",
 "RGB(110,168,133)":"C(aztec::skin::filigreeDim)",
 "RGB(102,79,120)":"C(aztec::skin::filigree)",
}
# kept: 61,122,31 / 44,39,53 / 40,90,60 / 92,97,99 / 80,100,80 / 75,82,67 /
# 58,69,61 / 41,26,59 / 39,34,49 / 30,25,39 / 25,43,34 / 19,51,22

def apply(fn, mapping):
    p = ROOT / fn
    t = p.read_text()
    n = 0
    for k, v in sorted(mapping.items(), key=lambda kv: -len(kv[0])):  # longest first
        cnt = t.count(k)
        if cnt:
            t = t.replace(k, v)
            n += cnt
    p.write_text(t)
    print(f"{fn}: {n} literals remapped")

apply("editor_mac.mm", MAC)
apply("editor_win.cpp", WIN)
