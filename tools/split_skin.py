"""Split the master artwork sheet into one PNG per GUI element.

Outputs:
  assets/sprites/<name>.png          factory sprites (baked into the bundle / RC)
  assets/GrainsDosage-skin/*.png     drop-in skin folder: replace any file here
                                     (background, knob, slider, step,
                                     header-left, header-right, xy-nebula) and
                                     the plugin reskins at runtime — no rebuild.
  assets/GrainsDosage-skin/@2x/      nearest-neighbour 2x variants for Retina.
"""
from pathlib import Path
from PIL import Image
import json
root=Path(__file__).resolve().parents[1]
im=Image.open(root/'assets/GrainsDosage-skin.png').convert('RGBA')
regions={'knob':(385,260,76,76),'slider':(453,636,24,32),'step':(734,456,54,48),
         'header-left':(1850,12,136,104),'header-right':(1726,15,95,104),
         'xy-nebula':(880,566,168,166)}   # XY Morph pad
out=root/'assets'/'sprites';out.mkdir(exist_ok=True)
crops={}
for name,(x,y,w,h) in regions.items():
    c=im.crop((x,y,x+w,y+h));crops[name]=c;c.save(out/(name+'.png'))
(out/'regions.json').write_text(json.dumps({k:list(v) for k,v in regions.items()},indent=2)+'\n')

# Drop-in skin folder: every GUI element as its own editable PNG.
skin=root/'assets'/'GrainsDosage-skin';skin.mkdir(exist_ok=True)
im.save(skin/'background.png')
for name,c in crops.items():c.save(skin/(name+'.png'))
# @2x variants (NEAREST keeps pixel-art edges crisp).
skin2x=skin/'@2x';skin2x.mkdir(exist_ok=True)
for name,src in [('background',im)]+list(crops.items()):
    src.resize((src.width*2,src.height*2),Image.NEAREST).save(skin2x/(name+'.png'))
print('wrote',len(crops)+1,'sprites +',len(crops)+1,'@2x variants')
