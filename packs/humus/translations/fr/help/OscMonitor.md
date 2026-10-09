# OscMonitor

Un journal des messages OSC qui traversent l'app, dans les deux sens, sous forme de lignes lisibles.

Il n'a pas de broches : pose-en un n'importe où et il liste chaque message qui arrive sur le port d'entrée OSC, assigné à un paramètre ou non, et chaque valeur transmise par un organism qui envoie de l'OSC, comme Hands, avec l'adresse vers laquelle elle est partie. Sers-t'en pour vérifier qu'un contrôleur sur téléphone ou un pont de capteurs atteint bien la machine, pour trouver l'adresse qu'envoie un fader avant de l'assigner, et pour confirmer ce qu'un récepteur doit attendre. L'entrée OSC doit être activée dans Réglages pour que les lignes reçues apparaissent ; l'en-tête indique quand elle est désactivée. Pause fige la liste et Clear la vide.

## Paramètres

**Direction** Affiche les lignes All, Received ou Sent. Une main suivie envoie en continu, donc Sent peut inonder la vue ; les lignes filtrées sont jetées, pas gardées.

## Recette

**Trouver une adresse** Règle Direction sur Received, dirige le contrôleur vers le port OSC de la machine et bouge un fader. L'adresse qui apparaît est celle à donner à l'assignation OSC du paramètre.

## Organisms associés

Hands, MidiMonitor
