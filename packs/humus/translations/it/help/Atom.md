# Atom

Quattro anelli di colpi distribuiti in modo uniforme che arpeggiano le note che tieni, o percorrono una scala quando non ne tieni nessuna.

Ogni anello risponde a una domanda: quando. Prende un numero di step e un numero di colpi e distribuisce i colpi lungo il cerchio nel modo più uniforme possibile, ed è la regola dietro moltissimi ritmi di danza del mondo: tre colpi su otto sono il tresillo, cinque su otto il cinquillo, sette su dodici la classica campana dell'Africa occidentale. Quale nota suona, lo decidono le tue mani. Collega una tastiera o un PianoRoll all'ingresso e ogni colpo suona la nota successiva dell'accordo che tieni; non tenere niente e gli anelli percorrono da soli la tonalità e la scala scelte. Le note che suoni vengono prima portate sulla scala, così niente cade fuori tonalità. Gira mentre il transport suona, e manda tutto da un'unica uscita MIDI.

Il pannello frontale mostra gli anelli dall'esterno verso l'interno, con l'anello 1 il più esterno. I punti accesi sono colpi, quelli vuoti sono pause, e il punto più luminoso è lo step che sta suonando ora.

I preset si dividono in tre famiglie. Rhythms sono i pattern di danza visti sopra. Elements trattano gli anelli come gusci elettronici: l'anello più interno è il primo guscio, ogni anello è lungo quanto il suo guscio può contenere (2, 8, 18 e 32 step, dall'interno verso l'esterno), i suoi colpi sono gli elettroni che contiene, e ogni guscio fa un giro a battuta, così gli atomi più pesanti si riempiono di strati più fitti e veloci. Planets fanno degli anelli delle orbite le cui lunghezze rispettano le proporzioni di quelle vere: otto giri dell'anello della Terra contro tredici di Venere, i dodici di Giove contro i ventinove di Saturno, Nettuno che fa tre giri mentre Plutone ne fa due, così gli anelli si allontanano e tornano a incontrarsi come fanno i pianeti.

## Parametri

**Steps** Quanti step ci sono lungo l'anello, da 1 a 32.

**Hits** Quanti di quegli step suonano. L'anello li distribuisce nel modo più uniforme possibile. 0 zittisce l'anello.

**Turn** Ruota l'intera figura lungo l'anello, uno step alla volta, così lo stesso ritmo parte da un punto diverso.

**Rate** Quanto dura uno step: un quarto, un ottavo, un sedicesimo o un trentaduesimo. Fit bar invece stende l'intero anello su una battuta, qualunque sia il suo numero di step, ed è questo che crea una vera poliritmia: un anello da 3 e uno da 4, entrambi su Fit bar, suonano tre contro quattro. Gli anelli su un valore di nota con numeri di step diversi vanno semplicemente avanti e si sfasano tra loro, e tornano a coincidere solo dopo molte battute.

**Fix** Fa suonare all'anello la nota impostata in Note a ogni colpo invece dell'arpeggio, per pilotare una batteria o una campana.

**Note** La nota che suona un anello con Fix attivo.

**Order** Il percorso che l'arpeggio segue tra le note: Up, Down, Up-down, Random, o As played, che mantiene l'ordine in cui sono scese le tue dita. Ogni anello tiene il proprio posto lungo il percorso.

**Octaves** Ripete le note su questo numero di ottave, verso l'alto.

**Key** La tonica della scala che percorrono gli anelli.

**Scale** La scala che percorrono gli anelli quando non tieni nessuna nota, e quella su cui vengono portate le note tenute. Chromatic lascia le note tenute come sono.

**Latch** Tiene l'accordo dopo che hai lasciato i tasti. Il prossimo accordo che suoni lo sostituisce.

**Gate** Per quanta parte di uno step dura ogni nota.

**Vel** La velocity di ogni nota.

**Swing** Lo swing proprio di questo organism, usato quando Follow è disattivato. Un anello su Fit bar non ha mai swing.

**Follow** Prende lo swing dal transport; disattivalo per impostarlo qui con Swing.

## Ricetta

**Un arpeggiatore** Collega una tastiera ad Atom e Atom a un synth. Lascia l'anello 1 a 5 colpi su 16, imposta Order su Up-down e Octaves a 2, premi Riproduci e tieni un accordo. Alza Hits per una linea più fitta, e usa Turn per spostare gli accenti.

**Tre contro quattro** Imposta l'anello 1 a 4 step e 4 colpi e l'anello 2 a 3 step e 3 colpi, entrambi su Fit bar con Fix attivo e due note diverse, e collega l'uscita a un Sampler. Aggiungi l'anello 3 come arpeggio sopra.

**Un pattern di campana** Un anello da 12 step e 7 colpi su Fit bar dà la lunga campana di gran parte della musica dell'Africa occidentale e centrale. Mettigli sotto un secondo anello da 12 con 4 colpi per il beat dei ballerini, e ruota la campana finché non cade al posto giusto.

## Organism correlati

Steps, Sequence, Riff, DNA, PianoRoll
