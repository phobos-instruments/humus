# Lumen

Un compositeur de shaders qui peint l'audio qu'il entend et empile des calques vidéo en dessous.

Relie de l'audio à ses deux entrées et la scène se dessine au rythme de ce son ; relie sa sortie vidéo à un VideoOut pour la voir, comme un synthé a besoin d'un SoundOut pour être entendu. Scene prend un fichier de fragment shader et le recharge dès que le fichier change sur le disque ; vide, c'est la scène intégrée qui tourne. Les shaders lisent l'audio sous forme de niveau, de bandes, de temps, de tempo, d'une texture de forme d'onde et d'une texture de spectre, et les paramètres float propres à une scène suivent Knob1 à Knob4. Une scène peut aussi lire les calques vidéo situés sous elle et sa propre image précédente, et servir ainsi d'effet de réinjection : la scène hallucination fournie rêve par-dessus tout ce que tu y branches. Les quatre entrées vidéo prennent des calques VideoPlayer, VideoPad, CameraIn ou VideoTrack, empilés de bas en haut, avec la scène qui flotte par-dessus selon SceneOpacity et SceneBlend. L'opacité et le mode de fusion de chaque calque se règlent sur l'organism qui l'alimente.

## Paramètres

**Scene** Le fichier de shader à exécuter : .frag, .fs, .glsl ou .fsh. Vide, c'est la scène intégrée qui tourne, et les modifications du fichier se rechargent pendant la lecture.

**Brightness** Le niveau de sortie global de la scène.

**Speed** La vitesse de l'horloge propre à la scène. 0 la fige ; 2 la fait tourner deux fois plus vite.

**Knob1** Une valeur que la scène lit pour elle-même ; son effet dépend du shader. Un shader d'effet y prend son intensité.

**Knob2** La deuxième valeur de ce genre. Un shader d'effet y prend la vitesse et la couleur.

**Knob3** La troisième. Un shader d'effet y prend la fréquence, la densité et le nombre de cases de la grille.

**Knob4** La quatrième. Un shader d'effet y prend la largeur et le détail.

**SceneOpacity** À quel point la scène se voit par-dessus les calques vidéo situés en dessous.

**SceneBlend** Over pose la scène au-dessus des calques vidéo. Add l'additionne avec eux, si bien que les parties sombres de la scène disparaissent et que les parties claires brillent.

## Recette

**Surimpression réactive à l'audio** Relie le mix master aux deux entrées audio et un VideoPlayer à la première entrée vidéo. Laisse Scene vide, règle SceneBlend sur Add et SceneOpacity sur 0.5, puis envoie un Follower placé sur la grosse caisse vers Brightness pour que la scène clignote sur le temps. Relie la sortie à un VideoOut et ouvre-le depuis le menu Sorties vidéo.

## Organisms associés

VideoPlayer, VideoMix, VideoOut
