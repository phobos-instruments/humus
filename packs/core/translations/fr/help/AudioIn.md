# AudioIn

Un canal d'entrée en direct de l'interface audio, comme source mono.

Sers-t'en pour atteindre n'importe quelle entrée d'une interface multicanal : un AudioIn par micro ou par ligne que tu veux traiter à part. SoundIn transporte la paire stéréo principale et peut aussi lire un fichier ; AudioIn est la simple prise sur un seul canal. Les anciens patchs le placent sous les noms AuxIn1 à AuxIn8, qui se chargent comme cet organism avec Channel préréglé à partir de 3. Branche sa sortie dans un Gain, une voie de Mixer ou directement dans un effet. Un canal que le périphérique n'a pas ne joue que du silence.

## Paramètres

**Channel** L'entrée de l'interface que lit cet organism. Le menu déroulant liste les noms de canaux du périphérique ouvert.

## Recette

**Deuxième micro** Dépose un AudioIn, règle Channel sur l'entrée où se trouve le micro, et branche-le dans une voie de Mixer à côté du SoundIn. Ajuste-le avec le Gain de la voie pour que les deux sources s'équilibrent avant qu'un effet ne les entende.

## Organisms associés

AudioOut, SoundIn, SoundOut
