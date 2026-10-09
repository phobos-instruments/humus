# Number

Eine Zahlenbox, die einen Wert live anzeigt, ihn von einem Steuereingang übernimmt und über einen Steuerausgang weitergibt.

Kabel tragen Klang, und Zahlen landen auf Reglern; ein Number ist der Ort, an dem du eine Zahl auf ihrem Weg zu einem Regler beobachtest. Verbinde einen Follower, einen Slider, ein RNG oder ein anderes Number mit seinem Eingang, und die Box zeigt unskaliert, was ankommt; tippst du einen Wert ein, bleibt er, bis etwas Neues hereinkommt. Jeder Regler im Patch kann dem Ausgang folgen: Zieh ein Steuerkabel auf eine Buchse, oder rechtsklicke den Regler und wähle "Steuern mit". Weil Value ein gewöhnlicher Parameter ist, können auch Automationsspuren und MIDI-Steuerung ihn bewegen, und so wird Number zur Brücke zwischen ihnen und jeder Buchse im Patch.

## Parameter

**Value** Die Zahl, die die Box hält. Der Eingang setzt sie, Tippen setzt sie, und der Ausgang gibt sie weiter.

**Format** Die Form, in der die Zahl hinausgeht. Any sendet genau das, was die Box hält, Whole rundet auf eine ganze Zahl, und 8-bit, 8-bit signed, 16-bit und 16-bit signed runden die Zahl und begrenzen sie auf diese Breite, sodass die Box zeigt, was tatsächlich hinausgeht.

## Rezept

**Beobachtete Sidechain** Verbinde den Ausgang eines Followers mit dem Eingang eines Numbers und den Ausgang des Numbers mit der Gain-Buchse eines Gains auf dem Bass. Die Box zeigt die Hüllkurve, während sie sich bewegt, so siehst du, welchen Pegel ein Ducking erreicht, bevor du den Bereich der Route einstellst.

## Verwandte Organisms

Slider, Sig, Follower, Gain
