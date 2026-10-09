# StereoTool

Un'utility di immagine stereo mid/side con controlli di larghezza, rotazione, basse in mono e fase.

Il mid è ciò che i due canali hanno in comune e il side è ciò che non hanno in comune; dividere un segnale in queste due parti permette a Width e MonoBass di cambiare l'immagine stessa, mentre gli altri controlli la spostano o ne regolano il livello. L'ordine è fisso: prima PhaseL, PhaseR e Swap, poi lo stadio mid/side dove Width e MonoBass agiscono sul side, poi Rotation, Balance, Mono e Output. Lo scope nell'editor disegna il campo e la sua correlazione, così una sorgente che sparirà in mono si vede prima che tu la senta. Collegalo prima di una Console o di un SoundOut, o sul ritorno di un Send.

## Parametri

**Width** Il livello del side. 0% collassa in mono, 100% lascia l'immagine com'è, e arriva fino a 400%. Sopra il 100% assottiglia il centro mentre allarga, quindi controlla Mono mentre lo regoli.

**Balance** Il bilanciamento di livello tra sinistra e destra, il normale controllo tipo pan.

**Rotation** Ruota tutto il campo fino a 45 gradi in ciascun senso. A differenza di Balance sposta l'immagine invece di regolarne il livello, così il centro si muove insieme a tutto il resto.

**MonoBass** Sotto questa frequenza il side viene tolto e le basse collassano in mono. 0 lo disattiva; da 100 a 150 Hz è l'impostazione tipica.

**PhaseL** Inverte il canale sinistro. Ribalta un lato quando una sorgente stereo suona vuota e sparisce in mono.

**PhaseR** Inverte il canale destro.

**Swap** Scambia sinistra e destra.

**Mono** Somma l'uscita in mono. Usalo per controllare che cosa sta facendo Width sopra il 100%.

**Output** Guadagno d'uscita, applicato per ultimo.

## Ricetta

**Allargare senza perdere il mono** MonoBass 120, Width 140%, Balance e Rotation a 0. Attiva e disattiva Mono mentre ascolti; se il centro si assottiglia troppo in mono, riporta Width verso 120%. Regola Width dopo MonoBass, non prima, altrimenti giudichi una larghezza che MonoBass sta per riprendersi.

## Organism correlati

Console, Mixer, Gain, SoundSpace
