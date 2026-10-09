# Var

Une valeur de contrôle avec un nom, pour qu'un câble ou une ligne série puisse transporter le nombre et le mot ensemble.

Tape un nom, et Value part par la sortie comme n'importe quelle autre source de contrôle ; le nom permet au câble de dire ce que signifie le nombre. Un SerialOut avec un Var sur sa prise écrit le nom à côté de la valeur avec %n1, et un SerialIn qui lit une ligne comme "temp 23.5" transmet le nombre au Var appelé temp. Une valeur qui arrive par son nom apparaît dans l'afficheur et sort par la sortie ; taper une nouvelle Value prend le dessus jusqu'à l'arrivée de la suivante. Relie sa sortie à n'importe quel bouton rotatif ou à une prise de SerialOut. Relie-le à l'entrée Vars d'un Math et la formule le lit par son nom ; nommé x, y, z ou w, il pilote ce potentiomètre.

## Paramètres

**Value** La prise, et le nombre à garder quand rien n'arrive par son nom. Tire un LFO ou un Slider dessus et le Var le suit.

**Name** Le mot auquel répond ce Var, affiché comme $ sur la façade. C'est le mot qu'il écrit sur un SerialOut et le nom sous lequel une formule de Math le lit. Les lettres, les chiffres et les tirets bas voyagent le mieux.

## Recette

**Un capteur par son nom** Nomme-le temp, avec un SerialIn au Parse vide qui lit une carte écrivant "temp 23.5" une fois par seconde. Relie la sortie du Var à la Frequency d'un Filter, et la température de la pièce fait balayer la fréquence de coupure sans aucun câble entre le SerialIn et le Var.

## Organisms associés

Math, Number, SerialIn, SerialOut, Slider
