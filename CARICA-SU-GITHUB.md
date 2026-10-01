# Pacchetto completo GrainsDosage 0.13.0

Estrai lo ZIP e apri la cartella GrainsDosage. Carica il CONTENUTO di questa cartella nella radice della repository: CMakeLists.txt, src, assets, Presets, installer e .github devono essere allo stesso livello. Non caricare solo lo ZIP e non mettere .github dentro un'altra sottocartella nella repo.

I workflow sono già veri file .yml dentro .github/workflows: non devi rinominarli o copiarne il testo.

| Workflow | Risultato |
| --- | --- |
| build-vst3.yml | Unico workflow: genera solo i bundle .vst3 (niente installer) — macOS Intel x64, macOS Apple Silicon arm64 e Windows 10/11 x64, ciascuno come ZIP negli Artifacts del run |

Su Mac, se la cartella .github non appare nel Finder, premi Cmd + Shift + punto. Assicurati che sia presente anche nella repository dopo il caricamento.

Da GitHub apri Actions, scegli il workflow e premi Run workflow. Quando termina, scarica l'artefatto del plugin/installer, non build-diagnostics. Per compilare non serve Xcode sul tuo Mac: la compilazione avviene sui runner GitHub.

Gli installer Mac senza certificati Apple configurati vengono prodotti non firmati/notarizzati. Le build native devono ancora essere verificate su Mac/Windows e in Cubase. Questo archivio contiene i sorgenti da compilare, non binari già pronti.
