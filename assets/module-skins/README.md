# Module backgrounds

Version 0.13.13 uses the artwork supplied in Archive.zip:

- granulizer.png: original granular.png, turquoise
- beatrepeater.png: original, red
- gater.png: original, amber
- reslice.png: AI-assisted vertical extension of the supplied purple reslice.png, for the taller 974:770 panel

Both native editors cache decoded images on opening. They draw a translucent
black overlay (30%, or 45% for Reslice) for control-label contrast; the PNGs
remain full brightness. Controls and hit areas are separate from the artwork.
Windows embeds these PNGs as resources 211–214. macOS bundles them in Resources;
optional user PNG overrides and legacy-JPG handling are documented in
../../docs/licensing.md. The SVG preview uses the same backgrounds and overlay.
