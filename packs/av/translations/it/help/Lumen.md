# Lumen

Un compositor a shader che dipinge l'audio che sente e impila sotto di sé dei layer video.

Collega l'audio ai suoi due ingressi e la scena si disegna seguendolo; collega la sua uscita video a un VideoOut per vederla, così come un synth ha bisogno di un SoundOut per farsi sentire. Scene prende un file di fragment shader e lo ricarica ogni volta che il file cambia sul disco; se è vuoto, gira la scena integrata. Gli shader leggono l'audio come livello, bande, beat, tempo, una texture della forma d'onda e una texture dello spettro, e gli ingressi float propri di una scena si comandano da Knob1 a Knob4. Una scena può anche leggere i layer video sotto di lei e il suo ultimo fotogramma, quindi può fare da effetto feedback: la scena hallucination inclusa sogna sopra qualsiasi cosa colleghi. I quattro ingressi video accettano layer da VideoPlayer, VideoPad, CameraIn o VideoTrack, impilati dal basso verso l'alto, con la scena che galleggia sopra di loro secondo SceneOpacity e SceneBlend. L'opacità e la fusione di ogni layer stanno sull'organism che lo alimenta.

## Parametri

**Scene** Il file shader da eseguire: .frag, .fs, .glsl o .fsh. Se è vuoto gira la scena integrata, e le modifiche al file si ricaricano mentre suona.

**Brightness** Il livello d'uscita complessivo della scena.

**Speed** Quanto velocemente corre l'orologio della scena. 0 lo congela; 2 va a velocità doppia.

**Knob1** Un valore che la scena legge per conto suo; cosa fa dipende dallo shader. Uno shader di effetto ci prende la sua intensità.

**Knob2** Il secondo valore di questo tipo. Uno shader di effetto ci prende velocità e colore.

**Knob3** Il terzo. Uno shader di effetto ci prende frequenza, densità e numero di celle della griglia.

**Knob4** Il quarto. Uno shader di effetto ci prende larghezza e dettaglio.

**SceneOpacity** Quanto si vede la scena sopra i layer video sottostanti.

**SceneBlend** Over posa la scena sopra i layer video. Add la somma a loro, così le parti scure della scena spariscono e quelle chiare brillano.

## Ricetta

**Overlay reattivo all'audio** Collega il mix master a entrambi gli ingressi audio e un VideoPlayer al primo ingresso video. Lascia Scene vuoto, imposta SceneBlend su Add e SceneOpacity a 0.5, poi manda un Follower dalla cassa su Brightness così la scena lampeggia a tempo. Collega l'uscita a un VideoOut e aprilo dal menu Uscite video.

## Organism correlati

VideoPlayer, VideoMix, VideoOut
