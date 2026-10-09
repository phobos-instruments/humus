# Mineral

Un synthé de percussion frappée dont les partiels se placent sur un réseau géométrique, chacun valant le précédent multiplié par Ratio.

Cette série est le spectre inharmonique du verre frappé, des cloches et du métal de gamelan, et la forme du réseau fait le timbre. Quatre voix jouent depuis l'entrée MIDI : séquence-le depuis un PianoRoll ou un DNA, ou joue-le au clavier quand son éditeur a le focus. Le sigil à gauche de l'éditeur dessine le modèle en direct : un côté par facette, étiré par Ratio, doublé par Shimmer, qui clignote à chaque frappe. Branche la sortie dans un Verbatim ou un Fern pour l'espace dont une cloche a besoin.

## Paramètres

**Facets** Le nombre de partiels, de 1 à 12. Peu, c'est une lame frappée ; beaucoup, ça s'étoffe en cloche.

**Ratio** La constante du réseau. 2.0 donne des octaves, 1.62 l'inharmonicité classique d'une cloche ; plus bas, c'est serré et métallique.

**Geometry** Confie Ratio au nombre de facettes : la diagonale d'un polygone régulier divisée par son côté, donc cinq facettes donnent 1.618 et plus de facettes montent vers l'octave. Le bouton rotatif Ratio s'estompe quand Geometry est activé.

**Shine** La pente de niveau à travers les facettes, de terne à cristallin.

**Decay** Le temps de résonance en millisecondes. Les facettes hautes s'éteignent toujours plus vite.

**Strike** Le transitoire de contact, une courte rafale de bruit.

**Shimmer** Désaccorde chaque facette un peu différemment à gauche et à droite, pour que le son batte à travers le champ stéréo.

**Level** Niveau de sortie.

**BendRange** Jusqu'où va la molette de pitch bend à fond de course, en demi-tons. Deux est la valeur par défaut courante ; zéro ignore la molette.

## Recette

**Gamelan** Facets 6, Ratio 1.62, Shine 0.75, Decay 1200, Strike 0.5, Shimmer 0.4. Branche un DNA sur l'entrée MIDI pour une ligne pentatonique, et la sortie dans un Verbatim avec une longue queue.

## Organisms associés

DNA, Rhizome, PianoRoll
