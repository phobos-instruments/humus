# Var

Un valore di controllo con un nome, così un cavo o una linea seriale possono portare insieme il numero e la parola.

Scrivi un nome, e la presa Value esce dall'uscita come qualsiasi altra sorgente di controllo; il nome permette al cavo di dire cosa significa il numero. Un SerialOut con un Var sulla sua presa stampa il nome accanto al valore con %n1, e un SerialIn che legge una riga come "temp 23.5" passa il numero al Var chiamato temp. Un valore che arriva per nome compare nel display ed esce dall'uscita; se scrivi un nuovo Value, comanda lui fino all'arrivo del prossimo. Collega la sua uscita a qualsiasi manopola o a una presa di un SerialOut. Collegalo all'ingresso Vars di un Math e la formula lo legge per nome; se si chiama x, y, z o w, pilota quella manopola.

## Parametri

**Value** La presa, e il numero da tenere quando non arriva niente per nome. Tiraci un cavo da un LFO o da uno Slider e il Var lo segue.

**Name** La parola a cui risponde questo Var, mostrata come $ sul pannello frontale. È la parola che stampa su un SerialOut e il nome con cui lo legge una formula di Math. Lettere, cifre e underscore sono quelli che viaggiano meglio.

## Ricetta

**Sensore per nome** Name temp, e un SerialIn con Parse vuoto che legge una scheda che stampa "temp 23.5" una volta al secondo. Collega l'uscita del Var a Frequency di un Filter, e la temperatura della stanza muove la frequenza di taglio senza nessun cavo tra il SerialIn e il Var.

## Organism correlati

Math, Number, SerialIn, SerialOut, Slider
