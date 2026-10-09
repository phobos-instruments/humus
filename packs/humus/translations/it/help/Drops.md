# Drops

Gocce d'acqua come strumento: una pioggia casuale di piccoli eventi fisici mentre il transport suona.

Ogni goccia è una sinusoide smorzata alla risonanza di una bolla, il cui pitch sale mentre si spegne, ed è questo che fa suonare bagnato un gocciolio. Le gocce cadono a caso mentre il transport va e si fermano quando si ferma, e la stessa patch genera ogni volta la stessa pioggia, quindi un bounce non riserva sorprese. Size imposta la bolla, Spread quanto ogni goccia se ne discosta, e Pool aggiunge una piccola coda risonante sotto la pioggia. Collegalo a un Fern per gocce che si rispondono da sole, o a un SoundSpace per una caverna.

## Parametri

**Density** Gocce al secondo. Una perdita a 1, pioggia a 8, un acquazzone a 20.

**Size** La scala della bolla. A sinistra sono tick brevi e brillanti, a destra gocce grasse con un pitch ben riconoscibile.

**Spread** Quanto ogni goccia si discosta da Size. A zero ogni goccia è un clone; tutto aperto piovono gocce di ogni misura insieme.

**Chirp** La salita di pitch dentro ogni goccia. A zero le gocce sono semplici ping; oltre metà fanno bloop.

**Pool** Una piccola coda risonante e bagnata sotto la pioggia. Pietra asciutta a zero, una cisterna a uno.

**Width** Quanto si allargano le gocce nel campo stereo.

**Level** Volume d'uscita.

## Ricetta

**Perdita lenta** Density 2, Size 0.8, Spread 0.3, Chirp 0.8, Pool 0.5, Width 0.7. Fallo passare per un Fern con Feedback intorno a 0.5 e ogni goccia ritorna un beat dopo. Spingi Density a 15 e abbassa Size a 0.2 per la pioggia su una finestra.

## Organism correlati

SoundSpace, Fern, Morse
