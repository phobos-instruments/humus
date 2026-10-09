# Limiter

Ein Brickwall-Limiter mit sofortigem Attack: Nichts kommt über die Obergrenze.

Threshold ist der Punkt, an dem das Limiting beginnt, und Ceiling der Pegel, den der Ausgang erreichen darf, und der Ausgang wird so skaliert, dass ein Klang, der am Threshold ankommt, am Ceiling herauskommt. Threshold zu senken macht das Ergebnis also lauter und zugleich stärker begrenzt, und das ist der Regler, an dem du drehst; Ceiling ist die Sicherheitslinie, die du einmal setzt. Der Attack ist sofort, und eine harte Begrenzung am Ceiling sichert ihn ab, also liegt alles Ausdrucksstarke im Release. Die Erkennung ist über die Kanäle gekoppelt, sodass das Limiting das Stereobild nie kippt. Setz ihn ans Ende der Kette, hinter eine Console oder einen Mixer und vor das SoundOut.

## Parameter

**InputGain** Passt den eingehenden Klang vor dem Limiting an.

**Threshold** Wo das Limiting beginnt. Senk ihn für mehr Limiting und ein lauteres Ergebnis.

**Ceiling** Der höchste erlaubte Ausgangspegel. Lass etwas Headroom unter Vollaussteuerung für alles, was danach kommt.

**HoldTime** Wie lange die volle Absenkung gehalten wird, bevor das Release beginnt, in Millisekunden. Ein wenig verhindert Flattern bei schnell aufeinanderfolgenden Spitzen.

**ReleaseTime** Wie schnell sich der Limiter nach einer Spitze erholt, in Millisekunden. Kurz hält die Lautheit, kann aber bei dichtem Material pumpen; lang bleibt sauber und gibt etwas Pegel ab.

## Rezept

**Master-Obergrenze** Verbinde den Ausgang der Console mit dem Limiter und den Limiter mit dem SoundOut. Ceiling 0.95, ReleaseTime 150, HoldTime 10, dann senk Threshold, bis der lauteste Abschnitt ein paar dB Absenkung zeigt und nicht mehr.

## Verwandte Organisms

Compressor, Console, VuMeter
