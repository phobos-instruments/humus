# PingPong

Un delay ping-pong stéréo : les échos alternent entre le canal gauche et le canal droit (chaque côté se régénère à partir de l'écho de l'autre côté).

## Paramètres

**Time ms** Le temps de delay en millisecondes (1.. 2000), quand Sync est désactivé.

**Feedback** Régénération de l'écho (0.. 0.95 - plafonnée sous l'unité pour la stabilité).

**Mix** Équilibre entre signal direct et signal traité.

**Sync** Cale le delay sur une croche au tempo du transport.

C'est l'exemple "concret" du SDK : il réutilise l'utilitaire DelayLine du SDK et se synchronise sur le tempo via le Transport. Source : packs/sample/organisms/PingPong/.
