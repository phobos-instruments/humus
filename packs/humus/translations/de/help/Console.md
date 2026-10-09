# Console

Ein analoges Summenpult: ein Mixer, dessen Kanäle sich eine Stromschiene und einen Ausgangsübertrager teilen und deshalb aufeinander reagieren.

Wo ein Mixer seine Eingänge addiert, bildet eine Console nach, was einem Pult seinen Zusammenhalt gibt. Jeder Kanal ist ein Stereopaar, nummeriert nach seinen Eingängen: 1-2, 3-4 und so weiter. Gain und Pan platzieren jeden Kanal auf dem Bus, Crosstalk lässt den Bus in sich selbst übersprechen, Sag zieht den ganzen Bus herunter, je nachdem, wie hart er arbeitet, und Drive sättigt den Übertrager. Weil Sag und Drive den summierten Bus auswerten, drückt und verbiegt ein lauter Kick auf einem Kanal jeden anderen Kanal; das ist der Zusammenhalt, und deshalb solltest du das Pult mit laufendem Mix abhören, nicht an einem einzelnen Schlag. Sie ist ein Bus, kein Kanalzug: Es gibt keinen EQ und keinen Kompressor, also setz vor dem Pult einen TransientShaper oder einen SideChain auf jede Quelle. Setz sie dort ein, wo sonst ein Mixer säße, vor dem SoundOut.

## Das Pult

Das Dropdown Inputs legt fest, wie viele Stereokanäle das Pult summiert, 2 bis 8, und wechselt die Klasse des Organisms an Ort und Stelle, sodass Name, Einstellungen, Kabel und Automation erhalten bleiben. Vergrößern fügt unten Kanäle hinzu; Verkleinern entfernt die höchsten samt ihren Kabeln. Ein Kabel in den linken Eingang eines Kanals speist beide Seiten, so sitzt eine Mono-Quelle in der Mitte und Pan platziert sie.

## Parameter

**Output** Master-Trim, als Letztes angewendet, nach jeder Färbung.

**Direct** Schaltet jede färbende Stufe ab, behält aber Summierung und Output. Ist es an, ist der Bus die schlichte Summe seiner Kanäle, für einen ehrlichen A/B-Vergleich.

**Drive** Wie hart der Übertrager angefahren wird. Seine Sättigung ist asymmetrisch, macht die Tiefen dicker und rundet Transienten ab; bei null ist die Stufe aus dem Signalweg.

**Crosstalk** Wie viel von den Höhen jeder Seite auf die Gegenseite überspricht. Bewusst subtil, mit einem Maximum um -30 dB; am deutlichsten hört man es bei Material, das links und rechts verschieden ist.

**Sag** Wie weit der summierte Bedarf die Stromschiene herunterzieht, die zurückfedert, wenn die Musik atmet. Niedrig und langsam ist Zusammenhalt; höher pumpt es mit dem lautesten Signal auf dem Pult.

**Flavor** Bestimmt den Charakter aller drei Kopplungen auf einmal. Clean ist transparente Summierung. British ist warm und klebrig mit einer trägen Schiene. American ist knackig mit einer schnelleren Schiene und härterer Kante. Modern ist nahezu linear: eine steife Schiene, wenig Übersprechen, fast kein Eisen.

**Gain_1-2** Pegel von Kanal 1 in den Bus. Ebenso für 3-4 bis 11-12.

**Pan_1-2** Balance von Kanal 1. In der Mitte gehen beide Seiten mit vollem Pegel durch; aus der Mitte heraus wird nur die gegenüberliegende Seite leiser. Ebenso für 3-4 bis 11-12.

**Mute_1-2** Schaltet Kanal 1 stumm, was auch die Schiene für die anderen entlastet. Ebenso für 3-4 bis 11-12.

**Solo_1-2** Schaltet jeden Kanal außer diesem stumm. Das eigene Mute eines Kanals gewinnt immer. Ebenso für 3-4 bis 11-12.

**Name_1-2** Der Klebestreifen über Kanal 1. Leer, bis etwas angeschlossen ist, dann zeigt er den Namen dieses Organisms. Doppelklick, um einen eigenen zu schreiben; leere ihn, um zum Namen des angeschlossenen zurückzukehren. Ebenso für 3-4 bis 11-12.

## Rezept

**Einen Mix zusammenkleben** Wähl zuerst Flavor, denn es verschiebt das Ziel für alles andere. Stell die Gains der Kanäle für die Balance ein, mit Drive und Sag niedrig, erhöh Sag, bis der Mix mit dem Kick atmet, und gib zuletzt Drive für Gewicht dazu. Pumpt es zu deutlich, senk lieber den lautesten Kanal als Sag, und gleich verlorenen Pegel an Output aus.

## Verwandte Organisms

Mixer, Bus, SideChain, TransientShaper
