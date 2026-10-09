# Follower

Ein Hüllkurvenfolger, der die Lautstärke eines Signals in einen Steuerwert und ein Gate verwandelt.

Er richtet das Audio an seinem Eingang gleich und glättet es zu einem Verlauf, der als Steuerwert "env" am ersten Ausgang erscheint, etwa 0 bis 1 bei Vollaussteuerung und skaliert mit Gain. Ein zweiter Ausgang und der Steuerwert "gate" stehen auf 0, bis der Verlauf Threshold überschreitet, und halten dann 1, bis der Pegel unter die Hälfte der Schwelle gefallen und Hold abgelaufen ist. Jedes Öffnen sendet außerdem Note am MIDI-Ausgang, mit einer Velocity aus dem Pegel des Schlags. Leg env auf einen Gain für Ducking oder einen Expander, auf die Frequency eines Filters für ein Auto-Wah, oder führ den MIDI-Ausgang in ein VideoPad, um Bilder im Beat zu schneiden. Für fertige Dynamik in einem einzigen Organism sieh dir SideChain und die Compressor-Familie an.

## Parameter

**Attack** Wie schnell der Verlauf in einen Schlag hinein ansteigt, in Millisekunden. Kurz fängt Transienten ein, lang glättet sie zu einem Pegel.

**Release** Wie langsam der Verlauf danach abfällt; das bestimmt, wie lange ein Klang das Gate offen hält, nachdem er aufgehört hat.

**Gain** Skaliert den Verlauf, damit sein Weg zu dem passt, was er steuert.

**Threshold** Der Pegel, den der Verlauf erreichen muss, um das Gate zu öffnen. 0 schaltet das Gate ab.

**Hold** Die Mindestzeit, die das Gate offen bleibt, nachdem ein Schlag es geöffnet hat, in Millisekunden, damit ein Trommelschlag nicht mehrere Noten auslöst.

**Note** Die MIDI-Note, die bei jedem Öffnen des Gates gesendet wird.

## Rezept

**Schnitte im Takt des Kicks** Schick einen Kick in den Follower, Attack 2, Release 150, Threshold 0.3, Hold 100. Verbinde seinen MIDI-Ausgang mit einem VideoPad, und jeder Kick löst das Pad aus; leg gleichzeitig env auf den Gain eines Pad-Synths, und der Synth duckt sich unter jedem Schlag weg.

## Verwandte Organisms

Gain, SideChain, Filter, Button
