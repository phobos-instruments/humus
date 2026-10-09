# Flanger

Un delay corto in movimento che filtra a pettine il segnale sommandolo a una copia di se stesso.

Una copia dell'ingresso viene ritardata di una frazione di millisecondo e rimescolata al segnale: alcune frequenze si cancellano e altre si rinforzano, in una fila di notch equidistanti. Uno sweep sinusoidale sposta quel ritardo tra i due estremi di FrequencyRange, così tutto il pettine scorre su e giù per lo spettro, e Feedback rimanda la copia ritardata all'ingresso, così ogni notch si stringe in una risonanza. Usalo su batterie, accordi tenuti o rumore, dove spesso il pettine in movimento è l'unico pitch che il suono ha. Chorus ritarda di più e si sente come un leggero scordamento; Phaser invece crea meno notch, spaziati in modo irregolare, spostando la fase.

## Parametri

**FrequencyRange** I due estremi dello sweep, espressi come frequenze. Una frequenza bassa è un ritardo lungo e un pettine fitto, una alta è un ritardo corto e un pettine rado e fischiante; avvicina i due cursori per un colore metallico fisso invece di uno sweep.

**Rate** Quanto velocemente lo sweep viaggia tra i due estremi, in cicli al secondo.

**Feedback** Quanta parte della copia ritardata torna all'ingresso. Basso è un soffio delicato, alto è la versione risonante che urla.

**WetDryMix** Quanto si sente la copia ritardata rispetto all'originale. Il pettine esiste solo dove i due si sentono insieme, quindi a metà la cancellazione è più profonda e tutto effettato l'effetto sparisce.

## Ricetta

**Fill effetto jet** Mettilo dopo un organism Drums. FrequencyRange da 100 a 3000, Rate 0.1, Feedback 0.7, WetDryMix 0.5. Collega un LFO a Rate se vuoi che lo sweep stesso acceleri e rallenti nel corso di una frase.

## Organism correlati

Chorus, Phaser, Fern
