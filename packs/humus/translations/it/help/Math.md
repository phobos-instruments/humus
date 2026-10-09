# Math

Una formula di una riga che gira una volta per campione, come oscillatore, processore o modulatore.

Prima abbassa il volume: Math manda dritto all'uscita qualsiasi cosa scrivi, e una formula può essere a fondo scala, pura componente continua o un tono ben fuori dall'udibile. I due ingressi arrivano come a e b, le quattro manopole come x, y, z e w, l'ingresso MIDI come note, freq, gate e vel, e il clock mette a disposizione t, beat, bpm e sr. Una bozza valida suona subito, e una modifica scritta a metà lascia girare l'ultima espressione buona. Ci sono due uscite: una riga che non nomina mai ch va a entrambe, una che lo nomina viene calcolata per ciascun lato. Il valore di controllo out segue l'ultimo campione di ogni blocco, quindi con Controlla con puoi mappare la formula su qualsiasi manopola. Collega un PianoRoll all'ingresso MIDI per avere una voce, o un segnale audio ad a per avere un processore.

## Il vocabolario

Numeri, parentesi, + - * / %, confronti che rispondono 0 o 1, if(c, then, else); i termini accostati si moltiplicano, quindi 2a funziona. Funzioni: sin cos tan tanh asin exp log log2 sqrt abs floor ceil fract pow min max clamp mix step smoothstep, le forme di fase saw tri sqr pulse, ph(hz), harm(hz, n, tilt), env(gate, attack, release) e noise(). prev è il campione d'uscita precedente, dt è un campione in secondi, e pi, tau e la costante e sono già definiti. Le istruzioni si separano con il punto e virgola, e nome := valore conserva un valore da un campione all'altro.

## Parametri

**Expression** La formula. a e b sono gli ingressi, x, y, z e w le manopole, ch il lato dell'uscita, e qualsiasi altro nome è il Var con quel nome collegato a Vars (in minuscolo, con gli underscore al posto degli spazi). Viene salvata così come l'hai scritta, valida o no.

**X** La variabile x della manopola, da 0 a 1. Automatizzala, mappala o mettila nel morphing. Un Var chiamato x collegato a Vars guida questa manopola: la manopola si muove con il Var e il suono la segue.

**Y** La variabile y della manopola, da 0 a 1.

**Z** La variabile z della manopola, da 0 a 1.

**W** La variabile w della manopola, da 0 a 1.

**Auto** Attivo, la formula suona da sola, con freq al valore della manopola Freq e gate a 1. Disattivato, suona solo mentre è tenuta una nota MIDI, al pitch di quella nota.

**Freq** Il pitch che porta freq mentre Auto è attivo e nessun tasto è premuto, da 20 a 2000 Hz.

**Vars** Un ingresso per ogni Var che vuoi mostrare sulla patch. Collega un Var e il suo nome fa il resto: x, y, z e w guidano le loro manopole, qualsiasi altro nome viene letto dalla formula. Contano solo i Var collegati: togli il cavo e il legame sparisce.


## Ricetta

**Voce suonabile** Auto disattivato, Expression `sin(tau*ph(freq)) * vel * env(gate, 0.01, 0.3)`, e collega un PianoRoll all'ingresso MIDI. Sostituisci `sin(tau*ph(freq))` con `harm(freq, 1 + x*15, y*2)`: x conta le armoniche e y dà loro forma.

**Controlli con un nome** Crea un Var chiamato x e uno chiamato depth, collegali entrambi a Vars e scrivi `sin(tau*ph(freq)) * depth * x`. Tira un cavo da uno Slider o da un LFO al Value di ogni Var: la manopola X si muove con il primo, la formula legge il secondo, fino a otto nomi.

## Organism correlati

Number, LFO, Follower, Var, Wave
