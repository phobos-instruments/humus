# Comber

Eine Bank gestimmter Kammfilter, die aus allem, was hindurchgeht, Akkorde herausholt.

Ein Kammfilter ist ein Delay, so kurz und mit Feedback, dass es auf einer Tonhöhe schwingt, statt ein Echo zu bilden. Comber stellt bis zu acht davon nebeneinander, fünf zu Beginn, jedes mit eigener Tonhöhe, eigenem Decay und eigenem Pegel, sodass Drums, Rauschen oder eine Stimme auf den Noten nachklingen, auf die die Kämme gestimmt sind. Jeder Kamm zeigt seine Tonhöhe auf drei Arten: als Schieberegler, als Zahl in Hertz und als nächstliegenden Notennamen mit der Abweichung in Cent. Klick auf den Notennamen, um eine Note auf einer Klaviatur auszuwählen, oder dreh das Mausrad darüber, um sie in Halbtonschritten zu ändern. Die Kämme klingen nebeneinander und werden summiert, also kommt ein Akkord heraus. Eine Tonhöhenänderung gleitet, statt zu klicken, und alles läuft in Stereo.

## Parameter

**InputGain** Der Master-Pegel, angewendet, wenn der Klang hereinkommt. Senke ihn, wenn sich lange Decays aufzutürmen beginnen.

**Combs** Wie viele Kämme in Gebrauch sind, eins bis acht. Die Zeilen darüber hinaus sind ausgeblendet und stumm.

**Frequency_1** Tonhöhe von Kamm 1 in Hertz, 16 bis 12000, entsprechend für die anderen. Das Delay ist eine Periode dieser Frequenz.

**DecayTime_1** Wie lange Kamm 1 nachklingt, wenn der Eingang verstummt, 0.02 bis 10 Sekunden, entsprechend für die anderen. Kurz ist ein gestimmter Schlag, lang ein gehaltener Ton.

**Gain_1** Pegel von Kamm 1, entsprechend für die anderen: der Anteil des Kamms an der Summe, null macht ihn also stumm.

## Rezept

**Akkord aus einem Drum-Loop** Schließ einen Drumcomputer oder einen Loop an. Stimme die fünf Kämme auf 110, 165, 220, 277 und 330, setz jedes Decay auf 2, jedes Kamm-Gain auf 0.6 und InputGain auf 0.5. Verkürze die Decays Richtung 0.3 für einen gestimmten Schlag, oder dreh einen Kamm auf 8 für einen Ton, der über dem Rest hängt.

## Verwandte Organisms

Fern, Verbatim, ParaEQ
