# AudioIn

Ein einzelner Live-Eingangskanal des Audio-Interfaces, als Mono-Quelle.

Damit erreichst du jeden einzelnen Eingang eines mehrkanaligen Interfaces: ein AudioIn pro Mikrofon oder Line-Signal, das du für sich bearbeiten willst. SoundIn führt das Haupt-Stereopaar und kann auch eine Datei abspielen; AudioIn ist der schlichte Abgriff für einen einzelnen Kanal. Ältere Patches setzen es als AuxIn1 bis AuxIn8 ein; diese werden als dieses Organism geladen, mit Channel ab 3 aufwärts voreingestellt. Verbinde seinen Ausgang mit einem Gain, einem Mixer-Kanalzug oder direkt mit einem Effekt. Ein Kanal, den das Gerät nicht hat, liefert Stille.

## Parameter

**Channel** Welchen Eingang des Interfaces dieses Organism liest. Das Dropdown zeigt die Kanalnamen, die das geöffnete Gerät selbst vergibt.

## Rezept

**Zweites Mikrofon** Setz ein AudioIn ins Patch, stell Channel auf den Eingang, an dem das Mikrofon hängt, und verbinde es mit einem Mixer-Kanalzug neben dem SoundIn. Pass seinen Pegel mit dem Gain des Kanalzugs an, damit die beiden Quellen zusammenpassen, bevor ein Effekt sie hört.

## Verwandte Organisms

AudioOut, SoundIn, SoundOut
