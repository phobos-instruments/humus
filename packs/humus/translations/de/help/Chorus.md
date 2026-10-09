# Chorus

Ein Stereo-Chorus, der einen Klang mit einer langsam wandernden, verzögerten Kopie andickt.

Eine Kopie des Eingangs wird um ein paar Millisekunden verzögert, und diese Verzögerung bleibt ständig in Bewegung, sodass die Kopie leicht über und unter dem Original schwebt, so wie zwei Musiker nie exakt dieselbe Tonhöhe treffen. Die Modulation ist ein Dreieck, und die des rechten Kanals läuft der des linken um RightLFOPhaseOffset hinterher; das öffnet das Stereobild. Rate und DelaySweepDepth wirken zusammen: Wie schnell sich die Verzögerung ändert, bestimmt, wie weit sich die Tonhöhe verbiegt, also verbiegt sie sich weiter, wenn du einen der beiden verdoppelst. Schalte den Chorus hinter ein Rhizome, eine Wave oder einen Sampler; ein Flanger ist dieselbe Idee mit kürzeren Verzögerungen, ein Phaser dieselbe Idee ganz ohne Verzögerung.

## Parameter

**MinDelay** Die kürzeste Verzögerung, die je erreicht wird, in Millisekunden. Kleine Werte driften in Richtung Flanging, größere bleiben klar Chorus.

**DelaySweepDepth** Wie weit die Verzögerung über MinDelay hinauswandert, in Millisekunden. Mehr Tiefe heißt mehr Verstimmung.

**Rate** Wie schnell die Verzögerung wandert, in Hz. Langsam ist ein sanftes Wogen, schnell ist Vibrato.

**RightLFOPhaseOffset** Wie weit die Modulation des rechten Kanals der des linken hinterherläuft, in Grad. Bei null verstimmen sich beide Seiten gemeinsam, und das Ergebnis bleibt in der Mitte; bei 90 oder 180 gehen sie auseinander, und das Bild öffnet sich.

**Feedback** Führt die verzögerte Kopie zurück zum Eingang. Kleine Mengen fügen Resonanz hinzu; ein negativer Wert invertiert auf dem Rückweg und höhlt den Klang aus, statt ihn anzudicken.

**HFRolloffFrequency** Dunkelt die verzögerte Kopie von oben ab, in Hz. Hältst du den Effektweg dumpfer als den Direktweg, klingt ein Chorus nicht gläsern.

**Stereoness** Reserviert für die Breite des Effektbilds. Der jetzige Chorus hält die Kopie jeder Seite auf ihrer eigenen Seite, daher hat dieser Regler noch keine hörbare Wirkung.

**WetDryMix** Wie viel vom verzögerten Klang gegenüber dem Original zu hören ist. Bei vollem Effektsignal hat die Verstimmung nichts Stabiles mehr, gegen das sie sich abheben kann.

## Rezept

**Breites Pad** MinDelay 8, DelaySweepDepth 6, Rate 0.3, RightLFOPhaseOffset 180, Feedback 0, HFRolloffFrequency 8000, WetDryMix 0.5. Verkable einen gehaltenen Rhizome-Akkord hinein, und die beiden Seiten driften um die Mitte auseinander.

## Verwandte Organisms

Flanger, Phaser, Fern, StereoTool
