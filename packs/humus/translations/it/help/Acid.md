# Acid

Un synth monofonico per linee, nella tradizione delle acid box, con accent e slide.

Un dente di sega o un'onda quadra a banda limitata entra in un filtro ladder a diodi risonante, che un inviluppo esponenziale apre a ogni nota. Due cose fanno parlare le sue linee: una nota con velocity 110 o più è un accent, che apre di più il filtro e colpisce più forte, e due note sovrapposte scivolano di pitch senza far ripartire l'inviluppo. Il ladder e gli stadi d'uscita che modellano le sue basse sono la nostra versione di quelli di [Open303](https://github.com/RobinSchmidt/Open303) di Robin Schmidt. Collega un Riff, uno Steps o un PianoRoll al suo ingresso MIDI, e con il suo editor in primo piano la tastiera del computer lo suona direttamente.

## Parametri

**Wave** Forma dell'oscillatore: SAW, SQR, PLS o TRI.

**Cutoff** La frequenza base del filtro. L'inviluppo parte da qui e sale.

**Resonance** Enfasi del filtro, da un'attenuazione morbida allo squelch pieno.

**EnvMod** Quanto l'inviluppo porta la frequenza di taglio sopra Cutoff.

**Decay** Quanto velocemente il filtro si richiude dopo l'apertura, in millisecondi.

**Accent** Quanto più forte colpisce una nota con accent. Zero rende ogni nota normale.

**Glide** Quanto ci mette uno slide tra note sovrapposte ad arrivare, in millisecondi. 60 è l'impostazione classica.

**Attack** Quanto velocemente sale il volume all'inizio di una nota, in millisecondi.

**AmpDecay** Quanto velocemente una nota tenuta scende verso Sustain. In cima alla sua escursione la nota non scende affatto.

**AccentDecay** Il decadimento del filtro sulle note con accent, in millisecondi, così gli accent possono suonare più a lungo o chiudersi più secchi delle note normali.

**FilterFM** Rimanda l'uscita alla frequenza di taglio, così il filtro si modula da solo. Zero è pulito, più alto è più ruvido.

**Track** Quanto la frequenza di taglio segue il pitch suonato. Zero tiene il filtro fermo qualunque sia la nota.

**Sustain** Dove si assesta una nota tenuta dopo AmpDecay. Zero è come l'acid box originale, dove ogni nota si spegne; alzandolo, la nota resta finché non la rilasci.

**Squelch** Quante basse può togliere il feedback del filtro. Valori bassi mantengono le basse anche con molta risonanza; valori alti le assottigliano nel classico squelch.

**Velocity** Quanto la velocity modella l'accent. A zero un accent è tutto o niente sopra velocity 110; a uno ogni velocity dà la sua quantità di accent.

**Punch** L'inviluppo del filtro alza anche il volume, di più sugli accent. Zero lo disattiva.

**Muffler** Un passa-basso sull'uscita, per domare gli acuti quando Drive è alto.

**Drive** Saturazione in uscita.

**LfoWave** La forma di un'oscillazione in più sulla frequenza di taglio: SIN, TRI, SQR, SAW, DSAW o S&H.

**LfoRate** La velocità dell'oscillazione in Hz quando LfoSync è disattivato.

**LfoDepth** Quanto l'oscillazione muove la frequenza di taglio. Zero la disattiva.

**LfoSync** Aggancia l'oscillazione al transport invece che a LfoRate.

**LfoBeats** La durata di un ciclo dell'oscillazione in beat quando LfoSync è attivo.

**Level** Volume d'uscita.

**BendRange** Fin dove arriva la rotella del pitch a fine corsa, in semitoni. Due è il valore più comune; zero ignora la rotella.

## Ricetta

**Linea squelch** Wave SAW, Cutoff 400, Resonance 8, EnvMod 0.7, Decay 250, Accent 0.6, Glide 60, Drive 0.4. Collega un Riff con qualche step accentato e uno slide, poi muovi Cutoff a mano lungo la battuta.

## Organism correlati

Riff, Steps, Microdot
