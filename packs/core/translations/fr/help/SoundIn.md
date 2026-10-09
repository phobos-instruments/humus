# SoundIn

Un flux stéréo en direct depuis l'interface audio, ou un fichier son joué à sa place.

Dépose-en un, choisis la paire d'entrées, et un micro, un instrument ou un signal ligne entre directement dans le patch ; câble ses sorties vers un Gain, un Fern ou le Mixer. Le menu déroulant Input liste les noms d'entrées propres à l'interface, et le vumètre montre ce qui arrive avant Gain. Quand Live input est désactivé, il joue le fichier son à la place, et c'est aussi ce qui sert quand un patch est rendu hors ligne : sans entrée en direct disponible, c'est le fichier qui sert. Une interface mono remplit les deux sorties avec son entrée unique. Dépose plusieurs SoundIn réglés sur des paires différentes pour traiter séparément chaque paire d'une interface multicanal. Les anciens patchs avec des entrées auxiliaires numérotées se chargent toujours, chacune portant son canal comme paramètre.

## Paramètres

**Channel** La paire d'entrées du périphérique que lit ce SoundIn, affichée comme Input ; 1/2 est la paire principale. Un changement s'applique en direct.

**UseADC** Live input. Activé, il transmet l'entrée de l'interface ; désactivé, il joue à la place le File ci-dessous.

**File** Le fichier son utilisé quand Live input est désactivé. Il se charge dès qu'il est défini, et un fichier mono remplit les deux sorties.

**Loop** Répète le fichier quand il arrive à la fin. Désactivé, le fichier joue une fois, puis les sorties se taisent.

**Gain** Un ajustement propre du niveau d'entrée, au gain unitaire par défaut. Baisse-le pour dompter une interface trop chaude, monte-le pour relever une source faible ; le vumètre lit le niveau avant lui.

## Recette

**Le micro dans le patch** Règle Input sur la paire où se trouve ton micro, laisse Live input activé, et câble les sorties vers un Gain puis vers un Fern avant le Mixer. Règle Gain pour que le vumètre reste bien en dessous du maximum sur ta phrase la plus forte, puis charge un File et désactive Live input pour continuer à travailler sur le patch sans le micro.

## Organisms associés

SoundOut, FilePlayer
