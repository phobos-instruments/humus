# SoundOut

La sortie audio : tout ce qu'elle reçoit part vers l'interface.

Branche dedans le Mixer ou n'importe quel organism final et choisis la paire de sorties ; le menu déroulant Output liste les noms de sorties propres à l'interface, et le vumètre montre ce qui sort, après Gain. Le premier SoundOut d'un patch est le master, celui que montre le vumètre de la barre d'outils, et un nouveau patch commence avec un SoundOut. Ajoutes-en d'autres pour des envois vers des retours de scène, des écoutes au casque ou des stems surround, chacun dirigé vers sa propre paire ; là où deux visent le même canal, leurs signaux s'additionnent. Un SoundOut prend son nombre de canaux de ce qui y est branché, stéréo au minimum, donc une source à six canaux occupe six sorties consécutives du périphérique à partir du canal choisi. Les anciens patchs avec des sorties auxiliaires numérotées se chargent toujours, chacune portant son canal en paramètre.

## Paramètres

**Channel** La paire de sorties du périphérique que ce SoundOut alimente, affichée comme Output ; 1/2 est la paire principale. Le changement s'applique en direct.

**Gain** Un réglage transparent du niveau de sortie, à gain unitaire par défaut. Le vumètre lit après lui, donc il montre exactement ce qui part vers l'interface.

**DcGuard** Un passe-haut doux à 5 Hz en sortie, activé par défaut, pour qu'une valeur constante venant d'un Number ou d'un Follower devienne un petit clic puis du silence, au lieu d'une membrane de haut-parleur bloquée en place. Ne le désactive que sur une sortie qui alimente une interface couplée en DC pour des tensions de contrôle.

## Recette

**Écoute au casque** Garde le premier SoundOut sur 1/2 pour la salle et ajoute-en un second réglé sur 3/4, avec le casque sur les sorties 3 et 4 de l'interface. Branche dedans la paire d'envoi d'un Send, ou un second Mixer, et règle son Gain à part, pour que le niveau de l'écoute ne touche jamais le mix principal.

## Organisms associés

SoundIn, Mixer, FileRecorder
