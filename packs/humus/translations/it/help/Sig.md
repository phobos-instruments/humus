# Sig

Trasforma un valore di controllo in un segnale audio, con una rampa così non fa mai clic.

I cavi portano suono e i numeri arrivano sulle manopole; Sig è l'unico organism che rimanda fuori un numero come cavo, per i pochi casi in cui serve. Quasi sempre un Follower, un sensore o un LFO vanno dritti su una manopola con un cavo di controllo, ma un Number che deve stare dentro una formula di Math come segnale, uno Slider che pilota un'uscita in tensione di controllo attraverso un AuxOut con la protezione DC disattivata, o una costante sommata a un cavo, hanno tutti bisogno di Sig. Qualunque cosa arrivi sulla sua presa Value esce dall'uscita come un segnale stabile.

## Parametri

**Value** Il numero da trasformare in segnale. Collegaci qualsiasi cosa, o scrivilo.

**Slew** Quanto tempo impiega un cambiamento ad arrivare, in millisecondi. A 0 un cambiamento arriva in un solo campione; i 5 ms di default smussano i gradini senza un ritardo percepibile.

## Ricetta

**Offset in Math** Collega uno Slider al Value di Sig e l'uscita di Sig all'ingresso b di Math, con l'audio su a. Scrivi a+b in Math e lo Slider aggiunge un offset DC al segnale; imposta Slew a 200, così muovendo il fader il valore scivola invece di andare a gradini.

## Organism correlati

Number, Slider, Follower, Math
