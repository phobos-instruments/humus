# SoundIn

Ein Live-Stereosignal vom Audio-Interface oder eine Audiodatei, die an seiner Stelle spielt.

Zieh eines ins Patch, wähl das Eingangspaar, und ein Mikrofon, ein Instrument oder ein Line-Signal fließt direkt ins Patch; verbinde seine Ausgänge mit einem Gain, einem Fern oder dem Mixer. Das Dropdown Input listet die eigenen Eingangsnamen des Interfaces, und die Pegelanzeige zeigt, was vor Gain ankommt. Ist Live input aus, spielt es stattdessen die Audiodatei, und so rendert ein Patch auch offline: Ohne verfügbaren Live-Eingang wird die Datei verwendet. Ein Mono-Interface füllt beide Ausgänge mit seinem einzigen Eingang. Setz mehrere SoundIns auf verschiedene Paare, um jedes Paar eines Mehrkanal-Interfaces getrennt zu behandeln. Ältere Patches mit nummerierten Hilfseingängen lassen sich weiterhin laden, jeder mit seinem Kanal als Parameter.

## Parameter

**Channel** Welches Eingangspaar des Geräts dieses SoundIn liest, angezeigt als Input; 1/2 ist das Hauptpaar. Eine Änderung wirkt sofort.

**UseADC** Live input. An führt den Eingang des Interfaces; Aus spielt stattdessen die Datei aus File.

**File** Die Audiodatei, die verwendet wird, solange Live input aus ist. Sie wird geladen, sobald sie gesetzt ist, und eine Mono-Datei füllt beide Ausgänge.

**Loop** Wiederholt die Datei, wenn sie das Ende erreicht. Aus spielt die Datei einmal, danach verstummen die Ausgänge.

**Gain** Ein sauberer Eingangs-Trim, standardmäßig Unity Gain. Dreh ihn runter, um ein zu heißes Interface zu zähmen, oder hoch, um eine leise Quelle anzuheben; die Pegelanzeige misst den Pegel davor.

## Rezept

**Mikrofon durchs Patch** Stell Input auf das Paar, an dem dein Mikrofon hängt, lass Live input an und verbinde die Ausgänge mit einem Gain und weiter mit einem Fern vor dem Mixer. Stell Gain so ein, dass die Pegelanzeige bei deiner lautesten Phrase deutlich unter dem Maximum bleibt, dann lade eine Datei in File und schalte Live input aus, um ohne Mikrofon am Patch weiterzuarbeiten.

## Verwandte Organisms

SoundOut, FilePlayer
