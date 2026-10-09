# MidiOut

Envoie le MIDI qui arrive sur son entrée vers un appareil matériel ou une autre application.

Câble-lui un PianoRoll, un Microdot, un Arpeggiator ou un MidiIn, et les notes et les contrôleurs sortent par le port que tu choisis. Le patch mémorise le numéro du port plutôt que le nom du périphérique, si bien qu'il s'ouvre sur une autre machine avec le périphérique assigné à ce port là-bas. Les paramètres de message font aussi de l'organism une source MIDI à part entière : changer Value envoie le Controller choisi sur le Channel choisi, et basculer Gate envoie un note-on puis un note-off, si bien qu'un couloir d'automation ou un bouton rotatif assigné peut piloter du matériel externe sans aucun câble MIDI. Associe-le à un MidiMonitor pour voir ce qui sort.

## Paramètres

**Port** Lequel des huit ports de sortie MIDI transporte les messages. Assigne un périphérique à chaque port dans les réglages MIDI ; le menu déroulant affiche chaque port avec son périphérique.

**Channel** Le canal MIDI, de 1 à 16, sur lequel partent les messages Controller et Gate. Les notes qui arrivent sur l'entrée gardent leur propre canal.

**Controller** Le numéro de contrôleur que Value envoie. Le changer envoie tout de suite la Value actuelle sur le nouveau numéro.

**Value** La valeur du contrôleur, de 0 à 127. Chaque changement envoie un message de contrôleur, alors automatise-la ou assigne-la à un bouton rotatif pour piloter un paramètre matériel depuis le patch.

**Note** Le numéro de note que Gate joue, 60 par défaut.

**Velocity** La vélocité du note-on que Gate envoie.

**Gate** Activé, il envoie un note-on pour Note à Velocity ; désactivé, il envoie le note-off correspondant. Assigne-le à un bouton ou à un pad.

## Recette

**Filtre matériel piloté** Règle Port sur le port assigné à ton synthé, Channel sur le canal qu'il écoute et Controller sur le numéro de sa fréquence de coupure. Automatise Value sur la timeline, ou module-la avec un LFO, et le filtre du synthé suit le patch ; câble un PianoRoll sur l'entrée pour jouer les notes par le même port.

## Organisms associés

MidiIn, MidiMonitor
