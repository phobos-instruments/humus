# Phaser

Un phaser à six étages dont les creux balaient l'espace entre deux fréquences.

Une chaîne de filtres passe-tout laisse le niveau intact et ne décale que la phase ; une fois ce signal remélangé au signal direct, les endroits où les deux ne concordent pas s'annulent en creux, et un balayage sinusoïdal déplace ces creux entre les extrémités de FrequencyRange. Comme rien n'est retardé dans le temps, les transitoires restent exactement là où ils étaient, donc il peut se poser sur tout un sous-mix sans l'étaler. Là où les creux d'un Flanger sont régulièrement espacés et métalliques, ceux d'un phaser sont peu nombreux et irréguliers : c'est le balayage plus subtil, plus vocal. Branche-le après un piano électrique, une nappe ou un bus de Mixer.

## Paramètres

**FrequencyRange** Les extrémités grave et aiguë du balayage. Une plage étroite donne une lente ondulation tonale ; une plage large, c'est le balayage complet de bas en haut.

**Rate** La vitesse à laquelle les creux voyagent entre les extrémités, en Hz.

**Feedback** Renvoie la sortie vers l'entrée et aiguise les creux en pics résonants.

**Depth** Quelle part du signal déphasé est mélangée au signal direct. Zéro, c'est le signal direct seul ; les creux les plus profonds sont vers le milieu, là où les deux sont à égalité.

## Recette

**Balayage lent sur une nappe** FrequencyRange 200 à 2000, Rate 0.15, Feedback 0.5, Depth 0.5. Fais entrer une nappe tenue de Rhizome et le balayage met environ sept secondes à traverser une fois ; pousse Feedback vers 0.8 pour un pic plus marqué.

## Organisms associés

Flanger, Chorus, Filter
