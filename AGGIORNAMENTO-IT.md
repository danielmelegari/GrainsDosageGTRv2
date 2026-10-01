# GrainsDosage 0.13.0 — aggiornamento

## Filter Sequencer
Il filtro ha un modulatore dedicato: attiva SEQUENCER e scegli Arp o Sample & Glide. Arp offre 64 pattern originali di 32 punti, non i preset proprietari Access Virus TI2. Rate segue il tempo del progetto; Depth regola l'escursione e Glide ammorbidisce le transizioni. Non occupa nessuno dei quattro Mod.

Nei modelli Comb positivo/negativo compaiono Root, Octave e Notes. Minor chord vincola le frequenze alle note della triade minore (tonica, terza minore, quinta); Natural minor usa le sette note della scala minore naturale. Cutoff e modulazione scelgono la nota entro questo insieme. Il filtro risonante non è un generatore polifonico: seleziona una nota alla volta e non garantisce di rimuovere tutte le altre frequenze della sorgente.

## Reslice
- Window: 4/1, 2/1, 1/1, 1/2.
- Step RND: cambia automaticamente le slice assegnate ai 16 step, con intervalli 1/2, 1/1 o 2/1 sincronizzati al transport.
- RANDOM ONCE: cambia le assegnazioni una sola volta e disattiva Step RND. Il risultato resta modificabile e salvabile nei preset.
- Entrambe le modalità mantengono gli step attivi/disattivi già disegnati. La griglia visualizza le slice correnti anche durante la randomizzazione automatica.

## Modulation
101 destinazioni, con i sei slot per ciascun Mod conservati. Elenco completo in MODULATION-DESTINATIONS.txt. Sono inclusi controlli musicali di effetti, pitch degli step Repeater, length/sustain Gater e sorgenti Reslice. Non sono destinazioni i bypass, la soglia del limiter, i comandi preset e i controlli che configurano i Mod stessi.

## Compatibilità
Le nuove funzioni automatiche partono disattivate. I vecchi preset e gli stati del plugin vengono migrati mantenendo gli ID dei parametri esistenti e rimappando gli indici delle destinazioni Mod.
Le vecchie Window 1/1 e 1/2 mantengono la durata. Le precedenti 1/3 e 1/4, eliminate dal menu richiesto, vengono convertite a 1/2: quei preset avranno una finestra diversa. Controllare eventuali automazioni host delle liste Window e destinazioni Mod, i cui valori normalizzati sono cambiati.

## Compilazione e installazione
Questo ZIP contiene i sorgenti, skin, 30 preset e workflow, non un VST3 già compilato. Caricare tutto il contenuto nella radice della repo, inclusa .github/workflows. L'unico workflow è build-vst3.yml: genera solo i bundle .vst3 (macOS Intel, Apple Silicon e Windows x64) come ZIP, senza installer. Scarica lo zip dal run riuscito nella sezione Artifacts e copia la cartella GrainsDosage.vst3 nel percorso VST3 della DAW; build-diagnostics contiene soltanto log.
La GUI conserva le skin e aggiunge una fascia per il sequencer filtro; dimensione iniziale 792 x 816, ridimensionabile.

## Verifica
Build Linux Release riuscita; 13/13 test CTest passati, comprese migrazione degli stati, 64 pattern distinti, vincoli minori e confini temporali del random Reslice. Il validator VST3 è eseguito separatamente (vedere validation-linux.txt). SDK locale 3.8.0_build_66; le CI conservano il proprio commit SDK fissato.
Non è stata eseguita qui una build nativa macOS/Windows né una prova in Cubase. Il superamento dei test Linux non garantisce il caricamento nelle DAW sulle altre piattaforme.
