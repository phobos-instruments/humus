# Paulstretch

Eine extreme Zeitdehnung, die eine Audiodatei in eine langsame, phasenlose Wolke verwandelt.

Es folgt dem gemeinfreien Algorithmus [Paulstretch](https://github.com/paulnasca/paulstretch_python) von Nasca Octavian Paul. Für Tempoarbeit ist es nicht gedacht: Unter etwa vierfacher Dehnung verschmiert es, und genau dieses Verschmieren ist das Instrument. Die Abspielposition kriecht mit 1/Stretch der normalen Geschwindigkeit durch die Datei; jeder Schritt wird gefenstert, sein Spektrum bleibt erhalten, und seine Phasen werden neu gewürfelt, sodass die Harmonie bleibt und der Moment vergeht. Die beiden Kanäle würfeln ihre Phasen unabhängig voneinander, sodass eine Mono-Datei breit herauskommt. Füttere es mit einem Akkord, einer Stimme oder einem ganzen Titel und verbinde den Ausgang für einen Klangteppich mit einem Fern oder einem Verbatim, oder direkt mit einem Mixer. Die Transportleiste unter der Datei springt durch die Quelle, und jede Taste darauf ist ein Parameter, also bietet ein Rechtsklick auf jede davon MIDI-Learn, OSC-Learn, Steuern mit oder Folgen. Stopp im Transport lässt es weiterspielen; drück Stopp ein zweites Mal, wenn nichts mehr läuft, und es stoppt mit allem anderen.

## Parameter

**File** Die Quell-Audiodatei.

**Stretch** Wie viel langsamer als Echtzeit, von 1 bis 100. Acht ist die klassische Einstellung; bei fünfzig bewegt sich kaum noch etwas.

**Window** Das Analysefenster in Sekunden. Kurz bewahrt etwas Rhythmus und Gluckern, lang löst alles in ein Pad auf.

**Loop** Lässt die Quelle umlaufen, sodass die Wolke nie endet. Aus spielt einmal durch und geht in Stille über.

**Active** Schaltet die Wiedergabe ein. Aus ist still.

**Level** Ausgangs-Gain.

## Rezept

**Pad aus einem Akkord** Lade einen zwei Sekunden langen Akkord, Stretch 30, Window 0.5, Loop an. Verbinde den Ausgang mit einem Verbatim mit langer Hallfahne und nimm Level zurück, bis es unter dem Rest des Patches sitzt.

## Verwandte Organisms

SpectralFreeze, FilePlayer, Bloom
