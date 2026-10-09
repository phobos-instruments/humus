# Hands

Un suivi de mains qui transforme les mouvements des doigts et de la main en MIDI et en valeurs de contrôle.

Montre une main à la caméra : chaque doigt, la position du poignet et un pincement deviennent des signaux lissés sur une seule sortie MIDI, sans audio. Branche-le sur un organism synthé ou un MidiOut, ou fais un clic droit sur n'importe quel bouton rotatif et choisis Hands sous Contrôle pour piloter un paramètre sans câble. Deux mains sont suivies dans des banques gauche et droite fixes : la banque gauche envoie les CC base +0 à +8 (ouverture du pouce, de l'index, du majeur, de l'annulaire et de l'auriculaire, X, Y, présence, pincement) et la banque droite +9 à +17. Quatre emplacements de geste envoient chacun une note quand la main en direct correspond à une pose apprise. L'entrée vidéo accepte un câble venant de CameraIn, de VideoPlayer ou de n'importe quel organism vidéo ; tant qu'un câble est branché, le suivi lit cette image et sa propre caméra reste fermée.

## Apprendre des gestes

Appuie sur Learn sur un emplacement et tiens la pose environ une seconde et demie ; la forme moyenne devient le modèle et elle est enregistrée dans le patch. Appuie sur Renf et montre à nouveau la même pose sous un autre angle pour l'intégrer au modèle. Le repère sur la barre de correspondance est le seuil de déclenchement de cet emplacement ; fais-le glisser vers la droite pour que seule une pose franche envoie la note. Quand plusieurs modèles correspondent, le meilleur fait taire les autres.

## Paramètres

**Enabled** Ouvre la caméra. Rien n'est capturé tant qu'il est désactivé, et un câble vidéo branché garde la caméra fermée quoi qu'il arrive.

**Camera** Quel périphérique de capture ouvrir, quand il y en a plusieurs.

**Mirror** Retourne l'image pour qu'un mouvement vers la droite déplace la valeur lue vers la droite. Si l'organism source fait déjà le miroir, désactive l'un des deux.

**BuiltIn** Utilise le suivi intégré au lieu de celui du système. Il est plus lent, et il tourne déjà partout où le système n'a pas de suivi à lui.

**TwoHands** Suit une deuxième main dans la banque droite. Désactivé, il suit une seule main.

**Confidence** À quel point le détecteur doit être sûr avant d'accepter une main. Monte-le si des lampes ou des visages créent des mains fantômes, baisse-le pour des gants ou une faible lumière.

**Smooth** Lissage de la réponse en millisecondes. Petit, c'est nerveux ; grand, c'est calme et plus lent.

**Tolerance** Avec quelle souplesse une pose en direct peut coller à un modèle et quand même correspondre. Les correspondances sont continues : elles peuvent monter progressivement en vélocité au lieu de basculer d'un coup.

**SendCC** Envoie les dix-huit valeurs de mouvement comme contrôleurs continus. Désactivé, il ne reste que les notes de geste.

**SendNotes** Envoie une note pour chaque correspondance de geste. Le désactiver relâche toute note en cours.

**SendOSC** Publie aussi chaque valeur sous le nom OSC de l'organism.

**MidiCC** Le premier numéro de CC ; les dix-huit valeurs se suivent à partir de lui.

**MidiChannel** Le canal MIDI des CC et des notes.

**Gestures** Les modèles appris, stockés dans le patch. Learn et Renf l'écrivent ; il n'y a rien à taper.

**GNote_1** La note qu'envoie l'emplacement de geste 1, avec une vélocité fixée par la force de la correspondance, et ainsi de suite pour 2 à 4.

**GThresh_1** La force de correspondance au-dessus de laquelle l'emplacement 1 se déclenche ; le relâchement se situe juste en dessous pour qu'une pose hésitante ne vacille pas, et ainsi de suite pour 2 à 4.

## Recette

**Filtre au pincement** Enabled activé, TwoHands désactivé, Smooth 150. Fais passer une source audio dans un Filter, fais un clic droit sur le bouton rotatif de fréquence de coupure du Filter, choisis Hands sous Contrôle puis pinch. Rapproche le pouce et l'index pour fermer le filtre, et ouvre la main pour laisser passer le son.

## Organisms associés

CameraIn, Skeleton, SoundSpace, MidiOut
