# SpectralFreeze

Un vocodeur de phase qui fige et étale un spectre, tout en le gardant en mouvement.

Smear étire la durée pendant laquelle le niveau de chaque bande persiste d'une trame d'analyse à l'autre, et Freeze verrouille le spectre actuel et ignore l'entrée jusqu'à ce que tu le relâches. Les bandes retenues gardent l'avance de phase mesurée sur la source, donc un accord figé continue de battre et de dériver. Diffusion floute le spectre retenu sur les bandes voisines et ajoute une lente errance de la phase ; Shimmer y réinjecte une copie à l'octave supérieure. Smear 0 avec Freeze désactivé laisse passer le signal à l'identique, et le chemin direct est retardé pour rester aligné. Câble à l'entrée un Substrate, un Sampler ou une voix, et envoie la sortie vers un Verbatim ou un Fern.

## Paramètres

**Freeze** Capture le spectre qui joue à cet instant et le retient, en ignorant l'entrée jusqu'à ce que tu le désactives.

**Smear** La durée pendant laquelle le niveau de chaque bande persiste, d'environ une milliseconde à dix secondes. À 0, l'entrée passe sans changement ; plus haut, une traîne spectrale de plus en plus longue se forme. C'est le bouton rotatif à automatiser.

**Diffusion** Floute les niveaux retenus d'une bande à l'autre et ajoute une lente marche aléatoire de la phase, pour qu'un son figé s'épanouisse au lieu de rester immobile. Ajoute-la après avoir figé, sinon elle étale la source avant que tu la captures.

**Shimmer** Réinjecte une copie à l'octave supérieure du spectre retenu.

**Mix** La part du signal figé qu'on entend face au signal direct.

## Recette

**Drone tenu** Smear 0.7, Diffusion 0.3, Shimmer 0.2, Mix 1. Câble à l'entrée un Sampler ou une nappe tenue, joue un accord et active Freeze au moment que tu veux garder ; assigne Freeze à un Button ou à une note MIDI pour le capturer en rythme. Désactive Freeze pour laisser repasser l'entrée.

## Organisms associés

SpectralFilter, SpectralMorph, SoundSpace, Substrate
