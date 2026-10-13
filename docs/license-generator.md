# GrainsDosage License Generator — guida del produttore

Applicazione Cocoa nativa **GrainsDosage License Generator.app**, per **macOS Mojave 10.14 o successivo, Intel x86_64**. Non richiede Python, un terminale o una connessione Internet durante l’uso. Il generatore è uno strumento del produttore; ai clienti si invia esclusivamente il TXT di attivazione.

## Scaricare e aprire

Nel workflow **GrainsDosage License Generator — Mojave Intel**, scaricare l’artifact **GrainsDosage-License-Generator-macOS-Mojave-Intel**. Estrarre lo ZIP dell’artifact e quindi `GrainsDosage-License-Generator-Mojave-Intel.zip`. Spostare l’app in Applicazioni sul Mac del produttore.

La build automatica è firmata ad hoc, senza certificato Developer ID o notarizzazione Apple. Al primo avvio su Mojave, se Gatekeeper la blocca, usare Finder → tasto destro sull’app → **Apri**. La firma delle licenze RSA è distinta dalla firma Apple dell’app.

Il workflow verifica architettura Intel e deployment minimo 10.14, compila con controlli sulle API e testa firme e registro. L’esecuzione su un Mac fisico Mojave resta un controllo distinto dall’esecuzione del runner macOS moderno.

## Build abbinate alla chiave del produttore — 1.1.2 / plugin 0.13.3

Questa build usa la chiave pubblica ricevuta in `GrainsDosage-Public-Key.json`, impronta SHA-256 `f41e04335d43d1c44322072831bd83266500da8a04cf98d0fa3f0f28b75d5dca`.

Se hai già creato quella chiave sul Mac, **non crearne un'altra e non importare un PEM**. Installa il generatore **1.1.2 sullo stesso Mac**, apri **Configurazione iniziale… → Nuovo registro (prima emissione)** e poi genera Trial o Permanent normalmente. La chiave nel Portachiavi viene riconosciuta automaticamente. Se un registro è già configurato, usalo senza ricrearlo. I destinatari devono installare i plugin **0.13.3 o successivi abbinati a questa chiave**.

La versione 1.1.2 corregge il caso in cui macOS restituisce la chiave privata dal Portachiavi ma non permette di copiarne la parte pubblica con `SecKeyCopyPublicKey`. Prima questo caso veniva indicato erroneamente come chiave diversa oppure «Chiave pubblica non disponibile». Ora l’app firma una prova casuale e la verifica con la chiave pubblica abbinata prima di riconoscerla/esportarla; verifica anche ogni licenza con quella stessa chiave pubblica. Non estrae né ricostruisce la chiave privata. Se macOS chiede l’accesso al Portachiavi per la nuova app, autorizzalo. Un accesso negato viene segnalato come errore di firma, senza invitare a creare un’altra chiave. La versione è visibile nel titolo della finestra.

Le vecchie chiavi pubbliche restano nei verificatori per accettare i codici già firmati, inclusi i GDS2 permanenti, senza modificare identità del plugin o sessioni. Il generatore firma soltanto con la nuova chiave abbinata. Le build precedenti non possono riconoscere le nuove firme. Nessuna chiave privata è stata ricevuta o aggiunta al repository.

## Prima volta senza PEM — versione 1.1

Il pacchetto non contiene una chiave privata. Se non hai mai avuto il PEM del produttore:

1. Apri **Configurazione iniziale… → Crea la mia chiave su questo Mac**. L’app crea una chiave RSA-2048 usando macOS e la conserva nel Portachiavi login. Non serve il Terminale. Una chiave già presente non viene sostituita.
2. Salva **GrainsDosage-Public-Key.json**. Questo file contiene esclusivamente la chiave **pubblica** e la sua impronta SHA-256: è il solo file da inviare a chi prepara le build.
3. Nel menu dell’app scegli **Backup chiave privata cifrato…**. Salva il PEM protetto da password su un supporto sicuro e conserva la password separatamente. **Non inviare questo PEM in chat, a GitHub o ai clienti.** È un backup facoltativo del produttore, distinto dai TXT consegnati ai clienti.
4. Invia il JSON pubblico per ottenere plugin e generatore abbinati. **La creazione della chiave non aggiorna le build esistenti.** Questa versione blocca la firma finché la chiave nel Portachiavi non corrisponde alla chiave pubblica compilata nell’app. Non installare la chiave pubblica ricevuta dal cliente e non disattivare questo controllo.
5. Installa il generatore abbinato **sullo stesso Mac**, riapri **Configurazione iniziale…** e scegli **Nuovo registro (prima emissione)**. La chiave creata al punto 1 viene riutilizzata automaticamente: non devi cercare o selezionare un PEM. Installa anche i plugin abbinati prima di distribuire i nuovi codici.

Se annulli il salvataggio del JSON, premi **Esporta chiave pubblica** per riprovare. Non generare un’altra chiave. Il Portachiavi mantiene la stessa chiave dopo la chiusura dell’app. Le licenze generate in seguito possono essere Trial o Permanent, per Windows o macOS: usano tutte la stessa chiave del produttore e sono legate alla Machine ID del destinatario.

La nuova chiave non è un recupero della precedente. Chi prepara l’aggiornamento deve conservare la vecchia chiave pubblica come chiave di verifica delle licenze permanenti già emesse; i plugin precedenti non riconoscono le firme nuove. Se hai già una chiave privata o un registro configurato, la creazione automatica è bloccata: usa il percorso di importazione/ripristino descritto sotto.

## Configurazione con PEM originale — una sola volta

1. Conservare un backup sicuro della **chiave privata RSA originale già usata dal plugin** e del **registro SQLite originale**. Non inviare questi file ai clienti e non caricarli su GitHub, negli artifact o in questa chat.
2. Aprire **Configurazione iniziale… → Importa PEM esistente…** e selezionare il PEM originale. Per un PEM cifrato, macOS può chiedere la password. L’app confronta la chiave pubblica derivata con quella già incorporata nel plugin: se non corrisponde, si ferma. L’importazione non genera una nuova coppia e non sostituisce la chiave esistente.
3. Scegliere **Usa registro esistente…** se sono già state emesse licenze con `license-issuer.py`. Viene usato direttamente quel file SQLite, preservando License ID e dispositivi. Non viene creata una copia indipendente.
4. Scegliere **Nuovo registro (prima emissione)** soltanto se non esistono precedenti registrazioni. Il nuovo registro risiede in `~/Library/Application Support/GrainsDosage License Generator/licenses.sqlite`, con permessi riservati al proprietario.
5. La chiave viene salvata nel **Portachiavi login** del Mac con etichetta **GrainsDosage Publisher — RSA signing**. Il percorso del registro viene conservato nello stesso Portachiavi. Durante l’uso normale non si seleziona più il PEM. Il Portachiavi può richiedere lo sblocco o l’autorizzazione all’app, soprattutto dopo un aggiornamento dell’app.

La chiave privata non è incorporata nell’app, nel repository o nell’artifact. Nel bundle è presente soltanto la chiave pubblica esistente. Le firme sono **RSA-2048 PKCS#1 v1.5 con SHA-256**, verificate subito dopo la generazione. Non esistono codici dimostrativi né modalità di firma simulata nella build distribuita.

Se il registro configurato manca o è illeggibile, l’emissione si blocca: ripristinare il backup aggiornato nel percorso originale. L’app non ricrea automaticamente un registro vuoto. **Mostra registro** individua il file per backup e assistenza. Conservare un solo registro autorevole e chiudere il generatore e gli eventuali strumenti CLI prima di copiarlo per un backup. Non alternare registri indipendenti o copie vecchie: offline non è possibile ricostruire tutte le attivazioni già consegnate.

## TRIAL 14 DAYS

1. Selezionare **TRIAL 14 DAYS**.
2. Incollare la Machine ID completa nel campo grande, oppure premere **Incolla**. Sono accettati `GDDEV2-` seguito da 64 caratteri esadecimali minuscoli per macOS, i 64 caratteri della Machine ID Windows, oppure l’intero JSON della richiesta Windows.
3. Premere **Genera Trial 14 Giorni** e scegliere la cartella del cliente.
4. Viene salvato esclusivamente **GrainsDosage-Trial-Key.txt** in quella cartella. Il contenuto completo è anche visibile e selezionabile nella finestra; **Copia codice** lo copia integralmente.

Il periodo dura **14 giorni di calendario dalla prima emissione per quella Machine ID**, registrati in UTC. Non è un monte ore di riproduzione e non esiste un limite di un’ora per sessione. Le date di inizio e scadenza sono incluse nella firma. La riemissione sullo stesso registro conserva le date originali, anche se il trial è già scaduto: non lo rinnova.

Il TXT contiene solo il codice, senza intestazioni aggiuntive. Su Mac il codice è una riga `GDS2.…`; su Windows è un documento firmato di cinque righe. Copiare tutte le righe, senza modificarle.

## PERMANENT

1. Selezionare **PERMANENT**.
2. Inserire il **nome cliente**. Riutilizzare la stessa denominazione: spazi esterni e maiuscole/minuscole ASCII non creano un nuovo cliente. Un nome differente identifica un altro cliente.
3. Lasciare **License ID** vuoto per trovare la licenza esistente o crearne una alla prima emissione. Se il vecchio registro contiene più licenze con lo stesso nome, inserire il `GDL2-…` originale da usare; l’app non ne sceglie una arbitrariamente. Il License ID mostrato dopo l’emissione può essere conservato con la fattura del cliente. Cambiando il nome, il campo ID si azzera.
4. Incollare la Machine ID completa.
5. Premere **Genera Licenza Permanente**, scegliere la cartella e inviare esclusivamente **GrainsDosage-Permanent-Key.txt**.

Sono consentiti **al massimo tre dispositivi distinti per License ID**, conteggiando insieme Windows e Mac. Una seconda emissione per lo stesso dispositivo non consuma un altro posto. Le transazioni SQLite serializzano anche richieste contemporanee. Un quarto dispositivo viene rifiutato; non esiste un pulsante per eliminare una registrazione e liberare abusivamente un posto, perché le licenze offline già consegnate non possono essere revocate.

La registrazione viene confermata nel database **prima** che il codice sia esposto o salvato nel TXT. Se il salvataggio del TXT fallisce, ripetere l’emissione con lo stesso cliente e dispositivo: il posto già assegnato viene riutilizzato. Non creare un altro cliente o registro per aggirare il limite.

## Installazione del codice sul computer del cliente

| Computer | Operazione |
| --- | --- |
| macOS | Nella build commerciale aggiornata AU/VST3, cliccare l’area licenza nell’intestazione, copiare l’intero `GDS2.…` dal TXT e premere **Activate**. |
| Windows | Cliccare **ACTIVATE** nel plugin, incollare il TXT completo e premere **Activate**. Non occorre riavviare Cubase. In alternativa usare **GrainsDosage Trial Registration** → **No**, importare il TXT e ricaricare il plugin. La permanente sblocca senza scadenza la stessa build Windows. |

Usare le build del plugin pubblicate con questo aggiornamento per **trial GDDEV2 firmati** e **permanenti Windows**: le vecchie build non conoscono questi due formati. Le permanenti macOS GDS2 già emesse e i trial Windows precedenti rimangono validi senza cambiare chiave. Il trial Mac aggiornato richiede ora l’attivazione firmata; il precedente contatore locale non concede da solo l’autorizzazione. Le build di sviluppo non commerciali rimangono strumenti interni e non devono essere distribuite ai clienti.

La verifica è offline: il codice è legato alla Machine ID, non al nome del file. Spostare il TXT su un altro computer non lo rende valido. Non modifica né cancella progetti. Su entrambe le piattaforme l’effetto passa gradualmente al segnale dry in 20 ms quando il processing non è autorizzato, preservando la latenza dichiarata al DAW. Le date firmate non si estendono reinstallando il plugin. Modifiche al sistema/hardware possono cambiare la Machine ID. Clonazione dell’identità hardware, modifica del binario e ripristino integrale di vecchi backup non possono essere esclusi in modo assoluto da una soluzione interamente offline.

## Note di manutenzione

- Non inserire la chiave privata nei secret del workflow: la compilazione dell’app non ne ha bisogno.
- Il workflow usa chiavi temporanee per i test, eliminate prima del packaging e mai incluse negli artifact. Un allowlist verifica il contenuto del bundle.
- La chiave pubblica macOS è `src/license_public_key.h`; quella Windows configurata di default è `config/windows-trial-public.b64`, rappresentazione CNG della stessa chiave. Un’eventuale variabile GitHub `GRAINS_TRIAL_PUBLIC_KEY` deve corrispondere a questa chiave, altrimenti il generatore e quella build Windows non saranno compatibili. Non cambiare la coppia già usata dai clienti.
- I vecchi script CLI sono strumenti separati: per il normale lavoro usare questa app e lo stesso registro persistente. Lo script storico di trial Windows non registra automaticamente le emissioni nel nuovo ledger; non alternarlo al generatore per concedere rinnovi involontari.
- Per compilare sul Mac: `bash tools/license-generator/build.sh`. L’app e lo ZIP vengono creati in `build-license-generator/`; nessuna chiave privata è necessaria alla compilazione.
