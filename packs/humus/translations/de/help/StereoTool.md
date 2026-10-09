# StereoTool

Ein Mid/Side-Werkzeug fürs Stereobild, mit Reglern für Breite, Rotation, Mono-Bass und Phase.

Mid ist, was beide Kanäle gemeinsam haben, und Side ist, was sie nicht teilen; teilst du ein Signal in diese beiden auf, können Width und MonoBass das Bild selbst verändern, während die anderen Bedienelemente es verschieben oder im Pegel ausgleichen. Die Reihenfolge ist fest: zuerst PhaseL, PhaseR und Swap, dann die Mid/Side-Stufe, in der Width und MonoBass auf das Side-Signal wirken, dann Rotation, Balance, Mono und Output. Das Scope im Editor zeichnet das Stereofeld und seine Korrelation, sodass du schon siehst, dass eine Quelle in Mono verschwinden wird, bevor du es hörst. Setz es vor eine Console oder ein SoundOut, oder in einen Send-Return.

## Parameter

**Width** Side-Pegel. 0% fällt auf Mono zusammen, 100% lässt das Bild unverändert, und es reicht bis 400%. Über 100% dünnt es beim Verbreitern die Mitte aus, also prüf Mono, während du es einstellst.

**Balance** Pegelbalance zwischen links und rechts, der übliche Panorama-Regler.

**Rotation** Dreht das ganze Stereofeld um bis zu 45 Grad in jede Richtung. Anders als Balance bewegt es das Bild, statt es im Pegel auszugleichen, sodass die Mitte mit allem anderen mitwandert.

**MonoBass** Unterhalb dieser Frequenz wird das Side-Signal entfernt, und der Bassbereich fällt auf Mono zusammen. 0 ist aus; 100 bis 150 Hz ist die übliche Einstellung.

**PhaseL** Invertiert den linken Kanal. Dreh die Phase einer Seite um, wenn eine Stereoquelle hohl klingt und in Mono verschwindet.

**PhaseR** Invertiert den rechten Kanal.

**Swap** Vertauscht links und rechts.

**Mono** Summiert den Ausgang zu Mono. Prüf damit, was Width über 100% anrichtet.

**Output** Ausgangs-Gain, ganz zuletzt angewendet.

## Rezept

**Monotauglich verbreitern** MonoBass 120, Width 140%, Balance und Rotation auf 0. Schalte beim Hören Mono ein und aus; wird die Mitte in Mono zu dünn, nimm Width Richtung 120% zurück. Stell Width nach MonoBass ein, nicht davor, sonst beurteilst du eine Breite, die der Mono-Bass gleich wieder zurücknimmt.

## Verwandte Organisms

Console, Mixer, Gain, SoundSpace
