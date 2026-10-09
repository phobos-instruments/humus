# AudioOut

Ein einzelner Live-Ausgangskanal des Audio-Interfaces, als Mono-Senke.

Nimm es für einen Monitor-Send, einen Kopfhörermix, einen Klick für den Drummer oder einen Stem einer Surround-Zuspielung: alles, was über den Master-Mix hinausgeht. SoundOut bleibt der Master-Ausgang, an dem der Master-Pegel und der Mix-Recorder hängen; AudioOut nimmt ein Signal und gibt es an einen Kanal weiter. Ältere Patches setzen es als AuxOut1 bis AuxOut8 ein; diese werden als dieses Organism geladen, mit Channel ab 3 aufwärts voreingestellt. Ein Kanal, den das Gerät nicht hat, schluckt das Signal lautlos. Schalte einen Gain oder einen Send davor, um den Pegel einzustellen.

## Parameter

**Channel** Welchen Ausgang des Interfaces dieses Organism speist. Das Dropdown listet die Kanalnamen, die das geöffnete Gerät selbst vergibt.

## Rezept

**Kopfhörermix** Setze einen Send auf den Part, den der Performer hören muss, führe sein Send-Paar über einen Gain in ein AudioOut und stelle Channel auf den Ausgang, an dem der Kopfhörerverstärker hängt. Der Hauptmix auf SoundOut bleibt unberührt, und den Pegel des Kopfhörermixes regelst du mit dem Gain.

## Verwandte Organisms

AudioIn, SoundIn, SoundOut
