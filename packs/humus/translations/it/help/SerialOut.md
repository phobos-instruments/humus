# SerialOut

Manda valori di controllo fuori da una porta seriale, nella forma di testo o di byte che il dispositivo si aspetta.

Collega degli Slider, un Follower, un LFO o un Number alle sue prese e scrivi nel campo Format la riga che devono formare: da %1 a %8 stampano le prese come testo, da %b1 a %b8 ne mandano una come byte grezzo e da %w1 a %w8 come word di due byte, \n chiude una riga e \xNN è un byte grezzo. Il valore di default "%1 %2\n" stampa due numeri e un a capo. Scale moltiplica un valore prima che venga stampato e Int lo arrotonda, così un controllo da 0 a 1 esce come da 0 a 255. Più righe in Format sono più messaggi, e una presa Message sceglie quale viene inviato. Il display in basso mostra la porta e gli ultimi byte inviati. SerialIn rilegge i valori allo stesso modo.

## Parametri

**Device** Con quale porta seriale parlare. Auto prende la prima porta trovata; il resto della lista sono tutte le porte presenti.

**BaudRate** La velocità della porta, che deve corrispondere a quella aperta dal firmware. 9600 e 115200 sono le scelte più comuni.

**Rate** Quante volte al secondo parte la riga in modalità Continuous, e il tetto massimo nelle altre due. Mostrato come Per second.

**Frame** Bit di dati, parità e bit di stop: 8N1 è quello che usa quasi tutto; 8E1, 8O1 e 8N2 ci sono per l'hardware che li vuole.

**Custom** Qualsiasi altro baud rate, digitato a mano. 0 lascia decidere alla lista BaudRate.

**Reset** Dà un impulso alla linea di reset della scheda quando la porta si apre, così il suo programma riparte dall'inizio a ogni connessione. Disattivato lascia la scheda in funzione durante una riconnessione.

**Reconnect** Chiude la porta e la riapre, per ogni organism che la condivide.

**Send** Continuous scrive la riga Rate volte al secondo. On change la scrive una volta ogni volta che si muove una delle prese visibili. On trigger la scrive quando sale la presa Trigger.

**Values** Quante prese mostra l'organism, e quante contano per On change. Mostrato come Sockets.

**Value1** La prima presa, e il numero che manda quando non c'è niente collegato. E così via da 2 a 8; %5 in Format stampa Value5 che la sua presa sia visibile o no.

**Trigger** La presa letta da On trigger; un Button collegato lì manda il messaggio una volta per ogni pressione.

**Message** Sceglie quale riga di un Format su più righe viene inviata, a partire da 1. Compare quando Format ha più di una riga.

**Scale1** Moltiplica Value1 prima che venga stampato o impacchettato in un byte. E così via da 2 a 8.

**Int1** Stampa Value1 come numero intero invece che con i decimali. E così via da 2 a 8.

**Format** Il modello del messaggio. Oltre ai segnaposto qui sopra, %n1 stampa il nome della sorgente su una presa, %c un byte di conteggio, %x e %X un checksum, e %(v1*180) un valore calcolato.

**Port** Un percorso di porta scritto a mano, che sostituisce la scelta di Device finché è impostato.

## Ricetta

**Servo da un LFO** Collega un LFO a Value1, imposta Scale1 a 180 e Int1 attivo, Format "M %1\r", Send Continuous, Rate 30. La scheda legge la riga e muove il servo con l'LFO; rallenta l'LFO a un ciclo per battuta e il braccio si muove a tempo con la patch.

## Organism correlati

SerialIn, Number, Slider, Button
