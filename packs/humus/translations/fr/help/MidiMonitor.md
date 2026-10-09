# MidiMonitor

Un journal des événements MIDI en lignes lisibles : source, canal, type et données.

Il surveille deux flux à la fois. Sans câble, il note tout ce qui arrive de chaque périphérique MIDI activé, marqué de in1 à in8 selon le port, et du clavier à l'écran, marqué kbd, y compris le trafic de contrôleurs consommé par l'assignation des paramètres avant d'atteindre un synthé. Branché en ligne entre un MidiIn et un Rhizome, son entrée MIDI devient un point d'écoute limité à ce câble, marqué cord, qui montre exactement ce que reçoit ce synthé, et la sortie laisse passer les événements sans les modifier. Source et Channel filtrent à l'affichage, donc rien n'est perdu pendant que tu les changes. Sers-t'en pour trouver quel bouton rotatif matériel envoie quel contrôleur, ou pour vérifier qu'un synthé qui n'écoute que ses câbles reçoit bien ce que son câble transporte.

## Paramètres

**Source** All montre les deux flux. Cords only montre le point d'écoute ; Live only montre les périphériques et le clavier.

**Channel** Montre un seul canal MIDI, de 1 à 16. 0 les montre tous.

## Recette

**Trouver un bouton** Pose un MidiMonitor sans câble, Source Live only, Channel 0. Tourne le bouton rotatif matériel et lis le numéro de contrôleur sur la ligne la plus récente, puis fais un clic droit sur le paramètre auquel tu veux l'assigner et utilise l'apprentissage MIDI.

## Organisms associés

MidiIn, OscMonitor, MidiBus
