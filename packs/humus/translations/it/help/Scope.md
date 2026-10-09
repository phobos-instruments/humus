# Scope

Un oscilloscopio con una traccia verde luminosa, nella tradizione dello strumento da banco. Ascolta soltanto: non ha uscite, quindi collegaci un secondo cavo da qualsiasi cosa tu voglia vedere.

Lo schermo mostra l'onda stessa: il tempo scorre in orizzontale, il livello in verticale, e la linea centrale è il silenzio. Ogni sweep parte dove l'onda sale attraversando la linea centrale, così un pitch stabile resta fermo invece di scorrere via. La traccia lascia un breve bagliore residuo, ed è più luminosa dove il fascio si muove piano e più debole su un fronte ripido, così un'onda quadra mostra tratti piatti luminosi uniti da salite pallide. Un cavo nel solo ingresso 1 disegna su entrambi i canali.

## Parametri

**Mode** STEREO disegna il canale sinistro in verde e il destro in un verde più pallido sullo stesso schermo. MONO disegna la loro somma. X/Y muove il fascio in orizzontale con il canale sinistro e in verticale con il destro: un segnale mono è una linea diagonale, uno stereo largo si apre in una nuvola, e due oscillatori scordati disegnano nodi che ruotano lentamente.

**Hold** Ferma lo schermo dov'è, così puoi leggerlo.

**Time** Il tempo mostrato sullo schermo, da 1 ms a mezzo secondo. Tempi brevi mostrano la forma di un ciclo; tempi lunghi mostrano l'inviluppo di una nota.

**Zoom** Ingrandisce la traccia, fino a otto volte, per i segnali deboli. Cambia solo l'immagine, mai il suono.

## Ricetta

**Vedere cosa fa un filtro** Collega un Wave che suona un dente di sega a un Filter, e il Filter allo Scope. Con Time vicino a 10 ms i denti aguzzi si arrotondano mentre la frequenza di taglio scende, e alzando la risonanza compaiono delle oscillazioni su ogni fronte.

**Controllare l'immagine stereo** Collegaci anche ciò che alimenta il SoundOut, e passa a X/Y. Una linea inclinata a destra è un mono sano, una linea inclinata a sinistra vuol dire che un canale è capovolto, e una nuvola tonda è uno stereo largo che potrebbe assottigliarsi su un solo altoparlante.

## Organism correlati

Spectrum, VuMeter, StereoTool, Wave
