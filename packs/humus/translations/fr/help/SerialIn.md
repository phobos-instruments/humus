# SerialIn

Lit des nombres sur un port série et les place sur les sorties de contrôle a à h.

Branche une carte à microcontrôleur qui écrit ses mesures sur sa liaison série, et chaque nombre d'une ligne arrive sur sa propre sortie : le premier sur a, le deuxième sur b, et ainsi de suite. Sépare les nombres par des espaces, des virgules ou des points-virgules, et mets un mot devant un nombre pour l'envoyer au Var de ce nom, sans câble. Un modèle Parse lit les lignes ou les paquets d'octets qui ne sont pas de simples nombres, et tant qu'il est défini, une neuvième sortie appelée match envoie une impulsion chaque fois qu'une ligne correspond. Le port est lu en dehors du chemin audio, un périphérique rebranché se reconnecte tout seul, et n'importe quel paramètre peut choisir Contrôler avec SerialIn dans son menu du clic droit. Câble les sorties sur des prises de Number, de Gain ou de LFO.

## Paramètres

**Device** Le port série à écouter. Auto prend le premier port trouvé ; le reste de la liste contient tous les ports présents.

**BaudRate** La vitesse du port, qui doit correspondre à celle qu'ouvre le firmware. 9600 et 115200 sont les choix habituels.

**Values** Le nombre de sorties, sur les huit, que l'organism affiche, à partir de a.

**Frame** Bits de données, parité et bits de stop : 8N1 est ce qu'utilise presque tout le matériel ; 8E1, 8O1 et 8N2 sont là pour celui qui les demande.

**Custom** N'importe quelle autre vitesse en bauds, saisie à la main. 0 laisse la liste BaudRate décider.

**Reset** Envoie une impulsion sur la ligne de reset de la carte à l'ouverture du port, pour que son programme reparte du début à chaque connexion. Désactivé, la carte continue de tourner pendant une reconnexion.

**Reconnect** Ferme le port et le rouvre, pour tous les organisms qui le partagent.

**Parse** Le modèle auquel une ligne ou un paquet doit correspondre : %1 à %8 capturent des nombres écrits en texte, %b1 à %b8 des octets seuls et %w1 à %w8 des mots de deux octets, ces deux derniers lus de 0 à 1. Vide, il prend tous les nombres de la ligne dans l'ordre ; une ligne qui ne correspond pas est ignorée et affichée comme sautée.

**Port** Un chemin de port saisi à la main, qui l'emporte sur le choix de Device tant qu'il est défini.

## Recette

**Capteur de lumière vers filtre** Fais écrire à la carte la valeur de son capteur sous forme d'un nombre de 0 à 1, trente fois par seconde, un par ligne. Laisse Parse vide, Values 1, BaudRate 115200. Fais un clic droit sur le Cutoff d'un Filter, choisis Contrôler avec SerialIn a, et le filtre suit le capteur. Surveille l'affichage sous Parse pour vérifier ce qui arrive.

## Organisms associés

SerialOut, Number, Follower, LFO
