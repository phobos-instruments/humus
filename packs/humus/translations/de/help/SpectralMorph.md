# SpectralMorph

Mischt die Spektren zweier Eingänge, sodass eine Klangfarbe durch die andere hindurchgeht, statt über ihr zu spielen.

Pro Band wird der Pegel zwischen Quelle A und Quelle B interpoliert, und die Phase kommt von der Quelle, die dort mehr beiträgt; dann wird das Ergebnis resynthetisiert. Im Modus Stereo sind die Eingänge 1 und 2 A und 3 und 4 B; im Modus Mono ist Eingang 1 A und Eingang 2 B, prüfe also die Kabel, nachdem du Mode umgeschaltet hast. Ist B nicht verbunden, wird Morph zu einer Blende in die Stille. Gehaltene, obertonreiche Quellen morphen am besten: Verkable ein Substrate und eine Gesangsstimme aus einem Sampler als die beiden Quellen und fahre Morph mit einem LFO oder einem Metapad.

## Parameter

**Morph** 0 ist reines A, 1 ist reines B, und alles dazwischen ist eine spektrale Mischung der beiden.

**A** Halten schaltet sofort auf A, egal wo Morph steht; lass los, und die Mischung kommt zurück. Weise es einem Pad zu, für einen spektralen Cut.

**B** Halten schaltet sofort auf B. Hältst du beide, landest du in der Mitte.

## Rezept

**Stimme wird zum Pad** Verkable einen gesungenen Vokal aus SoundIn mit den Eingängen 1 und 2 und einen Substrate-Akkord mit den Eingängen 3 und 4. Verkable einen LFO mit einer Periode von acht Takten mit Morph, damit sich die Stimme in das Pad verwandelt und wieder zurück. Lass beide Quellen während des ganzen Sweeps weiterspielen.

## Verwandte Organisms

SpectralFilter, SpectralFreeze, SoundSpace, Mixer
