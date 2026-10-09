# Verbatim

Une réverb à convolution qui fait jouer ton signal à travers une réponse impulsionnelle enregistrée.

Dépose ou choisis un fichier d'impulsion dans la bande du haut et tout ce qui y est relié par un câble se passe à l'intérieur de cet enregistrement : une cage d'escalier, une réverb à ressorts ou une salle. N'importe quel fichier audio peut servir d'impulsion, jusqu'à quinze secondes ; les fichiers faits pour ça sonnent comme des pièces plutôt que comme des notes. L'impulsion est normalisée au chargement, donc des fichiers différents arrivent à un niveau comparable et Mix garde son sens. Le chemin du signal traité passe par Predelay, puis Damp, puis LowCut. Relie-y un Sampler, un Wave ou tout un bus de Mixer, ou mets-le sur un retour de Send.

## Paramètres

**File** La réponse impulsionnelle à travers laquelle jouer. N'importe quel fichier audio se charge ; les quinze premières secondes sont utilisées.

**Mix** Signal direct contre signal traité, à puissance constante. Tout à gauche, c'est le signal intact ; tout à droite, il n'y a plus que la pièce.

**Predelay** Combien de temps la pièce attend avant de répondre, jusqu'à un quart de seconde. Repousse l'espace derrière le son direct.

**LowCut** Coupe le bas du signal traité, là où les grandes pièces tournent à la bouillie. Au minimum, il ne fait que stabiliser l'extrême grave.

**Damp** Un passe-bas sur le chemin du signal traité. Baisse-le et la pièce s'assombrit, comme si les murs devenaient plus mous.

**Reverse** Joue l'impulsion à l'envers, pour que la pièce enfle jusqu'à la note au lieu de décroître après elle.

## Recette

**Salle en envoi** Sur un retour de Send, Mix tout à droite, Predelay 20, LowCut 150, Damp 6000. Charge une impulsion de salle, relie les voix et les nappes au Send par un câble, et garde la batterie à l'écart pour que le grave reste sec.

## Organisms associés

Fern, SoundSpace, Bloom
