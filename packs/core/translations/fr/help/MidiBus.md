# MidiBus

Une jonction de fusion MIDI sans commandes : plusieurs câbles MIDI en entrée, un seul flux trié dans le temps en sortie.

Pour répartir vers plusieurs destinations, pas besoin d'organism, puisqu'une sortie MIDI peut être reliée à autant d'instruments que tu veux ; le MidiBus sert à rassembler. Branche dessus les contrôleurs et les séquenceurs, et un câble par instrument en sortie : pour tout rediriger, il suffit alors de déplacer un seul câble. Les événements de toutes les entrées sont fusionnés dans l'ordre chronologique à l'intérieur de chaque bloc. Le menu déroulant Inputs de l'éditeur de propriétés règle son nombre d'entrées, de 2 à 8, et redimensionne le bus sur place en gardant son nom, ses câbles et son automation. Un Tuning branché sur un MidiBus s'applique à chaque instrument en aval, ce qui en fait l'endroit où accrocher une seule gamme pour toute une section.

## Recette

**Clavier plus séquenceur** Branche un MidiIn et un Microdot sur un MidiBus, et le bus sur un Rhizome. Les notes jouées et les notes séquencées arrivent à la même voix, et un Tuning sur une troisième entrée réaccorde les deux à la fois.

## Organisms associés

Bus, MidiIn, MidiOut, Tuning
