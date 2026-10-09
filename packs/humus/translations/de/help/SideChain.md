# SideChain

Ein Kompressor, dessen Pegelreduktion von einem separaten Key-Eingang gesteuert wird.

Das Main-Signal an den Eingängen 1-2 wird vom Pegel des Keys an den Eingängen 3-4 heruntergeregelt: Verbinde den Kick mit dem Key und den Bass oder das Pad mit dem Main, und das Main duckt sich im Rhythmus des Kicks. Der Key speist nur den Detektor und geht nie durch, also verbinde den Kick zusätzlich direkt mit deinem Mixer. Das Auswahlmenü Mode schaltet an Ort und Stelle zwischen Stereo und Mono um; in Mono ist Eingang 1 Main und Eingang 2 Key. Liegt nichts am Key an, geht das Main unverändert durch. Für ein Ducking, das bei jedem Schlag gleich ist, egal wie der Kick klingt, nimm stattdessen SideKick.

## Parameter

**Detector** RMS mittelt den Key, sodass die Reduktion spät zupackt und langsam loslässt, der klassische pumpende Charakter. Peak folgt jedem Transienten genau.

**Threshold** Der Key-Pegel, ab dem die Reduktion einsetzt, in dB. Stell ihn so ein, dass der Kick ihn deutlich überschreitet.

**Ratio** Wie stark das Main heruntergeregelt wird, sobald der Key über dem Threshold liegt. 4 bis 8 ergibt ein deutliches Pumpen.

**Knee** Wie allmählich die Reduktion um den Threshold herum einsetzt, in dB.

**Attack** Wie schnell die Reduktion einsetzt, in Millisekunden. Bleib bei ein paar Millisekunden, damit der Transient des Kicks durchkommt.

**Hold** Wie lange die Reduktion auf voller Tiefe bleibt, nachdem der Key abfällt, in Millisekunden.

**Release** Wie schnell sich das Main erholt, in Millisekunden. Mit 150 bis 250 ms schwillt es vor dem nächsten Kick wieder an.

**Makeup** Gain, das nach der Kompression hinzukommt, in dB.

**Mix** Unter 1 mischt das geduckte Signal mit dem Direktsignal, für paralleles Ducking.

**Listen** Schickt den Key statt des Mains an die Ausgänge, damit du Threshold nach Gehör einstellen kannst. Schalte es danach wieder aus.

## Rezept

**Pumpender Bass** Verbinde den Kick mit den Eingängen 3-4 und die Bassline mit den Eingängen 1-2. Detector RMS, Threshold -20, Ratio 6, Attack 2, Release 180, Mix 1. Verbinde den Kick außerdem über ein eigenes Kabel mit dem Mixer, dann verschieb Release, bis der Bass genau vor dem nächsten Kick wieder hochkommt.

## Verwandte Organisms

SideKick, Kick, Microdot
