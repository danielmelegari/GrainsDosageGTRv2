# GrainsDosage 0.10.1

## Novità 0.10.1 — Input De-click

INPUT DE-CLICK e SENSITIVITY sono sotto il menu preset. Il processore agisce
prima di qualsiasi buffer o modulo, sul segnale destinato sia al wet sia al dry
(master e step Dry del Gater). ON per nuove istanze e nuovi preset di fabbrica,
sensibilità iniziale 50%. I vecchi stati/preset lo caricano OFF per conservare
il comportamento precedente. Le impostazioni sono salvate e automatizzabili.

Rileva impulsi brevi con salto e ritorno alla traiettoria locale entro 16 campioni
e interpola fra i campioni circostanti. Una soglia adattiva segue il contenuto.
Non è una rimozione universale: click lunghi, clipping/distorsione e discontinuità
senza ritorno rapido possono restare. Sensibilità alta può alterare microtransienti
voluti: confronta ON/OFF, soprattutto con percussioni, glitch e onde quadre.

Lookahead fisso 32 campioni (0,67 ms a 48 kHz), dichiarato al host VST3.
La latenza rimane identica con De-click OFF e con Bypass globale; Bypass globale
restituisce la sorgente originale ritardata, senza riparazione. Il tasto De-click
usa una transizione di 10 ms. Nessuna allocazione nel trattamento per campione.

Usa tutti i sorgenti di questo pacchetto per la repo. I workflow già inclusi
compilano questa versione: non serve modificarli per il De-click.


## Novità 0.10.0 — Gater Dry/Wet

Nuova fascia sopra Filter e Reverb con 16 step, sincronizzata al tempo host.
Attiva ON: il Gater è spento nei vecchi progetti e nei preset di fabbrica.
Rate: 1/4, 1/8, 1/16, 1/32.

- Click su uno step: alterna Wet (verde) e Off (grigio).
- Premi DRY STEP e clicca: alterna Dry (ambra) e Off.
- Shift-click seleziona senza cambiare lo stato.
- STEP LENGTH / RND MAX regola la durata dello step selezionato dal 5% al 100%.
- LENGTH RND genera una durata dal 5% al massimo impostato per ogni step.
- STEP RND genera Wet/Off con probabilità CHANCE. Gli step Dry dipinti restano protetti.
- Le randomizzazioni cambiano a ogni giro dei 16 step e quando il trasporto torna indietro nel loop.
- La barra in ogni cella indica la durata; il cursore indica lo step corrente.

Wet attraversa Filter e Reverb; Dry passa direttamente al master. Gli step Off
fermano nuovo audio ma lasciano proseguire le code del riverbero. Kill Dry del
riverbero silenzia anche il passaggio Dry del Gater. Il limiter rimane finale.
Per ascoltare pause complete usa Master Dry/Wet al 100%: abbassandolo aggiungi
la sorgente dry continua, come prima. Piccole dissolvenze attenuano i click.

La GUI completa parte da 792×660 (60%), con zoom fino al 100%, senza scroll.
Stati e preset salvano pattern, durate e opzioni; il modo pittura Dry e la
selezione visuale dello step sono strumenti locali dell'editor.

Aggiornamento: sostituisci i sorgenti, CMakeLists.txt, test e workflow con quelli
nel pacchetto e ricompila per la piattaforma desiderata. Il workflow Windows
include la correzione CRT e produce anche l'installer. Non basta cambiare il .yml.
Il pacchetto contiene sorgenti, non binari VST3 già compilati.


## Novità 0.9.4 — Limiter finale

Il menu accanto a Limiter seleziona 0, -6 o -10 dBFS. Il limite viene
applicato dopo tutti gli effetti, Normalize e Dry/Wet, su entrambi i canali.
La riduzione è stereo-linked con attacco immediato e release di 80 ms.
Un controllo finale impedisce anche arrotondamenti float sopra il limite.
Funziona anche quando si abbassa la soglia durante l'audio o si attiva il limiter.
Limiter Off lascia il segnale non limitato. Il Bypass generale esclude tutto.
È sample-peak, non true-peak: non garantisce il picco intersample/analogico.
Nuovi preset e preset precedenti senza questa scelta usano 0 dBFS (il vecchio
limite fisso era -1 dBFS). Scegliere -6 o -10 dBFS per maggiore margine.
Testate tutte le soglie anche attraverso il processor VST con buffer 32 e 64 bit.

## Novità 0.9.3 — Random Impulse e Mod Glide

Il menu in alto nel riverbero seleziona Step Sequencer o Random Impulse.
Random Impulse ignora tutti i pallini dello step sequencer. Random Rate offre
1/4, 1/8 e 1/16; a ogni intervallo il send viene aperto o chiuso casualmente
con probabilità 50%. La decisione resta fissa durante l'intervallo ed è
ripetibile alla stessa posizione musicale. Il LED mostra il send aperto.
La coda continua durante gli intervalli chiusi e nei cambi di modalità:
Length ne regola il decadimento, Kill Dry permette di ascoltare solo il return.

Mod Wave RND applica un glide automatico a ogni cambio effettivo di forma.
Il controllo Glide di ciascun Mod regola circa 12–250 ms di transizione.
Il cambio parte dall'ultimo valore emesso, senza un salto immediato.
Glide continua a regolare anche S&G quando Mod Wave RND è Off.

Preset e progetti precedenti mantengono Step Sequencer. Workflow invariati.
Sostituire src/, assets/, CMakeLists.txt, test_*.cpp e Presets/ e ricompilare.

## Novità 0.9.2 — Mod e skin viola

- Ultima skin caricata: pannelli antracite, controlli viola a fungo, LED menta.
- Mod 1–4 sostituiscono il nome LFO, mantenendo gli ID delle automazioni.
- Ogni Mod ha Mod Wave RND: Off, 1/1, 1/2, 1/4 o 1/8.
  Se attivo, sceglie fra 128 onde al tempo del progetto (4, 2, 1 o 0.5 beat).
  La scelta è ripetibile alla stessa posizione del progetto e distinta per Mod.
  Il display e il selettore mostrano l'onda attiva. Off ripristina l'onda manuale.
  Transizione regolata da Glide fra le forme randomizzate; S&H/S&G manuali restano invariati.
- Speed e Transpose sono nel frame Master Options del Granulizer.
  Agiscono ancora sul percorso wet dopo i tre moduli, prima del filtro/reverbero.
- On/Off di ogni modulo in alto accanto a Random; il titolo resta trascinabile.
- Transpose: smussamento di 25 ms della variazione di pitch, senza azzerare la fase.
  La riduzione del crack deve essere confermata all'ascolto nella DAW.
- Stato e preset delle versioni precedenti vengono caricati con Mod Wave RND Off.

Caricare tutti i sorgenti, assets e CMakeLists.txt, incluso il nuovo src/skin.rc.in
per Windows. I workflow .yml non cambiano. La GUI nativa è verificata dai test
GitHub Actions; non è stata eseguita localmente su macOS/Windows.

## Waveform centrata

Il cursore del grano attivo resta al centro; la waveform scorre sotto il cursore.
La zona del grano resta evidenziata e il contesto grigio. Le zone non ancora
registrate o uscite dal buffer restano vuote. Vale per Mac e Windows.
Nessuna modifica al motore audio o ai workflow .yml.
Test automatico: cursore al 50% e waveform aggiornata durante la lettura.
La resa grafica nativa deve essere verificata nella DAW dopo la compilazione.

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
