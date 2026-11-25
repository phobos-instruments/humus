# Steps

Un sequencer di trigger a una riga e sedici step che manda una nota MIDI con swing, gate e velocity.

Una riga è una voce: imposta Note e collega la sua uscita MIDI a un Kick, a un Mineral o a un Sampler, poi impila diversi organism Steps, uno per pezzo della batteria, ognuno con il suo swing e il suo gate. Resta muto finché il transport non suona, e gli step vengono letti dalla posizione del beat nel transport, così cade sempre a tempo con la patch. Un colpo dura Gate di uno step, a meno che la riga di tenuta non lo leghi agli step successivi. Mettere in mute il cavo di una riga, o il suo canale sul Mixer, è il mute per suonare dal vivo.

## La griglia

Clicca uno step per mettere un colpo e cliccalo di nuovo per toglierlo. La riga ^ sopra alza quel colpo di un'ottava. La riga di tenuta sotto lega un colpo oltre il confine con lo step successivo, e una serie di tenute fa una nota lunga.

**Probabilità dello step** Clicca il tasto % in alto a destra della griglia per vedere la probabilità di ogni step come una barra, poi trascina su o giù su uno step per decidere quanto spesso suona, da mai a sempre, a scatti del 5%. Cmd-trascina (Ctrl-trascina su Windows e Linux) fa lo stesso senza cambiare modalità. Il tasto % è l'interruttore View, quindi anche un pulsante o un pad mappato lo commuta. La probabilità di ogni step è anche un parametro, da Chance_1 a Chance_16, così puoi mapparci dei fader. Nella vista Chance, fai clic destro su uno step per mapparlo. Un colpo sotto il 100% appare più sbiadito e mostra la sua probabilità sopra di sé. I dadi sono del box: una patch suona gli stessi colpi a ogni riproduzione e a ogni bounce, e due box tirano ciascuno i propri. Ogni banco tiene le sue probabilità, Copia nel banco le porta con sé e Cancella le riporta al sicuro.

## Parametri

**Note** La nota MIDI che manda ogni colpo. 36 è C1, la solita cassa.

**SwingFollow** Follow fa prendere alla riga il groove della patch dal transport invece della sua manopola Swing.

**Swing** Ritarda ogni secondo sedicesimo, fino a un andamento terzinato. Si usa solo mentre Follow è disattivato; dai agli hi-hat più swing che alla cassa e il groove si apre.

**Gate** La lunghezza del colpo come frazione dello step. Le legature vanno oltre.

**Velocity** La velocity di ogni colpo. Automatizzala per i crescendo.

**Bank** Quale degli otto pattern suona, da A a H. Ogni bank tiene i suoi step e la sua riga ^, e cambiando suona subito il nuovo.

**View** Steps o Chance: cosa mostra e modifica la griglia. Il tasto % lo cambia.

**Chance_1 a Chance_16** Quanto spesso suona ogni step, da 0 a 100%. Ogni banco tiene le sue.

## Ricetta

**Hi-hat sopra un Kick** Due organism Steps. Il primo, Note 36, suona sugli step 1, 5, 9 e 13 ed è collegato a un Kick con FourFloor disattivato. Il secondo, Note 42, suona uno step sì e uno no con Follow disattivato e Swing 0.3, ed è collegato a una zona di un Sampler con dentro un hi-hat. Alza qualche step dell'hi-hat con la riga ^ per gli accenti.

## Organism correlati

Kick, Riff, Acid
