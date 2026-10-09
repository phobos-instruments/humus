# AudioOut

Un canal de sortie physique de l'interface audio, utilisé comme destination mono.

Utilise-le pour un retour de scène, une écoute au casque, un clic pour un batteur ou une piste séparée d'un flux surround : tout ce qui dépasse le mix master. SoundOut reste la sortie master, avec le niveau master et l'enregistreur du mix qui lui sont attachés ; AudioOut prend un signal et le confie à un canal. Les anciens patchs le placent sous les noms AuxOut1 à AuxOut8, qui se chargent comme cet organism avec Channel préréglé à partir de 3. Un canal que le périphérique n'a pas avale le signal sans rien dire. Câble un Gain ou un Send devant lui pour régler le niveau.

## Paramètres

**Channel** La sortie de l'interface que cet organism alimente. La liste déroulante affiche les noms de canaux propres au périphérique ouvert.

## Recette

**Retour casque** Mets un Send sur la partie que l'interprète a besoin d'entendre, câble sa paire d'envoi à travers un Gain jusqu'à un AudioOut, et règle Channel sur la sortie où est branché l'ampli casque. Le mix principal sur SoundOut reste intact, et le niveau du retour se dose sur le Gain.

## Organisms associés

AudioIn, SoundIn, SoundOut
