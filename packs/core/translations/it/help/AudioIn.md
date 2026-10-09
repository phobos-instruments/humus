# AudioIn

Un canale d'ingresso dal vivo dell'interfaccia audio, come sorgente mono.

Usalo per raggiungere un singolo ingresso qualsiasi di un'interfaccia multicanale: un AudioIn per ogni microfono o linea che vuoi trattare a parte. SoundIn porta la coppia stereo principale e può anche riprodurre un file; AudioIn è la semplice presa su un solo canale. Nelle patch più vecchie compare con i nomi da AuxIn1 ad AuxIn8, che si caricano come questo organism con Channel già impostato da 3 in su. Collega la sua uscita a un Gain, a un canale di un Mixer o direttamente a un effetto. Un canale che il dispositivo non ha resta in silenzio.

## Parametri

**Channel** Quale ingresso dell'interfaccia legge questo organism. Il menu a tendina elenca i nomi dei canali del dispositivo aperto.

## Ricetta

**Secondo microfono** Aggiungi un AudioIn, imposta Channel sull'ingresso a cui è collegato il microfono e collegalo a un canale del Mixer accanto al SoundIn. Regolalo con il Gain del canale, così le due sorgenti hanno lo stesso livello prima che un effetto le senta.

## Organism correlati

AudioOut, SoundIn, SoundOut
