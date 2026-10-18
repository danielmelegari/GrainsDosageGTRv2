# Module backgrounds

Version 0.13.13 uses the artwork supplied in Archive.zip:

- granulizer.png: original granular.png, turquoise
- beatrepeater.png: original, red
- gater.png: original, amber
- reslice.png: AI-assisted vertical extension of the supplied purple reslice.png, for the taller 974:770 panel

Both native editors cache decoded images on opening. They draw a translucent
black overlay (30%, or 45% for Reslice) for control-label contrast; the PNGs
remain full brightness. Controls and hit areas are separate from the artwork.
Windows embeds these PNGs as resources 211–215. macOS bundles them in Resources;
optional user PNG overrides and legacy-JPG handling are documented in
../../docs/licensing.md. The SVG preview uses the same backgrounds and overlay.

Input Options uses input-options.png, extracted unchanged from the supplied
input.png.zip. It is drawn without a dark overlay, with Transpose and Stretch
aligned to its two circular mounts. It also supports the macOS PNG override
folder. Native artwork smoke checks require this fifth image to load.

## 0.13.15 reference style

The current shared renderer uses charcoal vector panels, orange controls and
cyan readouts/steps across Modules, Modulation, Morph and Input Options, matching
the subsequently supplied reference. The previously supplied PNGs remain in the
repository as original artwork; they no longer define the active panel faces.
Native layout, control IDs and hit areas are preserved. Panel and control painting
shares the same implementation on Windows, macOS and the SVG review tool.
