# Comber

Un banco di filtri a pettine intonati che tira fuori accordi da qualsiasi cosa ci passi attraverso.

Un filtro a pettine è un delay così corto, con feedback, che risuona a un pitch preciso invece di fare eco. Comber ne mette fino a otto uno accanto all'altro, cinque per cominciare, ognuno con il proprio pitch, decadimento e livello, così batteria, rumore o una voce escono risuonando sulle note su cui sono accordati i pettini. Ogni pettine mostra il suo pitch in tre modi: uno slider, il numero in hertz e il nome della nota più vicina con i cent di scarto. Clicca il nome della nota per sceglierne una da una tastiera, o gira la rotella sopra di esso per spostarti di un semitono alla volta. I pettini risuonano fianco a fianco e vengono sommati, quindi quello che esce è un accordo. Quando sposti un pitch, scivola invece di fare clic, e Comber lavora in stereo.

## Parametri

**InputGain** Il livello generale, applicato quando il suono entra. Abbassalo quando i decadimenti lunghi cominciano ad accumularsi.

**Combs** Quanti pettini sono in uso, da uno a otto. Le righe oltre questo numero sono nascoste e mute.

**Frequency_1** Pitch del pettine 1 in hertz, da 16 a 12000, e così via per gli altri. Il ritardo dura un periodo di questa frequenza.

**DecayTime_1** Per quanto risuona il pettine 1 dopo che l'ingresso si ferma, da 0.02 a 10 secondi, e così via per gli altri. Corto è un colpo secco intonato, lungo è una nota tenuta.

**Gain_1** Livello del pettine 1, e così via per gli altri: la sua quota nella somma, quindi a zero lo zittisce.

## Ricetta

**Un accordo da un loop di batteria** Collega in ingresso una drum machine o un loop. Accorda i cinque pettini su 110, 165, 220, 277 e 330, imposta ogni decadimento a 2, il guadagno di ogni pettine a 0.6 e InputGain a 0.5. Accorcia i decadimenti verso 0.3 per un colpo secco intonato, o spingi un pettine a 8 per una nota che resta sospesa sopra le altre.

## Organism correlati

Fern, Verbatim, ParaEQ
