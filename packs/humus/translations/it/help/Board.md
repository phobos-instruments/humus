# Board

Una scheda a microcontrollore su un cavo seriale: i suoi ingressi analogici escono come uscite di controllo e le sue prese pilotano i pin della scheda.

Carica una volta sulla scheda lo sketch standard per il controllo dei pin, che arriva come esempio con l'editor della scheda stessa, collegala e scegli un preset per la tua scheda. Ogni uscita legge un canale analogico come valore da 0 a 1, qualunque sia la risoluzione della scheda, quindi un potenziometro o un sensore di luce su A0 diventa l'uscita 1: collegala a un Number per guardarla o alla presa di un Gain per modellare un livello. Ogni presa pilota un pin, accendendolo e spegnendolo o regolandone l'intensità. La scheda viene letta fuori dal percorso audio, e se la ricolleghi si riconnette da sola. Board è anche una sorgente di controllo, quindi il menu del clic destro di qualsiasi parametro offre Controlla con Board. Una scheda senza lo sketch standard viene lasciata avviare per tre secondi e poi le si parla comunque; una scheda che esegue uno sketch tuo va invece su SerialIn e SerialOut.

## La riga di stato

La riga sotto le uscite dice a che punto è il box: cerca un dispositivo, aspetta la porta, si avvia, comunica (talking), pin mappati. Le uscite restano a zero finché la scheda non è partita. I preset stabiliscono quali canali leggono le uscite e quali pin pilotano le prese; poi è la scheda stessa a dire cosa sa fare ogni pin, quindi anche una scheda che non è nella lista funziona, con i pin scritti a mano.

## Parametri

**Device** Su quale dispositivo seriale si trova la scheda. Auto sceglie il primo dispositivo seriale USB che trova; il resto della lista sono tutti i dispositivi presenti. Qualsiasi numero di box Board, SerialIn e SerialOut può condividere una porta.

**BaudRate** La velocità della porta. Lo sketch standard apre a 57600, il valore predefinito qui; cambialo solo se hai cambiato lo sketch.

**Custom** Qualsiasi velocità che non è nella lista. A zero decide BaudRate.

**Reset** Se è attivo, aprire la porta riavvia la scheda come fa il suo editor, ed è ciò che si aspetta lo sketch standard. Se è disattivato, la scheda continua a girare durante una riconnessione.

**Reconnect** Chiude la porta e la riapre per ogni box che la condivide. Usalo quando una scheda ha smesso di rispondere.

**Rate** Ogni quanto la scheda campiona e invia i suoi ingressi analogici, in millisecondi. 19 è il valore predefinito dello sketch; più basso è più reattivo e carica di più il cavo.

**Outlets** Quante uscite mostra il box, da una a sei.

**Inlets** Quante prese mostra il box, da una a sei.

**In1** La prima presa, e il livello che riceve il suo pin quando non c'è niente collegato. E così via da 2 a 6.

**Analog1** Il canale analogico letto dall'uscita 1, da 0 a 15. Di fabbrica l'uscita 1 legge A0, l'uscita 2 legge A1, e così via da 2 a 6.

**Pin1** Il pin della scheda pilotato dalla presa 1. E così via da 2 a 6.

**Mode1** Digital porta il pin alto quando la presa è sopra 0.5 e basso altrimenti. PWM imposta il duty cycle del pin dal valore da 0 a 1 della presa; solo alcuni pin supportano il PWM, e i preset li mettono sulle prese da 2 a 6. E così via da 2 a 6.

**Port** Un percorso esplicito del dispositivo, che ha la precedenza su Device finché è impostato. Di solito si lascia vuoto.

## Ricetta

**Dal sensore al filtro** Collega un potenziometro ad A0, scegli il preset della tua scheda e controlla che la riga di stato dica talking. Fai clic destro su Frequency di un Filter, scegli Controlla con Board out1, e la manopola segue il potenziometro. Collega l'env di un Follower a In2 con Mode2 su PWM e la luce del LED su quel pin segue la musica.

## Organism correlati

SerialIn, SerialOut, Number, Follower
