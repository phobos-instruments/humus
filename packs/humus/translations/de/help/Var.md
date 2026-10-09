# Var

Ein Steuerwert mit Namen, sodass ein Kabel oder eine serielle Leitung die Zahl und das Wort zusammen tragen kann.

Tipp einen Namen ein, und der Wert der Value-Buchse verlässt den Ausgang wie jede andere Steuerquelle; der Name lässt die Leitung sagen, was die Zahl bedeutet. Ein SerialOut mit einer Var auf seiner Buchse gibt mit %n1 den Namen neben dem Wert aus, und ein SerialIn, das eine Zeile wie "temp 23.5" liest, gibt die Zahl an die Var namens temp weiter. Ein Wert, der per Name ankommt, erscheint in der Anzeige und verlässt den Ausgang; tippst du einen neuen Value ein, gilt er, bis der nächste ankommt. Verbinde ihren Ausgang mit einem beliebigen Regler oder einer SerialOut-Buchse. Schick sie in den Vars-Eingang eines Maths, und die Formel liest sie unter ihrem Namen; heißt sie x, y, z oder w, steuert sie dieses Poti.

## Parameter

**Value** Die Buchse und die Zahl, die gehalten wird, wenn nichts per Name ankommt. Zieh einen LFO oder einen Slider darauf, und die Var folgt.

**Name** Das Wort, auf das diese Var hört, auf der Frontplatte als $ angezeigt. Es ist das Wort, das sie an einem SerialOut ausgibt, und der Name, unter dem eine Math-Formel sie liest. Buchstaben, Ziffern und Unterstriche kommen am besten durch.

## Rezept

**Sensor per Name** Nenn sie temp, dazu ein SerialIn mit leerem Parse, das ein Board liest, das einmal pro Sekunde "temp 23.5" ausgibt. Verbinde den Ausgang der Var mit Frequency eines Filters, und die Raumtemperatur fährt den Cutoff, ohne ein Kabel zwischen SerialIn und Var.

## Verwandte Organisms

Math, Number, SerialIn, SerialOut, Slider
