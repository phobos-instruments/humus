# NoiseGate

Ein Gate, das Klang durchlässt, solange er laut genug ist, und schließt, wenn nicht, oder ihn andersherum absenkt.

Es entfernt Rauschen, Brummen und Übersprechen, die in den Lücken zwischen Phrasen hausen, strafft eine Trommel, indem es ihren Ausklang kürzt, und hält im Duck-Modus den Klang unten, solange er die Schwelle überschreitet. Das Bedienelement Threshold hat zwei Griffe, weil das Gate zwei Pegel hat: Der obere öffnet es, der untere schließt es, und hältst du sie auseinander, flattert das Gate nicht bei einem Signal, das um die Linie schwebt. Die Erkennung ist über die Kanäle gekoppelt, sodass es nie eine Seite aus dem Stereobild herausschneidet. Es gibt keinen Key-Eingang: Um einen Klang im Rhythmus eines anderen zu gaten, nimm SideChain. Setz es hinter ein SoundIn oder vor einen Compressor.

## Parameter

**Mode** Off lässt laute Klänge durch und senkt leise ab. Duck macht das Gegenteil und hält den Klang unten, solange er die Schwelle überschreitet.

**InputGain** Passt den eingehenden Klang vor der Erkennung an, sodass Material über die Schwelle gefahren werden kann, ohne sie zu verschieben.

**Threshold** Der Öffnungspegel am oberen Griff und der Schließpegel am unteren. Halte sie auseinander, damit es nicht flattert.

**Range** Wie weit der Klang abgesenkt wird, solange das Gate geschlossen ist. Ganz unten kommt nichts durch; ein Stück weiter oben bleiben die Lücken leise statt leer.

**AttackTime** Wie schnell das Gate öffnet, in Millisekunden. Schnell genug, um keine Note abzustumpfen, langsam genug, um nicht zu klicken.

**HoldTime** Wie lange das Gate offen bleibt, nachdem das Signal unter den Schließpegel gefallen ist, in Millisekunden. So behält eine Trommel ihren Körper, bevor das Gate schließt.

**ReleaseTime** Wie schnell das Gate schließt, sobald die Haltezeit abgelaufen ist, in Millisekunden. Kurz ist abrupt und rhythmisch, lang blendet den Ausklang aus.

## Rezept

**Knackige Snare** Schließ die Snare an. Threshold öffnen bei 0.3 und schließen bei 0.15, Range 0, AttackTime 0.5, HoldTime 40, ReleaseTime 60. Senk HoldTime, bis der Ausklang gekürzt ist, und erhöh Range ein wenig, wenn die Lücken leer klingen.

## Verwandte Organisms

SideChain, Compressor, SideKick, TransientShaper
