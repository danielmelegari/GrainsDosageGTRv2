"""Extract unchanged source pixels into individually cached UI sprites."""
from pathlib import Path
from PIL import Image
import json
root=Path(__file__).resolve().parents[1]
im=Image.open(root/'assets/GrainsDosage-skin.png')
regions={'knob':(385,260,76,76),'slider':(453,636,24,32),'step':(734,456,54,48),'header-left':(1850,12,136,104),'header-right':(1726,15,95,104)}
out=root/'assets'/'sprites';out.mkdir(exist_ok=True)
for name,(x,y,w,h) in regions.items():im.crop((x,y,x+w,y+h)).save(out/(name+'.png'))
(out/'regions.json').write_text(json.dumps(regions,indent=2)+'\n')
