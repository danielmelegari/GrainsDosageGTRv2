# Modulation — GrainsDosage 0.9.3

Ogni LFO ha sei menu DESTINATION con AMT da −100% a +100%. None disattiva
l'assegnazione. Scegli fra Size, Density, Pitch, Lookback, Chaos, Grain Mix,
Speed, Transpose, Filter Cutoff, Filter Resonance e Filter Drive.
Le assegnazioni duplicate si sommano e il risultato resta nel range del parametro.
Il filtro deve essere acceso per sentire le sue modulazioni; Speed richiede Free Stretch.

Le 128 onde e S&H/S&G sono indipendenti dal moltiplicatore 0,25x / 0,5x / 1x / 2x.
Con 64 punti e 0,25x i punti restano 64 ma l'intero ciclo dura quattro volte.
Il moltiplicatore funziona sia in Sync sia in Free. La barra indica la fase
reale; le forme casuali usano il ciclo e il seme di retrigger del motore audio.

Gli ID delle vecchie rotte restano per compatibilità. Caricando vecchi stati,
le prime sei rotte attive di ogni LFO vengono mostrate nei nuovi menu. Eventuali
rotte legacy oltre la sesta restano attive come parametri nascosti: Clean Grains
azzera queste vecchie assegnazioni. Controlla i progetti con automazioni legacy.

I dati dei cursori sono parametri read-only inviati dal motore circa 30 volte
al secondo. La DAW deve inoltrarli al controller; occorre audio in elaborazione.

## Mod Wave RND (0.9.2)
Ogni Mod offre Off, 1/1, 1/2, 1/4 e 1/8. La randomizzazione sceglie
una delle 128 onde sul clock musicale del progetto, indipendentemente da Sync
e dal moltiplicatore Speed. Le destinazioni e gli amount non cambiano.
La forma selezionata manualmente viene ripristinata scegliendo Off.

Da 0.9.3 il controllo Glide regola anche la transizione automatica fra le onde
di Mod Wave RND (circa 12–250 ms).
