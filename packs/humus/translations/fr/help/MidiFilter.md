# MidiFilter

Laisse passer les notes et les contrôleurs choisis, retient les autres, ou transforme l'un en l'autre.

Relie un clavier, un MidiIn ou un séquenceur à l'entrée, et la sortie à un instrument ou à un MidiOut. Il démarre avec une règle vide marquée d'un plus : choisis Note ou CC dedans pour créer une règle, et la suivante, vide, apparaît en dessous, jusqu'à huit. Choisis Off pour retirer une règle. Quand il y a plus de règles que le panneau n'en affiche, il défile. Clique sur une case de note et joue une touche sur ton clavier MIDI pour choisir cette note. Chaque règle écoute une note ou un contrôleur, ou une plage de notes ou de contrôleurs, et décide de ce qui se passe : Block l'arrête, To note le transmet comme une autre note, To CC le transmet comme un contrôleur. Les règles se lisent de haut en bas et la première qui correspond l'emporte, donc une règle étroite placée au-dessus d'une règle large crée une exception. Une plage se déplace d'un bloc : de C1 à B1 envoyé vers C4 joue toute l'octave trois octaves plus haut. Une note relâchée arrive toujours sur la note vers laquelle est partie sa frappe, même si tu changes une ligne pendant qu'elle est tenue.

## Paramètres

**In1** Ce qu'écoute la ligne 1 : Off, Note ou CC. Et ainsi de suite pour les lignes 2 à 8.

**Low1** Le numéro de note ou de contrôleur le plus bas auquel répond la ligne 1. Et ainsi de suite pour les lignes 2 à 8.

**High1** Le numéro de note ou de contrôleur le plus haut auquel répond la ligne 1. Règle-le égal à Low pour un seul. Et ainsi de suite pour les lignes 2 à 8.

**Do1** Ce que fait la ligne 1 de ce qui correspond. Block l'arrête. To note envoie une note, et un contrôleur devient une note qui joue quand sa valeur atteint la moitié et se relâche quand elle repasse en dessous. To CC envoie un contrôleur, et une note devient ce contrôleur, à sa vélocité quand elle est enfoncée et à zéro quand elle est relâchée. Et ainsi de suite pour les lignes 2 à 8.

**Target1** La note ou le contrôleur vers lequel la ligne 1 envoie sa correspondance la plus basse ; le reste de la plage suit dans l'ordre. Et ainsi de suite pour les lignes 2 à 8.

**Others** Ce qui arrive à tout ce qu'aucune ligne ne capte, y compris le pitch bend, la pression et les changements de programme. Pass le laisse passer. Block ne garde que ce qu'envoient les lignes.

## Recette

**Un pad qui balaie un filtre** Ligne 1 : In Note, Low et High C1, Do To CC, Target 74. Relie le contrôleur à pads à l'entrée et la sortie à un synthé dont la fréquence de coupure est assignée au CC 74 : plus tu frappes C1 fort, plus le filtre s'ouvre, et quand tu relâches, il se referme.

**Juste la batterie** Ligne 1 : In Note, Low C1, High B2, Do To note, Target C1. Others Block. Tout ce qui est au-dessus des deux octaves du bas, et chaque bouton rotatif, reste hors de la boîte à rythmes.

## Organisms associés

MidiIn, MidiOut, MidiMonitor, Cluster
