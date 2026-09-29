# GrainsDosage 0.9.4

## Installazione e compilazione

Estrai GrainsDosage-source.zip. Su GitHub sostituisci src/, CMakeLists.txt,
tutti i test_*.cpp, assets/ e Presets/. Non caricare solo lo ZIP.
I tre workflow .yml restano invariati. Avvia Actions per Mojave Intel,
Silicon M1 e/o Windows x64. Scarica l'artifact dopo la spunta verde.
Questo pacchetto contiene sorgenti: non è ancora un VST3 compilato.

Mac: copia il bundle GrainsDosage.vst3 in ~/Library/Audio/Plug-Ins/VST3/.
Windows: copia l'intera cartella bundle in C:\Program Files\Common Files\VST3\.
Chiudi e riapri la DAW; mantieni una sola versione dello stesso plugin.

## Menu preset e cartella

Apri PRESETS nella testata: vengono installati dieci preset iniziali mancanti.
Quelli esistenti non vengono sovrascritti. Il menu elenca anche i tuoi file.
SAVE PRESET propone questa cartella; LOAD PRESET apre anche file esterni.
OPEN PRESET FOLDER apre direttamente la cartella:

- Mac: ~/Library/Audio/Presets/GrainsDosage/
- Windows: %APPDATA%\GrainsDosage\Presets\

I dieci .gdspreset si trovano anche in Presets/ nello ZIP. Sono punti di partenza
per suoni diversi, non campioni audio. I preset 0.8.1 restano leggibili.

## Display del grano e pan

Pan compatto: scegli Manual / Alternate / Random. Lo slider L–C–R è visibile
solo in Manual. Lo schermo sottostante legge il vero buffer del Granulizer:
contesto grigio, porzione sorgente verde e testina di lettura chiara.
Con più grani sovrapposti segue quello attivo partito più recentemente, non
l'insieme di tutti i grani. È una panoramica a 128 colonne, aggiornata circa
30 volte al secondo, con almeno 2,5 secondi di contesto.

## LFO e filtro

Ogni LFO ha sei menu DESTINATION, ognuno con AMT bipolare. Puoi assegnare
Cutoff, Resonance e Drive del filtro, oltre agli otto parametri granulari/finali.
Assegnazioni duplicate si sommano. SPEED MULTIPLIER offre 0,25x / 0,5x / 1x / 2x:
a 0,25x il ciclo dura quattro volte, mantenendo tutti i 64 punti S&H/S&G.
La barra verticale segue la fase reale. Anche l'anteprima casuale segue il ciclo
corrente. Waveform e cursori richiedono audio in elaborazione e l'inoltro dei
parametri di monitor da parte della DAW.

Catena: moduli riordinabili → Speed/Transpose → Filtro/Drive → Riverbero
→ Normalize → Dry/Wet → Limiter. Il filtro precede SEMPRE il riverbero e non
modifica la coda già generata. Il dry del master non attraversa questi effetti wet.

## Riverbero e Kill Dry

Gli step accesi aprono il send; quelli spenti lasciano proseguire la coda.
LENGTH regola il decadimento nominale fra 0,2 e 20 secondi. I cinque tipi hanno
assorbimenti diversi, quindi il tempo percepito varia con il contenuto sonoro.

KILL DRY, con REVERB ON, fa ascoltare soltanto il ritorno del riverbero attivato
dagli step, comprese le code. Esclude anche il dry del master. AMOUNT continua
a dosare il ritorno. Senza send e senza coda, l'uscita è silenziosa.
REVERB OFF ripristina il dry; BYPASS globale esclude tutto.

I test locali non sostituiscono la prova nelle build native e nella tua DAW.
