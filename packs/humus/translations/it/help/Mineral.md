# Mineral

Un synth di suoni percossi, con le parziali disposte su un reticolo geometrico: ognuna è la precedente moltiplicata per Ratio.

Quella serie è lo spettro inarmonico del vetro percosso, delle campane e dei metalli del gamelan, e la forma del reticolo è il timbro. Quattro voci suonano dall'ingresso MIDI, quindi mettilo in sequenza con un PianoRoll o un DNA, oppure suonalo dalla tastiera mentre il suo editor ha il focus. Il sigillo a sinistra dell'editor disegna il modello dal vivo: un lato per ogni faccetta, allungato da Ratio, raddoppiato da Shimmer, e lampeggia a ogni colpo. Collega l'uscita a un Verbatim o a un Fern per dare lo spazio che una campana richiede.

## Parametri

**Facets** Quante parziali, da 1 a 12. Poche fanno una barra percossa; tante si riempiono fino a diventare una campana.

**Ratio** La costante del reticolo. 2.0 sono ottave, 1.62 la classica inarmonicità da campana, più basso è ammassato e metallico.

**Geometry** Affida Ratio al numero di faccette: la diagonale di un poligono regolare divisa per il suo lato, così cinque faccette danno 1.618 e più faccette salgono verso l'ottava. La manopola Ratio si oscura mentre è attivo.

**Shine** La pendenza del livello attraverso le faccette, da opaco a vetroso.

**Decay** Il tempo di risonanza in millisecondi. Le faccette più alte si spengono sempre prima.

**Strike** Il transiente del contatto, un breve burst di rumore.

**Shimmer** Scorda ogni faccetta in modo leggermente diverso a sinistra e a destra, così il suono batte attraverso il campo stereo.

**Level** Livello d'uscita.

**BendRange** Fin dove arriva la rotella del pitch bend a fine corsa, in semitoni. Due è il valore predefinito più comune; zero ignora la rotella.

## Ricetta

**Gamelan** Facets 6, Ratio 1.62, Shine 0.75, Decay 1200, Strike 0.5, Shimmer 0.4. Collega un DNA all'ingresso MIDI per una linea pentatonica e l'uscita a un Verbatim con una coda lunga.

## Organism correlati

DNA, Rhizome, PianoRoll
