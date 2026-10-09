# TransientShaper

Formt Attack und Sustain eines Klangs unabhängig von seinem Pegel.

Ein schneller und ein langsamer Hüllkurvenfolger laufen auf dem Eingang, und ihre normalisierte Differenz sagt, ob das Signal gerade einschwingt oder ausklingt, egal wie laut es ist, sodass eine leise Ghost Note genauso stark geformt wird wie ein voller Schlag. Attack wirkt auf das Einschwingen und Sustain auf den Ausklang, jeweils bis zu 15 dB in beide Richtungen. Die Erkennung ist stereo-gekoppelt, sodass das Stereobild nicht wandert. Übersprechen und Grundrauschen werden mitgeformt, also setz bei einer verrauschten Quelle ein Gate davor. Schick ein Drums-Organism, einen Sampler-Loop oder einen Kick hinein.

## Parameter

**Attack** Formt das Einschwingen. Positiv übertreibt den Transienten für mehr Snap; negativ macht ihn weicher. Wenn es zu klicken anfängt, nimm hier zurück, statt zu OutputGain zu greifen.

**Sustain** Formt Körper und Ausklang. Positiv verlängert ihn und holt den Raum hoch; negativ verkürzt ihn für einen trockenen, straffen Klang ohne Gate.

**OutputGain** Gleicht den Pegel nach dem Formen aus.

## Rezept

**Straffere Drums** Attack 0.3, Sustain -0.4, OutputGain 1 auf einem Drum-Loop aus einem Sampler. Die Schläge bekommen mehr Snap, und der Raum zwischen ihnen fällt weg. Kleine Bewegungen bewirken hier viel, weil dem Effekt nie der Headroom ausgeht, wie es einem Kompressor passiert.

## Verwandte Organisms

SideChain, Console, Kick, Sampler
