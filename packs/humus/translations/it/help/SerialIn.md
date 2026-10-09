# SerialIn

Legge numeri da una porta seriale e li mette sulle uscite di controllo dalla a alla h.

Collega una scheda a microcontrollore che stampa le sue letture sulla connessione seriale, e ogni numero di una riga finisce sulla sua uscita: il primo su a, il secondo su b e così via. Separa i numeri con spazi, virgole o punti e virgola, e metti una parola prima di un numero per mandarlo, senza cavi, al Var con quel nome. Un modello in Parse legge righe o pacchetti di byte che non sono semplici numeri, e finché ce n'è uno impostato una nona uscita chiamata match manda un impulso ogni volta che una riga corrisponde. La porta viene letta fuori dal percorso audio, un dispositivo ricollegato si riconnette da solo, e qualsiasi parametro può scegliere Controlla con SerialIn dal suo menu del clic destro. Collega le uscite alle prese di Number, Gain o LFO.

## Parametri

**Device** Quale porta seriale ascoltare. Auto prende la prima porta trovata; il resto dell'elenco sono tutte le porte presenti.

**BaudRate** La velocità della porta, che deve corrispondere a quella aperta dal firmware. 9600 e 115200 sono le scelte più comuni.

**Values** Quante delle otto uscite mostra l'organism, a partire da a.

**Frame** Bit di dati, parità e bit di stop: 8N1 è quello che usa quasi tutto; 8E1, 8O1 e 8N2 ci sono per gli apparecchi che li richiedono.

**Custom** Qualsiasi altro baud rate, scritto a mano. 0 lascia decidere all'elenco BaudRate.

**Reset** Manda un impulso sulla linea di reset della scheda quando la porta si apre, così il suo programma riparte dall'inizio a ogni connessione. Disattivato, lascia la scheda in funzione anche quando ti riconnetti.

**Reconnect** Chiude la porta e la riapre, per tutti gli organism che la condividono.

**Parse** Il modello a cui deve corrispondere una riga o un pacchetto: da %1 a %8 catturano numeri scritti come testo, da %b1 a %b8 singoli byte e da %w1 a %w8 parole di due byte, entrambi letti da 0 a 1. Vuoto, prende tutti i numeri della riga in ordine; una riga che non corrisponde viene ignorata e mostrata come saltata.

**Port** Un percorso di porta scritto a mano, che sostituisce la scelta di Device finché è impostato.

## Ricetta

**Sensore di luce sul filtro** Fai stampare alla scheda il valore del sensore come numero da 0 a 1, trenta volte al secondo, uno per riga. Lascia Parse vuoto, Values 1, BaudRate 115200. Fai clic destro sul Cutoff di un Filter, scegli Controlla con SerialIn a, e il filtro segue il sensore. Guarda il display sotto Parse per verificare cosa arriva.

## Organism correlati

SerialOut, Number, Follower, LFO
