# PodOut

La sortie d'un pod : une broche de sortie sur la boîte du pod, alimentée par une entrée à l'intérieur.

Tout ce qui est relié par un câble à un PodOut dans le pod apparaît, à l'extérieur, sur la broche de sortie correspondante de la boîte du pod, un PodOut par broche, dans l'ordre où ils ont été créés. Ajoutes-en un depuis le menu du clic droit sur le canevas du pod ou depuis la catégorie Pod. L'éditeur du port est un simple menu déroulant qui le change de classe sur place entre Mono, Stéréo, MIDI, Vidéo et Contrôle ; le port garde son nom, donc chaque câble est conservé, et la boîte du pod gagne une broche correspondante. Un PodOut Contrôle est le miroir du PodIn Contrôle : tire n'importe quelle sortie de contrôle dans le pod jusqu'à sa prise et la boîte gagne une sortie de contrôle qui porte la valeur à l'extérieur, pour qu'un pod puisse être lui-même une source de contrôle. Un port Vidéo transporte des images plutôt que du son et les transmet sans les changer.

## Recette

**Le pod comme source de contrôle** Dans un pod, relie un PodIn Stéréo à un Follower par un câble et tire la sortie env du Follower jusqu'à un PodOut Contrôle. À l'extérieur, relie le kick au pod par un câble et tire la nouvelle sortie de contrôle du pod jusqu'à un Gain sur la basse : le pod fait baisser la basse selon ce qu'il écoute.

## Organisms associés

PodIn, Follower, Gain
