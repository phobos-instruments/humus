# Phono

Un preamplificatore phono: de-enfasi RIAA, guadagno per la testina e un filtro antirombo, per un giradischi collegato a un ingresso di linea.

I dischi vengono incisi con i bassi ridotti e gli acuti esaltati, e una testina eroga pochi millivolt, così un disco suonato direttamente in un ingresso di linea suona sottile e molto basso. Phono annulla la curva e ripristina il livello. Il filtro di de-enfasi è calcolato sulla risposta RIAA analogica alla frequenza di campionamento in uso, ottava più alta compresa. Collegalo subito dopo un SoundIn e prima di un Deck, di una Console o di un Mixer. Se il canale dell'interfaccia ha un selettore phono/line, mettilo su line, altrimenti la correzione viene applicata due volte. Il percorso del segnale è de-enfasi, poi filtro antirombo, poi guadagno.

## Parametri

**Curve** RIAA è la curva di riproduzione standard. RIAA + IEC aggiunge un leggero taglio a 20 Hz che alcuni stadi phono includono. Flat salta l'equalizzazione e tiene solo il guadagno, per riversamenti che equalizzerai dopo o per dischi precedenti allo standard.

**Gain** Guadagno della testina in dB. Una testina a magnete mobile, cioè ogni testina standard da DJ, vuole circa 40; una testina a bobina mobile a bassa uscita vuole circa 60. Se è troppo alto, va in clipping.

**Rumble** Un passa-alto a 18 Hz che toglie l'ondeggiamento dei dischi imbarcati e il rombo del giradischi prima che si mangino l'headroom. Attivo di default; disattivalo per le misurazioni.

## Ricetta

**Giradischi in ingresso** Collega SoundIn a Phono e Phono a un Deck o a un Mixer. Curve RIAA, Rumble attivo, Gain 40 per una testina a magnete mobile; alza Gain a passi di 5 finché un passaggio forte arriva appena sotto il fondo scala sui misuratori.

## Organism correlati

Console, Gain, Deck, SoundIn
