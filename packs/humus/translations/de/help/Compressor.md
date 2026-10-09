# Compressor

Ein Feed-Forward-Kompressor mit Soft Knee, Hold-Stufe und tempo-synchronem Release.

Er misst den Eingang, bestimmt einen Gain-Wert und wendet ihn an, sodass laute Momente leiser werden und der Abstand zwischen Lautestem und Leisestem schrumpft. Die Erkennung ist über die Kanäle gekoppelt: Ein einziger Gain-Wert wird berechnet und auf jede Seite angewendet, sodass ein lauter Moment links das Stereobild nicht nach rechts ziehen kann. Stell zuerst Threshold ein, dann CompressionRatio, dann AttackTime und ReleaseTime und zuletzt MakeupGain. Bei einem Part mit Puls schalte Sync an, damit die Erholung auf einer Unterteilung des Schlags landet und dort bleibt, wenn sich das Tempo ändert. Es gibt keinen Key-Eingang; um einen Klang mit einem anderen zu komprimieren, nimm SideChain.

## Parameter

**InputGain** Trimmt den eingehenden Klang vor allem anderen. Hebst du ihn an, treibt das mehr vom Klang über die Schwelle, ohne die Schwelle zu verschieben.

**MakeupGain** Gain nach der Reduktion, um den ausgeglichenen Klang wieder anzuheben.

**AutoMakeupGain** Berechnet den Makeup-Gain aus Threshold und CompressionRatio, sodass eine Änderung an einem der beiden nicht auch den Pegel ändert. Praktisch beim Einstellen.

**AttackTime** Wie schnell er reagiert, sobald die Schwelle überschritten ist, in Millisekunden. Schnell fängt den Schlag ab; langsam lässt den Schlag durch und gleicht aus, was folgt.

**HoldTime** Wie lange die volle Reduktion gehalten wird, bevor das Release beginnt, in Millisekunden. Verhindert, dass der Kompressor bei Material mit kurzen Einbrüchen loslässt und gleich wieder zupackt.

**ReleaseTime** Wie schnell er aufhört, sobald der Klang wieder unter die Schwelle fällt, in Millisekunden. Wird ignoriert, solange Sync an ist.

**CompressionRatio** Wie fest der Klang gehalten wird. 2 heißt: Ein Klang 2 dB über der Schwelle landet 1 dB darüber. Niedrige Ratios gleichen aus, hohe machen platt.

**Threshold** Der Pegel, ab dem die Kompression einsetzt. Darüber wird der Klang heruntergedrückt, darunter passiert nichts.

**KneeWidth** Wie allmählich die Ratio einsetzt, in dB zu beiden Seiten der Schwelle. Ein weites Knie setzt sanft ein, statt umzuschalten.

**Sync** Richtet das Release am Transport aus statt an ReleaseTime. Attack bleibt, wo du es eingestellt hast.

**SyncUnit** Der Notenwert, in dem das synchronisierte Release zählt, etwa 1/8.

**SyncMultiplier** Wie viele dieser Einheiten ein Release ergeben.

## Rezept

**Drum-Bus-Glue** Threshold 0.4, CompressionRatio 3, KneeWidth 8, AttackTime 20, HoldTime 0, Sync an mit SyncUnit 1/8. Schick die Drum-Summe eines Mixers hinein, schalte AutoMakeupGain an, während du die Schwelle einstellst, und wieder aus, sobald der Pegel sitzt, wo du ihn haben willst.

## Verwandte Organisms

SideChain, Limiter, NoiseGate, TransientShaper
