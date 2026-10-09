# Crossover

Ein Frequenzteiler, der ein Signal in Bänder aufteilt, ein Stereo-Ausgangspaar pro Band, das tiefste zuerst.

Die Trennungen sind phasenangepasst, also verbinde jedes Band mit einem Eingang, und sie summieren sich linear zum Originalklang zurück. Alles zwischen Trennung und Zusammenführung betrifft nur ein Band: Ein Compressor auf jedem Band ist Multiband-Dynamik, ein Fern nur auf den Mitten ist ein Echo, aus dem der Bass herausbleibt, ein Follower am tiefen Ausgang macht den Kick zu einem Steuersignal, unberührt von den Hats. Ein neues Crossover hat drei Bänder; das Dropdown Bands in seinem Editor formt es an Ort und Stelle zwischen 2 und 5 um, behält Kabel und Trenneinstellungen bei, und Freq3 und Freq4 erscheinen, wenn Bänder hinzukommen. Bearbeite die Bänder und führ sie zusammen, aber misch das zusammengeführte Ergebnis nicht mit dem Direktsignal, aus dem es stammt: Eine Frequenzweiche dreht die Phase, deshalb löschen sich die beiden seltsam aus, wenn sie übereinanderliegen.

## Parameter

**Freq1** Die tiefste Trennfrequenz, in Hz.

**Freq2** Die nächsthöhere Trennfrequenz. Die Trennungen behalten ihre Reihenfolge, deshalb lässt sich eine Trennung nicht unter die vorherige ziehen.

**Slope** Wie steil die Naht zwischen den Bändern ist. 24 dB/oct ist der Standard, phasenangepasst und linear summierend. 12 dB/oct ist sanfter, für tonales Verschmelzen; in diesem Modus sind abwechselnde Bänder in der Polarität umgekehrt, damit es weiterhin linear summiert, was für sich allein unhörbar ist. 48 dB/oct ist fast eine Wand, sodass jedes Band für sich hart angefahren werden kann.

## Rezept

**Drei-Band-Drums** Freq1 120, Freq2 2000, Slope 24 dB/oct. Verbinde einen Drum-Mixer mit dem Eingang, einen Compressor an jedes Ausgangspaar, mit langsamerem Attack am tiefen Band, und alle drei Compressors mit dem nächsten Eingang, der sie wieder summiert.

## Verwandte Organisms

ParaEQ, Filter, StereoTool, Follower
