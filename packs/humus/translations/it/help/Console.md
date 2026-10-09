# Console

Un banco di somma analogico: un mixer i cui canali condividono la stessa linea di alimentazione e lo stesso trasformatore d'uscita, e quindi reagiscono l'uno all'altro.

Un Mixer somma i suoi ingressi; una Console riproduce quello che fa da colla in un banco. Ogni canale è una coppia stereo numerata secondo i suoi ingressi, 1-2, 3-4 e così via. Gain e Pan collocano ogni canale sul bus, Crosstalk fa trapelare il bus da un lato all'altro, Sag abbassa tutto il bus in base a quanto sta lavorando, e Drive satura il trasformatore. Siccome Sag e Drive leggono il bus sommato, una cassa forte su un canale abbassa e piega tutti gli altri canali; è questa la colla, ed è per questo che il banco va ascoltato con il mix che suona e non su un colpo singolo. È un bus, non un channel strip: non ci sono né EQ né compressore, quindi metti un TransientShaper o un SideChain su ogni sorgente prima della Console. Collegala dove andrebbe un Mixer, prima del SoundOut.

## Il banco

Il menu a tendina Inputs stabilisce quanti canali stereo somma il banco, da 2 a 8, e cambia la classe dell'organism sul posto, così il suo nome, le impostazioni, i cavi e l'automazione restano. Se lo aumenti, i canali si aggiungono in fondo; se lo riduci, spariscono i canali più alti con i loro cavi. Un solo cavo nell'ingresso sinistro di un canale alimenta entrambi i lati, così una sorgente mono sta al centro e Pan la posiziona.

## Parametri

**Output** Il trim generale, applicato per ultimo, dopo ogni colorazione.

**Direct** Disattiva tutti gli stadi che colorano il suono ma mantiene la somma e Output. Quando è attivo il bus è la semplice somma dei suoi canali, per un confronto A/B onesto.

**Drive** Quanto viene spinto il trasformatore. La sua saturazione è asimmetrica, ispessisce le basse e arrotonda i transienti; a zero lo stadio è fuori dal percorso.

**Crosstalk** Quanto delle alte di ogni lato trapela nel lato opposto. Discreto per scelta, arriva al massimo intorno a -30 dB; si sente di più sul materiale che cambia tra sinistra e destra.

**Sag** Quanto la richiesta complessiva abbassa l'alimentazione, che poi si riprende mentre la musica respira. Basso e lento fa da colla; più alto pompa con la cosa più forte sul banco.

**Flavor** Regola il carattere di tutti e tre gli accoppiamenti in una volta. Clean è una somma trasparente. British è caldo e coeso, con un'alimentazione lenta. American è incisivo, con un'alimentazione più rapida e un carattere più duro. Modern è quasi lineare: un'alimentazione rigida, poco crosstalk, quasi niente ferro.

**Gain_1-2** Il livello del canale 1 nel bus. E così via da 3-4 a 11-12.

**Pan_1-2** Il bilanciamento del canale 1. Al centro passano entrambi i lati a livello pieno; spostandolo dal centro si abbassa solo il lato opposto. E così via da 3-4 a 11-12.

**Mute_1-2** Silenzia il canale 1, e questo alleggerisce anche l'alimentazione per gli altri. E così via da 3-4 a 11-12.

**Solo_1-2** Silenzia tutti i canali tranne questo. Il Mute del canale stesso ha sempre la precedenza. E così via da 3-4 a 11-12.

**Name_1-2** L'etichetta di nastro adesivo sopra il canale 1. Vuoto finché non ci colleghi qualcosa, poi mostra il nome di quell'organism. Fai doppio clic per scrivere il tuo; cancellalo per tornare al nome di ciò che è collegato. E così via da 3-4 a 11-12.

## Ricetta

**Mix coeso** Scegli prima Flavor, perché sposta il punto di riferimento per tutto il resto. Regola i Gain dei canali per il bilanciamento con Drive e Sag bassi, alza Sag finché il mix non respira con la cassa, poi aggiungi Drive per ultimo per dare peso. Se pompa in modo troppo evidente, abbassa il canale più forte invece di Sag, e recupera il livello perso con Output.

## Organism correlati

Mixer, Bus, SideChain, TransientShaper
