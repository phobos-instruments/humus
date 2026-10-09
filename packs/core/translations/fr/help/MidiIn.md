# MidiIn

Le MIDI en direct d'un appareil physique ou d'une autre application, livré sur une sortie MIDI.

Câble sa sortie vers un instrument comme Rhizome, vers un MidiBus, ou vers un MidiMonitor pour voir ce qui arrive. Le patch retient un numéro de port plutôt qu'un nom de périphérique, donc un patch déplacé sur une autre machine prend le périphérique que cette machine attribue au même port ; les périphériques sont attribués aux huit ports dans les réglages, et changer de port s'applique immédiatement. Le clavier à l'écran, en bas de l'éditeur de propriétés, joue des notes directement dans la sortie de cet organism, donc tu peux essayer les instruments derrière lui sans matériel. Il allume aussi les touches que tu tiens sur un clavier connecté, pour que tu voies ce qui arrive. Les anciens patchs utilisent MidiIn1 à MidiIn8, qui se chargent comme cet organism avec Port réglé en conséquence.

## Paramètres

**Port** Les ports d'entrée MIDI qu'écoute cet organism. All inputs, la valeur par défaut d'un nouveau MidiIn, entend tous les claviers et contrôleurs connectés à la fois ; un seul port n'entend que le périphérique qui lui est attribué, et le menu déroulant montre chaque port avec ce périphérique.

## Recette

**Du clavier au synthé** Pose un MidiIn, choisis le port attribué à ton clavier, câble-le vers un Rhizome et le Rhizome vers le Mixer. Clique d'abord sur les touches à l'écran pour entendre la voix, puis joue sur le matériel.

## Organisms associés

MidiOut, MidiMonitor
