# GUI Psychedelic — controlli nativi

Adattamento dello stile del mockup ai renderer Cocoa e Win32 esistenti: pannelli viola/nero, bordi decorativi vettoriali, verde lime per gli stati attivi, knob metallici con puntatore, display numerici incassati ciano, LED con alone, pulsanti SAVE/LOAD più compatti.

Sono controlli funzionanti collegati ai parametri esistenti, non un'immagine sovrapposta. Disposizione e aree cliccabili rimangono quelle del layout verificato: questo aggiornamento non riproduce pixel per pixel l'illustrazione e mantiene la waveform nel deck Granular.

Installazione sorgenti: estrarre e caricare il contenuto della patch nella radice della repo mantenendo src/. Include i precedenti fix e il pretest geometrico. Nessuna modifica richiesta ai workflow YAML già corretti. Avviare una nuova build dopo il commit.

Verifica locale: pretest geometrico passato su 29184 configurazioni. Rendering, click nativi e compatibilità Mac/Windows da confermare nelle CI: non è stato compilato un nuovo VST3 nativo in questo ambiente.
