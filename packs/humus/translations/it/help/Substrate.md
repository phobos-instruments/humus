# Substrate

Un drone o un pad in evoluzione, fatto di fino a 64 strati di onde a dente di sega scordate, impilati su una scala a pioli di intervalli.

Ogni strato sale di un piolo di Interval sopra la fondamentale e ha una sua lenta deriva di pitch e di livello; gli strati si alternano nel campo stereo. Interval viene chiesto all'accordatura della patch, quindi sotto un organism Tuning la scala a pioli percorre i gradi di quella scala. Dopo otto pioli la scala a pioli ricomincia, quindi con valori alti di Layers gli strati si accumulano come copie all'unisono che vagano, e il livello resta stabile man mano che aggiungi strati. I preset contengono le scale a pioli Octaves, Fifths e Thirds. Suonalo da un PianoRoll o da un DNA, oppure lascia Drone attivo e muovi Cutoff da un Metapad.

## Parametri

**Drone** Disattivato, suona dal MIDI attraverso Attack e Release. Attivo, suona di continuo su Note, sempre come una sola fondamentale, qualunque cosa dica Mode.

**Note** La fondamentale mentre Drone è attivo, o finché non è stato suonato niente in modalità Mono.

**Layers** Quanti strati, da 1 a 64. Oltre i 16 circa smette di essere un accordo di strati e diventa una texture.

**Interval** Di quanti gradi della scala sale ogni piolo. Nell'accordatura normale 12 impila ottave, 7 quinte e 4 terze; tutto quello che sta in mezzo è un tappeto che nessuna scala a pioli fissa ti dà.

**Spread** Profondità della deriva in cent, quanto si allontanano gli strati tra loro.

**Motion** Quanto la deriva muove pitch e livello. 0 congela il tappeto.

**Cutoff** Il passa-basso del tappeto. Automatizzalo per far crescere il pad.

**Attack** Quanto ci mette il tappeto ad aprirsi, in millisecondi.

**Release** Quanto ci mette a sfumare dopo che hai lasciato le note.

**Level** Livello d'uscita.

**BendRange** Fin dove arriva la rotella del pitch a fine corsa, in semitoni. Due è il valore più comune; zero ignora la rotella.

**Mode** Come le note MIDI diventano voci. Mono è una sola fondamentale, vince l'ultima nota. Para distribuisce gli strati disponibili su tutte le note tenute, così una triade con 12 strati dà a ogni nota una pila di quattro, con lo stesso costo qualunque sia il numero di tasti premuti. Poly dà a ogni nota la pila completa con il suo inviluppo, otto voci.

## Ricetta

**Pad che si ridistribuisce** Drone disattivato, Mode Para, Layers 24, Interval 7, Motion 0.6, Attack 1500, Release 3000, Cutoff 1200. Tieni accordi lenti da un PianoRoll; gli strati si distribuiscono su ogni nota tenuta e, quando lasci una nota, tornano a quelle ancora tenute.

## Organism correlati

Tuning, Rhizome, DNA
