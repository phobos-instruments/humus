# SpectralFilter

Ein spektraler Klangformer, der die tonale Balance kippt und das Spektrum relativ zu seiner eigenen Hüllkurve schärft oder glättet.

Wo ein EQ eine feste Kurve anwendet, misst SpectralFilter das Eingangssignal Band für Band und skaliert jedes relativ zur lokalen spektralen Hüllkurve. Tilt ist eine Helligkeitsneigung mit nur einem Regler, und Contrast ist ein spektraler Kompressor: Unter null zieht er Spitzen an ihre Umgebung heran, über null hebt er sie an. Die Phase bleibt unberührt, daher ist es bei Tilt 0 und Contrast 0 transparent, und der Direktweg wird passend verzögert, damit jeder Mix-Wert phasenkohärent bleibt. Der Schalter Mode wählt Stereo oder Mono. Es erzeugt Latenz. Setz es hinter ein Substrate, einen Sampler oder einen ganzen Mixer.

## Parameter

**Tilt** Eine Gain-Neigung über das Band, bis etwa 18 dB in jede Richtung. Unter null wird es dunkler, indem die Tiefen angehoben und die Höhen abgesenkt werden; über null heller.

**Contrast** Wie weit jedes Band von der spektralen Hüllkurve weg oder zu ihr hin geschoben wird. Negativ glättet und verwäscht einen dichten Klang; positiv schärft Spitzen gegenüber Tälern für ein klareres, artikulierteres Ergebnis.

**Mix** Wie viel vom geformten Signal gegenüber dem Direktsignal zu hören ist.

## Rezept

**Präsenz ohne Anhebung** Tilt 0, Contrast 0.3, Mix 1 auf einem dumpfen Pad aus einem Substrate. Statt einer festen Frequenz werden die Bänder angehoben, die ohnehin Spitzen haben, sodass das Pad ohne neue Resonanz an Kontur gewinnt. Leg einen LFO auf Contrast für ein langsames spektrales Atmen.

## Verwandte Organisms

SpectralFreeze, SpectralMorph, Filter, Console
