# DNA

Un sequencer generativo che suona un genoma di sedici basi come note MIDI.

Sedici basi, A, C, G e T, girano in loop a Rate rispetto al transport, e ogni base suona il suo intervallo sopra Root: quello del voicing di Chord, o uno tuo, impostato sui tasti delle basi in fondo all'editor. A ogni battuta c'è una probabilità Mutate che una base muti, così la linea si evolve piano mentre suona, mentre lo stesso Seed fa crescere sempre lo stesso strand di partenza. La striscia sopra le righe delle basi mostra lo step in riproduzione e le pause del giro attuale; clicca una cella per silenziare del tutto quello step, trascina per coprirne diversi, fai clic destro per tutto lo strand. Collega l'uscita MIDI a un Acid, a un Mineral o a un Substrate.

## Parametri

**Root** La tonica, come nota MIDI.

**Chord** Il voicing che prendono le quattro basi: Minor, Major, Phrygian, Pentatonic, Dorian, Harmonic Minor, Whole Tone, Hirajoshi o Harmonic (JI), la serie 4:5:6:7 in intonazione naturale.

**Rate** La lunghezza dello step, da 1/64 fino a 1/1, con valori in terzina e puntati nel mezzo.

**Gate** Lunghezza della nota come frazione dello step.

**Rest** Quanto dello strand viene diradato in pause. Le pause cadono secondo Seed, e alzare Rest aggiunge sempre e solo pause alla stessa linea.

**Mute** Gli step silenziati a mano sulla striscia, un bit per step. Uno step silenziato non suona mai, qualunque cosa facciano Rest o Mutate.

**Octaves** Con 2 o 3, alcuni step saltano di un'ottava in su per allargare l'estensione. 1 lascia ogni base al suo posto.

**Mutate** La probabilità per battuta che una base muti. Zero è un riff fisso; alto è irrequieto.

**Velocity** La velocity di ogni nota.

**Seed** Quale strand cresce. Lo stesso Seed dà sempre lo stesso inizio.

**Base A** L'intervallo che la base A suona sopra Root, da 0 a 12 semitoni. -1 segue il voicing di Chord.

**Base C** Lo stesso per la base C.

**Base G** Lo stesso per la base G.

**Base T** Lo stesso per la base T.

**SwingFollow** Follow fa prendere al pattern il groove della patch dal transport invece che dalla manopola Swing.

**Swing** Ritarda ogni secondo step verso un andamento terzinato. Conta solo quando SwingFollow è disattivato.

## Ricetta

**Acid in evoluzione** Collega l'uscita MIDI a un Acid. Root 45, Chord Phrygian, Rate 1/16, Gate 0.5, Rest 0.3, Octaves 2, Mutate 0.2. Premi Riproduci e lascialo andare per qualche battuta; quando compare uno strand che ti piace, porta Mutate a zero per tenerlo.

## Organism correlati

Mineral, Substrate, Rhizome
