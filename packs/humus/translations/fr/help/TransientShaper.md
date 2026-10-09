# TransientShaper

Façonne l'attaque et le maintien d'un son indépendamment de son niveau.

Deux suiveurs d'enveloppe, un rapide et un lent, tournent sur l'entrée, et leur différence normalisée indique si le signal est dans une attaque ou dans une queue, quel que soit son volume, donc une note fantôme discrète est façonnée autant qu'un coup plein. Attack agit sur l'attaque et Sustain sur la queue, chacun jusqu'à 15 dB dans un sens ou dans l'autre. La détection est liée en stéréo, donc l'image ne bouge pas. La repisse et le bruit de fond sont façonnés aussi, alors place un Gate avant lui sur une source bruyante. Fais-y entrer un organism Drums, une boucle de Sampler ou un Kick.

## Paramètres

**Attack** Façonne l'attaque. En positif, le transitoire est exagéré pour plus de claquant ; en négatif, il est adouci. Si ça commence à cliquer, recule ici plutôt que de te rabattre sur OutputGain.

**Sustain** Façonne le corps et la queue. En positif, il les allonge et fait ressortir l'ambiance de la pièce ; en négatif, il les raccourcit pour un son sec et serré, sans gate.

**OutputGain** Compense le niveau après le façonnage.

## Recette

**Batterie plus serrée** Attack 0.3, Sustain -0.4, OutputGain 1 sur une boucle de batterie dans un Sampler. Les coups gagnent en claquant et l'ambiance de la pièce entre eux s'efface. De petits mouvements font beaucoup ici, car l'effet n'est jamais à court de marge, contrairement à un compresseur.

## Organisms associés

SideChain, Console, Kick, Sampler
