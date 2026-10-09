# VideoTrack

Une piste vidéo de la timeline qui lit ses clips en suivant le transport et enregistre ce qui arrive sur son entrée.

Ajoute-en une depuis le menu d'ajout de piste de la timeline, ou dépose une vidéo sur l'arrangement et une piste apparaît pour elle. Les clips se coupent, se raccourcissent, se déplacent, bouclent et ont des fondus comme les clips audio, et la sortie montre ce qui se trouve sous la tête de lecture : arrête la lecture et elle garde cette image, saute ailleurs et elle y saute aussi, mets le morceau en boucle et elle boucle avec lui. Warp étire un clip au tempo du morceau d'après son tempo d'origine, Inverser le lit à l'envers, et les fondus sont des fondus au noir. Chaque clip porte une bande d'images tirées de son film, et un clic droit sur un clip ouvre Infos sur le média pour lire son codec, sa taille et sa fréquence d'images. Branche la sortie dans un VideoOut, un VideoFX ou un calque de Lumen ; deux pistes font deux calques.

## Enregistrement

Branche un CameraIn, une chaîne VideoFX, un VideoPad ou une autre piste dans l'entrée, arme la piste avec Record et lance l'enregistrement sur le transport : tout ce qui arrive sur l'entrée est écrit en vidéo dans le dossier des enregistrements tant que le transport tourne, et atterrit sur la timeline en clip au temps où il a commencé, un tour par passage de la boucle. Le son ne fait pas partie de l'image ; arme une piste audio à côté. La taille d'enregistrement se règle dans Réglages, sous Vidéo.

## Paramètres

**Record** Arme la piste. Tant qu'elle est armée, le prochain enregistrement du transport écrit une prise depuis l'entrée.

**Monitor** Ce que montre la sortie. In montre toujours l'entrée, Off montre toujours les clips, et Auto montre l'entrée quand la piste est armée ou que le transport est arrêté, et les clips quand il tourne. Il n'a de choix à faire que si quelque chose est branché dans l'entrée : sans rien de branché, ce sont les clips qui s'affichent, quel que soit ce réglage.

**Opacity** L'intensité avec laquelle l'image de la piste apparaît. Par-dessus des calques dans un Lumen, ou par-dessus une autre source qui partage la même entrée, elle fait un fondu entre eux ; sans rien en dessous, elle fait un fondu de l'image vers le noir. Dans tous les cas, elle s'applique exactement une fois, où qu'elle arrive.

**Blend** La façon dont elle se combine avec les calques en dessous : Normal, Add, Multiply ou Screen. Contrairement à Opacity, il lui faut quelque chose en dessous pour se combiner, donc elle n'a aucun effet sur une piste câblée directement dans un VideoOut ou dans un côté d'un VideoMix.

## Recette

**Capture live sur une boucle** Branche un CameraIn dans l'entrée de la piste et la sortie de la piste dans un VideoOut. Règle Monitor sur Auto, arme Record, mets quatre mesures en boucle et lance l'enregistrement : tu vois la caméra pendant l'enregistrement, et une fois que tu arrêtes, la prise est lue dans l'arrangement, coupée à la longueur de la boucle.

## Organisms associés

VideoPad, VideoPlayer, VideoOut, AudioTrack
