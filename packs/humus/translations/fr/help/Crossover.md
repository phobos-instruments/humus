# Crossover

Un séparateur de fréquences qui envoie un signal en bandes, une paire de sorties stéréo par bande, de la plus grave à la plus aiguë.

Les séparations sont alignées en phase : câble toutes les bandes vers une même entrée et leur somme redonne le son d'origine, à plat. Tout ce qui se passe entre la séparation et la réunion ne touche qu'une seule bande : un Compressor sur chaque bande donne une dynamique multibande, un Fern sur les médiums seuls donne un écho dont la basse reste à l'écart, un Follower sur la sortie grave fait de la grosse caisse un signal de contrôle que les charlestons ne touchent pas. Un nouveau Crossover est à trois voies ; le menu déroulant Bands de son éditeur le remodèle sur place entre 2 et 5 bandes, en gardant les câbles et les réglages de séparation, et un Freq3 puis un Freq4 apparaissent à mesure que des bandes s'ajoutent. Traite les bandes et réunis-les, mais évite de mélanger le résultat réuni avec le signal direct dont il provient : un crossover fait tourner la phase, donc les deux s'annulent bizarrement quand on les superpose.

## Paramètres

**Freq1** La fréquence de séparation la plus basse, en Hz.

**Freq2** La fréquence de séparation suivante, vers le haut. Les séparations gardent leur ordre, donc on ne peut pas faire glisser une séparation sous la précédente.

**Slope** La raideur de la jonction entre les bandes. 24 dB/oct est le standard, aligné en phase et à somme plate. 12 dB/oct est plus doux, pour mêler les timbres ; dans ce mode, une bande sur deux a sa polarité inversée pour que la somme reste plate, ce qui est inaudible en soi. 48 dB/oct est presque un mur, donc chaque bande peut être poussée fort de son côté.

## Recette

**Batterie en trois bandes** Freq1 120, Freq2 2000, Slope 24 dB/oct. Câble un Mixer de batterie en entrée, un Compressor sur chaque paire de sorties avec une attaque plus lente sur la bande grave, et les trois Compressor vers l'entrée suivante, qui les resomme.

## Organisms associés

ParaEQ, Filter, StereoTool, Follower
