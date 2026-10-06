# GrainsDosage Module PNGs

Each module has its own PNG file that you can edit independently.

## Module Files

| Module | File | Dimensions | Canvas Position |
|--------|------|------------|-----------|
| Granulizer | `granulizer.png` | 416×404 | (16, 98) |
| PreSlicer | `preslicer.png` | 416×404 | (452, 98) |
| BeatRepeater | `beatrepeater.png` | 416×404 | (888, 98) |
| Modulation | `modulation.png` | 836×236 | (16, 516) |
| Morph | `morph.png` | 440×236 | (864, 516) |
| Reslice | `reslice.png` | 1288×108 | (16, 766) |
| Gater | `gater.png` | 1288×108 | (16, 886) |
| Filter | `filter.png` | 540×136 | (16, 1006) |
| Reverb | `reverb.png` | 736×136 | (568, 1006) |
| FilterSeq | `filterseq.png` | 1288×128 | (16, 1154) |
| MasterOut | `masterout.png` | 1288×56 | (16, 1294) |

## How to Use

1. Replace each PNG with your own design for that module
2. Keep the dimensions exact—the editor reads them from these files
3. Edit, save, rebuild
4. The editor will load each module PNG from this folder

## Workflow

### Option A: Split from the original
Crop each region from `assets/GrainsDosage-skin.png` using the canvas coordinates above.

### Option B: Design from scratch
Create a new PNG at the specified dimension for each module and place it here.

### Option C: Serum-style skins
Create multiple folders (e.g., `skin-dark/`, `skin-light/`) with different module PNGs and let users swap them.
