# Grit

Un synth chiptune costruito sul chip sonoro Ricoh 2A03 della console, con uno slot per cartuccia per i chip di espansione del Famicom.

La console da sola ha due voci pulse con i quattro duty cycle dell'hardware, un'onda triangolare che sta sotto la nota tenuta più bassa, un canale di rumore sui tasti alti e un campionatore delta a 1 bit sui tasti bassi. Inserisci una cartuccia e le sue voci si aggiungono a quelle disponibili e prendono le note per prime, mentre le voci pulse della console entrano quando quelle della cartuccia sono tutte occupate; sul VRC6 parte per primo il dente di sega. L'inviluppo conta in frame per ogni gradino di volume su sedici livelli, quindi un attacco veloce scatta e uno lento sale in modo udibile. L'emulazione è quella di [NSFPlay](https://github.com/bbbradsmith/nsfplay), pilotata registro per registro. Collega un PianoRoll, un Riff o un DNA all'ingresso MIDI.

## Parametri

**Cartridge** Il chip nello slot. 2A03 è la console nuda; VRC6 aggiunge due pulse e un dente di sega, MMC5 due pulse, FDS una voce wavetable, N163 quattro voci wavetable, 5B tre onde quadre fisse e VRC7 sei voci FM a due operatori. Cambiarlo silenzia le note tenute.

**Region** Console NTSC o PAL. Clock, tabelle di pitch e tick dell'inviluppo si adeguano, quindi PAL va un po' più lento e più scuro.

**Duty** Larghezza d'impulso per le pulse della console, del VRC6 e dell'MMC5: 12%, 25%, 50% o 75%.

**Attack** Frame per gradino di volume in salita. 0 fa partire una nota a volume pieno.

**Decay** Frame per gradino di volume in discesa fino a Sustain.

**Sustain** Il livello su cui si assesta una nota tenuta, da 0 a 15.

**Release** Frame per gradino di volume dopo che hai rilasciato il tasto.

**Triangle** Tri layer: mette il canale a onda triangolare della console sotto la nota tenuta più bassa, allo stesso pitch. Non ha un controllo del volume, solo attivo o disattivato.

**NoiseKeys** Noise keys: da C7 in su i tasti suonano il canale di rumore invece di un tono, scorrendo i sedici periodi dell'hardware man mano che sali.

**Buzz** Mette il rumore nella sua modalità a loop corto, un crepitio metallico invece di un fruscio diffuso.

**Wave** La wavetable suonata da FDS e N163: SIN, TRI, SAW, SQR o ORG.

**Patch** Uno dei quindici strumenti FM integrati del VRC7, per numero.

**Sample** Un suono per il canale delta, ridotto a 1 bit. I tasti sotto C2 lo fanno partire alle sedici velocità di riproduzione dell'hardware, sempre più grave scendendo da B1.

**DpcmLoop** Loop: fa girare il campione in loop finché tieni premuto il suo tasto.

**Level** Livello d'uscita.

**BendRange** Fin dove arriva la rotella del pitch a fine corsa, in semitoni. Due è il valore predefinito più comune; zero ignora la rotella.

## Ricetta

**Lead da cartuccia** Cartridge VRC6, Duty 25%, Attack 0, Decay 6, Sustain 10, Release 3, Tri layer attivo. Collega un Riff all'ingresso MIDI per la linea e metti sotto un Kick con 4/4 attivo. Trascina un breve colpo di batteria in Sample e suonalo da B1 su un PianoRoll.

## Organism correlati

pH, Silt, Riff
