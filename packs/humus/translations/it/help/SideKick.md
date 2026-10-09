# SideKick

Un modellatore di guadagno sincronizzato col transport che fa pompare il segnale seguendo una curva disegnata, senza ingresso key.

Una curva di guadagno viene percorsa una volta per ogni intervallo sincronizzato, agganciata alla griglia del transport in modo che la fase 0 cada sul beat; il classico ducking scende esattamente dove c'è la cassa ed è identico a ogni ciclo. Non ci sono key, soglia o rilevatore, solo la forma. Funziona solo mentre il transport suona, e da fermo lascia passare il segnale intatto. Anche il suo guadagno e la sua fase sono valori di controllo, quindi qualsiasi manopola può scegliere Controlla con SideKick per pompare a tempo. Mettilo su un basso, un pad o un intero bus di un Mixer. Per un ducking pilotato da audio vero, usa SideChain.

## La forma

Clicca una delle miniature sotto la curva per caricare un ducking, un crescendo, un pattern di gate, un riser o un triangolo, oppure trascina sulla curva per disegnare a mano libera. Le frecce sotto la scala in dB spostano la forma prima o dopo di un trentaduesimo del ciclo, REV la suona al contrario e INV ribalta il guadagno, così le discese diventano picchi.

## Parametri

**Sync** La lunghezza del ciclo: 1 bar, 1/2 bar, 1/4 bar, 1/8 bar o 1/16 bar. 1/4 bar è un beat, il classico pompaggio.

**Mix** A 1 il segnale modellato sostituisce quello secco; più in basso li mescola per un pompaggio in parallelo.

**Smooth** Arrotonda gli spigoli della curva, in millisecondi. Alzalo se una forma di gate fa clic, abbassalo per tagli più netti.

**Shape** La curva di guadagno disegnata, salvata con la patch.

## Ricetta

**Ducking da club** Sync 1/4 bar, la prima miniatura di ducking, Smooth 3, Mix 1, sul basso. Fai clic destro sul Gain di un pad e scegli Controlla con SideKick gain per far pompare il pad sulla stessa curva. Sposta la forma in anticipo di un passo di freccia per pompare prima della cassa.

## Organism correlati

SideChain, Kick, Microdot
