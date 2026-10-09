# Slider

Ein Fader, dessen Position ein Steuerwert ist, dem jeder Regler im Patch folgen kann.

Der Ausgang führt die Position des Griffs als Steuerwert, sodass ein Regler ihr über ein Steuerkabel oder eine Steuern-mit-Route folgen kann, und der Eingang setzt den Griff aus einem anderen Steuersignal, sodass ein Sensor oder ein LFO ihn bewegen kann. Min und Max sind die Zahlen, für die die beiden Enden stehen, und der Ausgang meldet diesen Bereich, sodass ein Regler der Position über seinen eigenen Stellweg folgt, während ein Number am selben Ausgang den umgerechneten Wert liest. Weise Value per MIDI-Learn einem Hardware-Fader zu, und er wird zum Makro, das mehrere Routen auf einmal steuert. Gib einer Route eine Schalterform, und der Slider schaltet ab einer Schwelle einen Bypass um oder öffnet ein Gate.

## Parameter

**Value** Der Griff, von 0 bis 1. Der Eingang setzt ihn, der Ausgang sendet ihn.

**Min** Die Zahl, für die das untere Ende des Stellwegs steht. Min über Max kehrt den Stellweg um.

**Max** Die Zahl, für die das obere Ende des Stellwegs steht.

**Slew** Wie lange der Wert braucht, um eine neue Einstellung zu erreichen, in Millisekunden. Bei 0 kommt er sofort an; höhere Werte machen aus einem Sprung ein Gleiten.

**Log** Biegt den Weg zwischen Min und Max logarithmisch, sodass das untere Ende mehr Raum bekommt. Nimm es für Frequenzen und Zeiten; beide Enden müssen über null liegen.

## Rezept

**Filter-Makro** Min 80, Max 12000, Log an, Slew 60. Rechtsklicke bei einem Filter auf Cutoff und wähle Steuern mit Slider, dann rechtsklicke bei einem Fern auf Feedback und mach dasselbe mit einem engeren Bereich. Ein einziger Fader öffnet jetzt den Filter und verlängert zugleich das Echo; weise Value per MIDI-Learn einem Hardware-Fader zu.

## Verwandte Organisms

Number, Sig, LFO, Gate
