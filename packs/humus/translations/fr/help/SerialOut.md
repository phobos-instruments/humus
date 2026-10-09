# SerialOut

Envoie des valeurs de contrôle par un port série, sous la forme de texte ou d'octets qu'attend l'appareil.

Relie des Sliders, un Follower, un LFO ou un Number à ses prises et écris dans le champ Format la ligne qu'ils doivent produire : %1 à %8 écrivent les prises sous forme de texte, %b1 à %b8 en envoient une sous forme d'octet brut et %w1 à %w8 sous forme de mot de deux octets, \n termine une ligne et \xNN est un octet brut. La valeur par défaut "%1 %2\n" écrit deux nombres et un saut de ligne. Scale multiplie une valeur avant qu'elle soit écrite et Int l'arrondit, si bien qu'un contrôle de 0 à 1 sort de 0 à 255. Plusieurs lignes dans Format font plusieurs messages, et une prise Message choisit celui qui part. L'afficheur en bas montre le port et les derniers octets envoyés. SerialIn relit les valeurs de la même façon.

## Paramètres

**Device** Le port série avec lequel dialoguer. Auto prend le premier port trouvé ; le reste de la liste, ce sont tous les ports présents.

**BaudRate** La vitesse du port, qui doit correspondre à celle qu'ouvre le firmware. 9600 et 115200 sont les choix habituels.

**Rate** Combien de fois par seconde la ligne part en mode Continuous, et le plafond dans les deux autres. Affiché comme Per second.

**Frame** Bits de données, parité et bits d'arrêt : 8N1 est ce qu'utilise presque tout le matériel ; 8E1, 8O1 et 8N2 sont là pour les appareils qui les demandent.

**Custom** N'importe quel autre débit en bauds, tapé à la main. 0 laisse la main à la liste BaudRate.

**Reset** Envoie une impulsion sur la ligne de reset de la carte à l'ouverture du port, pour que son programme reparte du début à chaque connexion. Off laisse la carte tourner pendant une reconnexion.

**Reconnect** Ferme le port et le rouvre, pour chaque organism qui le partage.

**Send** Continuous écrit la ligne Rate fois par seconde. On change l'écrit une fois chaque fois qu'une des prises affichées bouge. On trigger l'écrit quand la prise Trigger monte.

**Values** Le nombre de prises qu'affiche l'organism, et combien comptent pour On change. Affiché comme Sockets.

**Value1** La première prise, et le nombre qu'elle envoie quand rien n'y est relié. Et ainsi de suite de 2 à 8 ; %5 dans Format écrit Value5, que sa prise soit affichée ou non.

**Trigger** La prise que lit On trigger ; un Button dessus envoie le message une fois par appui.

**Message** Choisit quelle ligne d'un Format multiligne est envoyée, à partir de 1. Il apparaît dès que Format a plus d'une ligne.

**Scale1** Multiplie Value1 avant qu'elle soit écrite ou rangée dans un octet. Et ainsi de suite de 2 à 8.

**Int1** Écrit Value1 sous forme de nombre entier plutôt qu'avec des décimales. Et ainsi de suite de 2 à 8.

**Format** Le modèle du message. En plus des marqueurs ci-dessus, %n1 écrit le nom de la source reliée à une prise, %c un octet de comptage, %x et %X une somme de contrôle, et %(v1*180) une valeur calculée.

**Port** Un chemin de port tapé à la main, qui prend le pas sur le choix de Device tant qu'il est renseigné.

## Recette

**Un servo piloté par un LFO** Relie un LFO à Value1, règle Scale1 sur 180 et active Int1, Format "M %1\r", Send Continuous, Rate 30. La carte lit la ligne et fait bouger le servo avec le LFO ; ralentis le LFO à une mesure et le bras balaie en rythme avec le patch.

## Organisms associés

SerialIn, Number, Slider, Button
