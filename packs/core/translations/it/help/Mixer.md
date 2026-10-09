# Mixer

Somma più segnali in uno, con Gain, Mute e Solo per ogni ingresso e un guadagno e un mute generali.

Collega le sorgenti agli ingressi numerati e l'uscita a un SoundOut o a un Bus. Il menu a tendina Inputs in alto stabilisce quanti ingressi ha, da 2 a 8, e Mode sceglie tra Stereo pairs, Mono channels o Pan inputs; cambiare l'uno o l'altro ridimensiona il mixer sul posto, mantenendo il suo nome e i suoi cavi. In modalità Stereo l'ingresso sinistro di una coppia fa anche da jack mono: un cavo nel lato sinistro alimenta entrambi i lati del mix, così una sorgente mono sta al centro, e collegando anche il lato destro torni al vero stereo. La modalità Pan dà a ogni ingresso mono la sua manopola Pan, con una legge di bilanciamento che lascia il centro a pieno livello e abbassa solo il lato opposto.

## Parametri

**MasterGain** Livello d'uscita dopo i guadagni degli ingressi, fino al doppio del guadagno unitario.

**MasterMute** Silenzia tutto il mix.

**Gain_1-2** Livello della coppia d'ingresso 1-2, e così via per 3-4 fino a 7-8. In cima è a guadagno unitario; il misuratore del canale mostra cosa arriva prima del guadagno.

**Mute_1-2** Silenzia l'ingresso 1-2 senza clic, e così via per le altre coppie. Un ingresso in mute resta muto anche se è in solo.

**Solo_1-2** Fa sentire l'ingresso 1-2 da solo, e così via per le altre coppie. Finché un ingresso è in solo, si sentono solo gli ingressi in solo.

## Ricetta

**Mix a quattro sorgenti** Imposta Inputs su 4 e Mode su Stereo, collega un organism di batteria, un basso, un synth e il ritorno di un Fern alle coppie da 1-2 a 7-8, e collega l'uscita al SoundOut. Abbassa ogni Gain finché il misuratore del SoundOut lascia un po' di headroom sulla battuta più forte, metti in solo un ingresso alla volta per controllare ogni parte, e regola il livello complessivo con MasterGain.

## Organism correlati

Bus, Gain, Console
