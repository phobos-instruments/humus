# PodIn

L'entrée d'un pod : une broche d'entrée sur la boîte du pod, transmise à une sortie à l'intérieur.

Tout ce qui est relié à la boîte du pod depuis l'extérieur passe par un PodIn et apparaît sur sa sortie à l'intérieur, un PodIn par broche, dans l'ordre de leur création. Ajoutes-en un depuis le menu du clic droit sur le canevas du pod ou depuis la catégorie Pod, et relie sa sortie à ce qui, dans le pod, doit l'entendre. L'éditeur du port est un simple menu déroulant qui le change de type sur place entre Mono, Stéréo, MIDI, Vidéo et Contrôle ; le port garde son nom, donc chaque câble est conservé, et la boîte du pod gagne une broche correspondante. Un port Contrôle est une prise sur la boîte du pod : un Number, un LFO ou un Follower qui y est relié depuis l'extérieur apparaît sur la sortie de contrôle du port à l'intérieur, prêt à être tiré vers n'importe quelle prise du pod. Un port Vidéo transporte des images plutôt que du son et les transmet sans changement.

## Recette

**Emballer une chaîne d'effets** Crée un pod, ajoute un PodIn Stéréo et un PodOut Stéréo, puis relie le PodIn à un Fern et le Fern au PodOut. Ajoute un PodIn Contrôle et tire sa sortie vers le Mix du Fern ; à l'extérieur, la boîte du pod affiche maintenant une entrée stéréo, une sortie stéréo et une prise de contrôle qu'un LFO peut piloter.

## Organisms associés

PodOut, Number, LFO
