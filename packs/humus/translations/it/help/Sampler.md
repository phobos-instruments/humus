# Sampler

Un lettore di campioni polifonico mappato a zone, con otto slot, caricamento di banchi di suoni e registrazione direttamente dalla patch.

Carica fino a otto file audio, dai a ognuno una nota di riferimento e suonali via MIDI da un MidiIn, da un PianoRoll o dalla tastiera a schermo. Oppure carica un intero file di banco di suoni (.sf2, o le librerie di campioni .nsmp3 e .nsmp4 di una tastiera da palco) e suona i suoi strumenti; un banco caricato è lo strumento, e gli slot suonano solo quando non c'è nessun banco caricato. Collega un segnale all'ingresso audio e Record cattura un take direttamente in uno slot, come file audio che si salva con la patch. Parte grezzo, dodici bit a 32 kHz, nella tradizione dei sampler rack di fine anni Ottanta; la fila dei preset va da Ancient a Pristine, e muove Bits e Rate e nient'altro. La velocity scala il livello, e oltre Polyphony la voce più vecchia viene tagliata.

## Slot e banchi

In modalità Pitched le zone si dividono la tastiera a metà strada tra le loro note di riferimento, e le note vengono intonate ricampionando dalla nota di riferimento più vicina; una zona sola copre tutta la tastiera. In modalità Kit ogni zona suona solo sul suo tasto di riferimento, alla velocità originale, per one-shot su singoli tasti. Un banco porta le sue divisioni della tastiera, i suoi punti di loop e i suoi inviluppi, e allora le manopole dell'inviluppo li scalano invece di sostituirli. I take finiscono in una cartella di campioni dentro Documenti.

## Parametri

**Mode** Pitched distribuisce le zone sulla tastiera in base alla nota di riferimento. Kit suona ogni zona solo sul suo tasto di riferimento.

**Gain** Livello d'uscita.

**BendRange** Fin dove arriva la rotella del pitch a fine corsa, in semitoni. Due è il valore di default più comune; zero ignora la rotella.

**Attack** Tempo di salita di ogni nota in millisecondi. Con un banco caricato scala l'attacco della zona stessa, quindi il valore di default suona il banco così com'è scritto.

**Decay** Tempo di discesa fino al livello di Sustain in millisecondi, scalato allo stesso modo con un banco.

**Sustain** Il livello tenuto, da 0 a 1.

**Release** Tempo di dissolvenza dopo che rilasci il tasto, in millisecondi.

**Polyphony** Il numero di voci che suonano insieme. Oltre, la voce più vecchia viene tagliata.

**Loop** Attivo, tiene la nota sui punti di loop che porta il file, finché tieni giù il tasto; un file che non ne ha suona comunque fino in fondo. Disattivato, ignora ogni punto di loop e lascia andare ogni nota fino alla fine del suo campione: è la scelta giusta quando i loop di una libreria sono fatti male. Il giro del loop è in crossfade, così un loop i cui estremi non combaciano tiene invece di fare un clic a ogni ciclo.

**File1** Il file audio nello slot 1, e così via da File2 a File8.

**Root1** La nota MIDI su cui lo slot 1 suona al suo pitch originale, 60 di default, e così via da Root2 a Root8. Un file che memorizza la propria nota la imposta al caricamento; per un kit, metti ogni nota di riferimento sul tasto dove vuoi quel suono.

**FileBank** Un file di banco di suoni. Finché ce n'è uno caricato, suona quello.

**Preset** Quale strumento del banco risponde; gli strumenti sono elencati per nome.

**RecSlot** Lo slot in cui finisce un take.

**RecordDuration** Ferma da solo un take dopo quel numero di millisecondi. A 0 un take va avanti finché non lo fermi tu o fino al tetto di trenta secondi.

**Record** Avvia un take dall'ingresso audio. Premilo di nuovo per fermarlo.

**Bits** Lunghezza della parola del campione, da 8 a 24. Sotto 24 il file viene memorizzato in modo grezzo, come facevano i primi sampler hardware, con un dither perché la grana diventi un fondo costante invece di un ronzio che segue la nota mentre si spegne. Muoverlo rilegge il file. Le frecce passano per le profondità standard, 8, 12, 16 e 24; scrivi qualsiasi altro numero nel riquadro.

**Rate** Frequenza di campionamento in kHz, da 4 a 48. Il file viene filtrato alla nuova frequenza prima di essere sfoltito, come faceva il convertitore all'ingresso di un sampler rack, così quello che stava sopra il nuovo tetto si spegne invece di ripiegarsi in un tintinnio metallico. A 48 il file resta com'è. Le frecce passano per le frequenze standard, 4, 8, 11, 16, 22, 32, 44 e 48; scrivi qualsiasi altro numero nel riquadro.

**Monitor** Fa passare l'ingresso alle uscite, così senti quello che stai per registrare. Disattivato di default, così un Sampler già collegato a un mixer non raddoppia il segnale.

**Threshold** Trattiene un take armato finché l'ingresso non arriva a quel livello, disegnato come il segno sopra il misuratore. Tutto a sinistra registra nell'istante in cui armi.

## Ricetta

**Kit dalla patch** Mode Kit, Root1 36, Root2 38, Root3 42. Collega un Kick all'ingresso audio, RecSlot 1, Threshold 0.05, Record attivo, e fai partire il Kick una volta; ripeti per gli altri slot con altre sorgenti. Collega un Riff o un PianoRoll all'ingresso MIDI e suona il kit.

## Organism correlati

PianoRoll, MidiIn
