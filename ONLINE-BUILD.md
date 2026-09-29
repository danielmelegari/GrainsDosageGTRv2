# GrainsDosage 0.12.0

## Installazione e compilazione

Estrai GrainsDosage-source.zip. Su GitHub sostituisci src/, CMakeLists.txt,
tutti i test_*.cpp, assets/ e Presets/. Non caricare solo lo ZIP.
Avvia il workflow desiderato tra macOS Mojave Intel, Apple Silicon e Windows 10
x64. Scarica l'artifact dopo la spunta verde; per queste funzioni i workflow
inclusi nell'archivio sono aggiornati.
Questo pacchetto contiene sorgenti: non è ancora un VST3 compilato.

Mac: copia il bundle GrainsDosage.vst3 in ~/Library/Audio/Plug-Ins/VST3/.
Windows: copia l'intera cartella bundle in C:\Program Files\Common Files\VST3\.
Chiudi e riapri la DAW; mantieni una sola versione dello stesso plugin.

## Menu preset e cartella

Apri PRESETS nella testata: vengono installati 30 preset iniziali mancanti.
Quelli esistenti non vengono sovrascritti. Il menu elenca anche i tuoi file.
SAVE PRESET propone questa cartella; LOAD PRESET apre anche file esterni.
OPEN PRESET FOLDER apre direttamente la cartella:

- Mac: ~/Library/Audio/Presets/GrainsDosage/
- Windows: %APPDATA%\GrainsDosage\Presets\

I 30 file .gdspreset si trovano anche in Presets/ nello ZIP. Nove sono i preset
utente importati dai file forniti. Sono punti di partenza per suoni diversi,
non campioni audio. I preset precedenti restano leggibili.

## Reslice, Gater e nuovi effetti

Reslice registra il segnale dopo i tre moduli riordinabili. Il suo sequencer a
16 step sceglie in quale punto della sequenza riprodurre ciascuna slice e usa
loop da 1/1, 1/2, 1/3 o 1/4. Il modulo è bypassabile. Gater Latch mantiene il
gate fra step vuoti e si chiude su uno step Release.

Il Gater usa step Wet/Off (non c'è più Dry Step). Sustain regola la coda di
ciascuno step fino a 500 ms; Minimum Length imposta la durata minima anche con
Length Random attivo. Tie unisce gli step Wet adiacenti in un'unica nota,
compreso il passaggio fra fine e inizio del loop. Latch continua invece a
tenere aperto il gate sugli step Off fino a uno step Release. I vecchi valori
Dry dei preset vengono interpretati come Wet e gli ID dei parametri esistenti
restano invariati.

Il banco aggiunge modelli di filtro originali (ladder, transistor, comb,
formant, phaser e combinazioni di bande) e quattro riverberi: Plate, Cosmic
Space, Dark Space e Bloom Space. Sono interpretazioni DSP originali, non
emulazioni esatte o copie di modelli proprietari.

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
