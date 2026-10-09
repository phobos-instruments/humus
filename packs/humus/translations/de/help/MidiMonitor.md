# MidiMonitor

Ein Protokoll der MIDI-Ereignisse als lesbare Zeilen: Quelle, Kanal, Typ und Daten.

Es beobachtet zwei Datenströme gleichzeitig. Ohne Kabel protokolliert es alles, was von jedem aktivierten MIDI-Gerät ankommt, nach Port mit in1 bis in8 markiert, und von der Bildschirmtastatur, mit kbd markiert, einschließlich des Controller-Verkehrs, der für die Parameterzuweisung verbraucht wird, bevor er einen Synth erreicht. Inline zwischen ein MidiIn und ein Rhizome gekabelt, ist sein MIDI-Eingang ein gezielter Abgriff, mit cord markiert, der genau zeigt, was dieser Synth empfängt, und der Ausgang gibt die Ereignisse unverändert weiter. Source und Channel filtern nur die Anzeige, sodass nichts verloren geht, während du sie änderst. Nimm es, um herauszufinden, welcher Hardware-Regler welchen Controller sendet, oder um zu prüfen, dass ein Synth, der nur auf Kabel hört, bekommt, was sein Kabel führt.

## Parameter

**Source** All zeigt beide Datenströme. Cords only zeigt nur den Abgriff; Live only zeigt die Geräte und die Tastatur.

**Channel** Zeigt einen MIDI-Kanal, 1 bis 16. 0 zeigt alle.

## Rezept

**Einen Regler finden** Leg ein MidiMonitor ohne Kabel an, Source Live only, Channel 0. Dreh den Hardware-Regler und lies die Controllernummer in der neuesten Zeile ab, dann klick mit rechts auf den Parameter, auf den du ihn legen willst, und nimm MIDI-Learn.

## Verwandte Organisms

MidiIn, OscMonitor, MidiBus
