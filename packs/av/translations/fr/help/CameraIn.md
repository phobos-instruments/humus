# CameraIn

Une caméra comme source vidéo.

Active Enabled et branche la sortie vidéo sur un calque de Lumen, un VideoMix, un VideoFX ou un VideoOut : l'image en direct devient un calque comme n'importe quelle cassette. La caméra ne s'ouvre que tant qu'Enabled est activé et se ferme quand il se désactive ; les images vont à l'affichage et nulle part ailleurs. C'est une image, pas un contrôleur : pour jouer de quelque chose avec une caméra, branche ceci sur Hands ou Skeleton, ou laisse-les ouvrir la caméra eux-mêmes.

## Paramètres

**Enabled** Ouvre la caméra. Désactivé, il ferme le périphérique au lieu de simplement masquer l'image.

**Camera** Quel périphérique de capture ouvrir, quand la machine en a plusieurs.

**Mirror** Retourne l'image de gauche à droite : quand tu te penches à gauche, tu bouges à gauche à l'écran. Active-le quand tu joues face à la caméra.

**Opacity** Avec quelle force ce calque apparaît par-dessus les calques du dessous dans un Lumen.

**Blend** Comment le calque se combine avec ceux du dessous : Normal, Add, Multiply ou Screen.

## Recette

**Caméra sous une scène** Enabled activé, Mirror activé, puis branche la sortie vidéo sur la première entrée vidéo de Lumen, et Lumen sur un VideoOut. Charge un shader de scène dans Lumen, règle SceneBlend sur Add et SceneOpacity sur environ 0.6, pour que la scène brille par-dessus l'image en direct.

## Organisms associés

Lumen, VideoMix, VideoOut, Hands
