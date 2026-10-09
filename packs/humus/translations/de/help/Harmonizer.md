# Harmonizer

Ein Harmonizer mit Tonhöhenerkennung, der einer einzelnen Melodielinie bis zu vier verschobene Kopien hinzufügt.

Er hört auf die Tonhöhe an seinem Eingang und baut Harmonien darum herum, jede davon dieselbe Performance, auf eine neue Note verschoben. Harmonien erklingen nur, solange eine klare Tonhöhe zu hören ist, sodass Atmer und Konsonanten sauber durchgehen. Im MIDI-Modus legen die am MIDI-Eingang gehaltenen Noten die Ziele fest: Sing eine Note, und der Akkord folgt deiner Hand. Im SCALE-Modus legen Key, Scale und die vier Voice-Intervalle sie fest, sodass die Harmonien in der Tonart bleiben, wohin die Melodie auch geht. Verbinde ein SoundIn oder einen FilePlayer damit und für den MIDI-Modus eine PianoRoll oder ein Keyboard mit dem MIDI-Eingang.

## Parameter

**Mode** MIDI nimmt die Ziele aus den am MIDI-Eingang gehaltenen Noten, bis zu vier, mit vollem Pegel. SCALE leitet sie aus Key, Scale und den Voice-Intervallen ab.

**Key** Der Grundton für den SCALE-Modus, C bis B.

**Scale** Die Tonleiter, in der die Intervalle zählen, von Chromatic und Major bis Blues, Whole Tone und Hirajoshi.

**Voice1** Das Intervall der ersten Stimme in Tonleiterstufen, -7 bis 7: 2 ist eine Terz höher, -3 eine Quarte tiefer, 7 die Oktave. Dasselbe für Voice2 bis Voice4.

**Level1** Der Pegel der ersten Stimme im SCALE-Modus; eine Stimme auf null pausiert. Dasselbe für Level2 bis Level4.

**Glide** Wie lange eine Stimme braucht, um ein neues Ziel zu erreichen, in Millisekunden. 0 springt.

**Humanize** Langsames Verstimmungsdriften pro Stimme, bis etwa 14 Cent, damit der Stapel nicht mehr wie eine einzige Maschine klingt.

**Spread** Fächert die Stimmen um die Originallinie herum über das Stereofeld auf.

**Mix** Überblendung von der Originallinie allein bis zu den Harmonien allein. Nur Effektsignal ist überall still, wo keine Harmonie erklingt.

## Rezept

**Diatonische Terzen** Mode SCALE, Key auf die Tonart des Songs, Scale Major, Voice1 2 mit Level1 0.8, Voice3 -3 mit Level3 0.5, Glide 40, Humanize 0.2, Spread 0.6, Mix 0.5. Schick einen Gesang von einem SoundIn hinein und setz einen Compressor dahinter, um den Stapel zusammenzuhalten.

## Verwandte Organisms

Cluster, Prism, Trellis, Decomposer
