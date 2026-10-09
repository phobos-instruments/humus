# AudioOut

Un canale d'uscita live dell'interfaccia audio, come destinazione mono.

Usalo per una mandata monitor, un ascolto in cuffia, il click per il batterista o uno stem di un'uscita surround: tutto quello che va oltre il mix master. SoundOut resta l'uscita master, con il livello master e il registratore del mix; AudioOut prende un segnale e lo consegna a un canale. Le patch più vecchie lo usano sotto i nomi da AuxOut1 ad AuxOut8, che si caricano come questo organism con Channel già impostato da 3 in su. Un canale che il dispositivo non ha inghiotte il segnale in silenzio. Metti davanti un Gain o un Send per regolare il livello.

## Parametri

**Channel** L'uscita dell'interfaccia che questo organism alimenta. Il menu a tendina elenca i nomi dei canali del dispositivo aperto.

## Ricetta

**Ascolto in cuffia** Metti un Send sulla parte che il musicista deve sentire, collega la sua coppia di mandata attraverso un Gain a un AudioOut, e imposta Channel sull'uscita a cui è collegato l'amplificatore per cuffie. Il mix principale su SoundOut resta intatto, e il livello in cuffia lo regoli con il Gain.

## Organism correlati

AudioIn, SoundIn, SoundOut
