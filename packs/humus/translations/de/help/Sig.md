# Sig

Macht aus einem Steuerwert ein Audiosignal, mit einer Rampe, damit es nie klickt.

Kabel tragen Klang, und Zahlen landen auf Reglern; Sig ist das einzige Organism, das eine Zahl als Kabel wieder hinausschickt, für die wenigen Stellen, die eins brauchen. Meistens geht ein Follower, ein Sensor oder ein LFO über ein Steuerkabel direkt auf einen Regler, aber ein Number, das als Signal in einer Math-Formel stehen muss, ein Slider, der über ein AuxOut mit ausgeschaltetem DC-Schutz einen Steuerspannungsausgang treibt, oder eine Konstante, die zu einem Kabel addiert wird, brauchen alle Sig. Was auch immer auf seiner Value-Buchse landet, verlässt den Ausgang als gleichmäßiges Signal.

## Parameter

**Value** Die Zahl, die zum Signal werden soll. Verbinde irgendetwas damit oder tipp sie ein.

**Slew** Wie lange eine Änderung braucht, um anzukommen, in Millisekunden. Bei 0 landet eine Änderung innerhalb eines Samples; der Standard von 5 ms rundet Sprünge ab, ohne merkliche Verzögerung.

## Rezept

**Offset in Math** Verbinde einen Slider mit Value von Sig und den Ausgang von Sig mit dem b-Eingang von Math, mit dem Audio auf a. Schreib a+b in Math, und der Slider addiert einen DC-Offset zum Signal; stell Slew auf 200, damit das Bewegen des Faders gleitet statt springt.

## Verwandte Organisms

Number, Slider, Follower, Math
