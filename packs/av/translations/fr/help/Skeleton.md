# Skeleton

Un tracker du corps entier qui transforme la posture et le mouvement en valeurs MIDI et de contrôle.

Place-toi devant la caméra : ta tête, tes mains, tes bras levés, ton inclinaison, ta position accroupie et ta posture deviennent des signaux lissés sur une seule sortie MIDI, sans audio. Branche-le dans un organism de synthé ou un MidiOut, ou fais un clic droit sur n'importe quel bouton rotatif et choisis Skeleton sous Contrôle pour le piloter sans câble. Douze valeurs passent sur les CC base +0 à +11 : tête X et Y, main gauche X et Y, main droite X et Y, bras gauche et bras droit levés, inclinaison, accroupi, posture et présence. La gauche et la droite sont les tiennes, quelle que soit la façon dont Mirror retourne l'image. Une seule personne est suivie à la fois, cadrée à partir des genoux ou des pieds, et une lecture dont les articulations sont cachées garde sa dernière valeur. L'entrée vidéo accepte un câble venant de CameraIn, de VideoPlayer ou de n'importe quel organism vidéo ; tant qu'elle est branchée, le tracker lit cette image et sa propre caméra reste fermée.

## Apprendre des poses

Appuie sur Learn sur un emplacement et tiens la pose, bras croisés ou un poing levé, pendant environ une seconde et demie ; la forme moyennée devient le modèle et est enregistrée dans le patch. Appuie sur Renf et reprends la pose depuis un autre endroit pour l'intégrer au modèle. Une pose, c'est la géométrie des bras, pas une position dans l'image, donc elle se déclenche où que tu te tiennes. La marque sur la barre de correspondance est le seuil de déclenchement, et la meilleure correspondance fait taire les autres.

## Paramètres

**Enabled** Ouvre la caméra. Rien n'est capturé tant qu'il est désactivé, et un câble vidéo branché garde la caméra fermée quoi qu'il arrive.

**Camera** Le périphérique de capture à ouvrir, quand il y en a plusieurs.

**Mirror** Retourne l'image pour qu'un mouvement vers la droite déplace la valeur vers la droite. Si l'organism source fait déjà le miroir, désactive l'un des deux.

**Confidence** Le degré de certitude dont le tracker a besoin avant d'accepter une personne. Monte-le si un meuble ou une affiche est sans cesse pris pour une personne, baisse-le quand la lumière est mauvaise.

**Smooth** Lissage de la réponse, en millisecondes. Plus c'est haut, plus c'est calme et lent.

**Tolerance** Avec quelle souplesse une pose en direct peut coller à un modèle et quand même correspondre.

**SendCC** Envoie les douze valeurs du corps en contrôleurs continus. Désactivé, il ne reste que les notes de pose.

**SendNotes** Envoie une note à chaque correspondance de pose. Le désactiver relâche toute note en cours.

**SendOSC** Publie aussi chaque valeur sous le nom OSC de l'organism.

**MidiCC** Le premier numéro de CC ; les douze valeurs comptent à partir de lui.

**MidiChannel** Le canal MIDI des CC et des notes.

**Gestures** Les modèles appris, stockés dans le patch. Learn et Renf l'écrivent ; il n'y a rien à taper.

**GNote_1** La note qu'envoie l'emplacement de pose 1, avec une vélocité donnée par la force de la correspondance, et ainsi de suite de 2 à 4.

**GThresh_1** La force de correspondance au-dessus de laquelle l'emplacement 1 se déclenche ; le relâchement se situe juste en dessous, pour qu'une pose hésitante ne papillote pas, et ainsi de suite de 2 à 4.

## Recette

**Montée au bras levé** Enabled activé, SendNotes désactivé, Smooth 200. Fais passer une source audio par un Gain, fais un clic droit sur son bouton rotatif Gain, choisis Skeleton sous Contrôle et prends raise/r. Lève le bras droit pour faire monter le son et baisse-le pour le faire disparaître.

## Organisms associés

Hands, CameraIn, MidiOut
