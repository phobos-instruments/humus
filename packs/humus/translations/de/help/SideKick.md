# SideKick

Ein zum Transport synchronisierter Gain-Former, der das Signal entlang einer gezeichneten Kurve pumpen lässt, ganz ohne Key-Eingang.

Eine Gain-Kurve wird einmal pro synchronisiertem Intervall abgetastet, fest am Raster des Transports, sodass Phase 0 auf dem Schlag landet; das klassische Ducking taucht genau dort ab, wo der Kick sitzt, und ist in jedem Zyklus gleich. Es gibt keinen Key, keinen Threshold und keinen Detektor, nur die Form. SideKick arbeitet nur, solange der Transport spielt, und lässt das Signal im Stillstand unberührt durch. Gain und Phase sind auch Steuerwerte, also kannst du für jeden Regler unter Steuern mit den SideKick wählen, damit er im Gleichschritt pumpt. Setz ihn auf einen Bass, ein Pad oder einen ganzen Mixer-Bus. Für Ducking, das von echtem Audio gesteuert wird, nimm SideChain.

## Die Form

Klick auf eine der Kacheln unter der Kurve, um ein Ducking, einen Swell, ein Gate-Muster, einen Riser oder ein Dreieck zu laden, oder zieh auf der Kurve, um frei zu zeichnen. Die Pfeile unter der dB-Skala schieben die Form um ein Zweiunddreißigstel des Zyklus früher oder später, REV spielt sie rückwärts, und INV kehrt den Gain um, sodass aus Senken Spitzen werden.

## Parameter

**Sync** Die Zykluslänge: 1 bar, 1/2 bar, 1/4 bar, 1/8 bar oder 1/16 bar. 1/4 bar ist ein Schlag, das klassische Pumpen.

**Mix** Bei 1 ersetzt das geformte Signal das Direktsignal; niedrigere Werte mischen beide für paralleles Pumpen.

**Smooth** Rundet harte Kanten der Kurve ab, in Millisekunden. Erhöh den Wert, wenn eine Gate-Form klickt, senk ihn für knackigere Schnitte.

**Shape** Die gezeichnete Gain-Kurve, mit dem Patch gespeichert.

## Rezept

**Club-Ducking** Sync 1/4 bar, die erste Ducking-Kachel, Smooth 3, Mix 1, auf dem Bass. Klick mit rechts auf Gain eines Pads und wähl unter Steuern mit den Eintrag SideKick gain, damit das Pad auf derselben Kurve pumpt. Schieb die Form um einen Pfeilschritt früher, damit sie vor dem Kick pumpt.

## Verwandte Organisms

SideChain, Kick, Microdot
