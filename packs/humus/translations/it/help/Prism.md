# Prism

Un risintetizzatore armonico che divide l'ingresso in sedici armoniche e dà a ognuna un fader.

All'unità un'armonica passa così come si sente, a zero viene sottratta e sopra l'unità cresce, comprese le armoniche che la sorgente non aveva; un'armonica aggiunta segue l'inviluppo della fondamentale stessa, così sboccia e muore con la nota. Refract ricostruisce il suono una parziale alla volta sopra un rilevatore di intonazione e vuole materiale intonato, una nota alla volta; Filter è un equalizzatore armonico sullo spettro stesso e gestisce accordi, batteria e rumore. Il percorso secco viene ritardato per restare allineato, così qualsiasi miscela di Mix resta coerente in fase. Il display mostra la nota su cui è agganciato il rilevatore, chiamata secondo il Tuning della patch. Dagli in pasto una voce, una linea di basso o un Wave.

## Parametri

**Mode** Refract risintetizza le parziali in mono e sfuma nel silenzio quando non si sente niente di intonato. Filter rimodella le armoniche già presenti, con lo stereo intatto, e non può aggiungerne di nuove.

**Dispersion** Solo Refract. Allarga la distanza tra le parziali, così un'armonica cade oltre il suo multiplo intero della fondamentale; i valori positivi scivolano verso campane e metallo, un tocco di negativo stringe la serie.

**Residual** Solo Filter. Il guadagno di tutto ciò che sta tra le armoniche. A 0 un suono è ridotto al suo tono; con i fader a 0 e Residual alto, il tono sparisce e restano il respiro e l'ambiente.

**Mix** Secco contro effettato.

**Level** Livello d'uscita del segnale effettato.

**Harm_1** Il fader della prima armonica, da 0 a 2 con l'unità al centro, e così via per Harm_2 fino a Harm_16.

## Ricetta

**Campana da un basso** Mode Refract, e collega un Rhizome che suona note singole. Harm_1 a 1, da Harm_2 a Harm_5 a 1.6, il resto a 0, Dispersion 6, Mix 1. Ora ogni nota risuona come una campana che segue l'inviluppo del basso stesso.

## Organism correlati

Wave, Decomposer, Tuning
