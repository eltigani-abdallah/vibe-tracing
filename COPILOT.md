🚂 Projet Ray-Tracing : "Le Voyage de Chihiro" (Vibe Coding Plan)
Objectif : Créer un moteur de Ray Tracing en C capable de rendre la scène de la "Sixième Station" (Gare inondée). Contrainte : Mode "Cinématique" (Caméra automatique) + Mode Interactif basique. Méthode : Vibe Coding (Utilisation intensive de l'IA pour le code boilerplate et mathématique). Équipe : 3 Personnes. Durée estimée : 1 Journée "Commando".

🛠️ Pré-requis Techniques
Langage : C Norme C11.
Affichage : SDL2.
Pas de 3D externe : Tout est codé avec des primitives mathématiques (Sphères, Plans, Cubes).

📅 Planning de la Journée (Sprint par Sprint)
09:00 - 10:30 : Phase 1 - Le Squelette (Setup)
Objectif : Une fenêtre noire qui ne crash pas et des structures de données prêtes.
👤 Membre A : L'Architecte (Window & Main)
Tâche : Mettre en place le Makefile et la fenêtre SDL2.
Prompt IA : "Génère un squelette de projet C avec SDL2. Je veux un main.c qui ouvre une fenêtre 800x600, gère la boucle d'événements (fermeture fenêtre) et prépare un buffer de pixels pour dessiner dedans."
Livrable : Une fenêtre noire qui s'ouvre et se ferme proprement.
👤 Membre B : Le Mathématicien (Vectors)
Tâche : La librairie mathématique (le cœur du moteur).
Prompt IA : "Crée un header vec3.h et vec3.c. Implémente une struct vec3 et toutes les opérations vectorielles nécessaires pour un Raytracer (add, sub, dot, cross, normalize, length). Utilise static inline pour la performance."
Livrable : vec3.h et vec3.c fonctionnels.
👤 Membre C : La Caméra (Ray Launcher)
Tâche : Générer les rayons depuis l'œil.
Prompt IA : "Crée une structure Camera et Ray. Écris une fonction ray_for_pixel(camera, x, y) qui renvoie un rayon partant de la caméra et passant par le pixel x,y de l'écran."
Livrable : La logique de la caméra prête à être connectée.

10:30 - 12:30 : Phase 2 - Le Premier Rendu (Sphère & Plan)
Objectif : Voir des objets 3D simples à l'écran.
👥 Tout le monde sur le fichier render.c
Assemblage : Connecter la Caméra (C) aux Maths (B) dans la Fenêtre (A).
Prompt IA Global : "Écris la fonction de rendu principale. Pour chaque pixel x,y : lance un rayon. Si le rayon touche une sphère (équation mathématique simple), colorie en rouge. Sinon, colorie en bleu."
Tâche Spéciale Chihiro :
Demander à l'IA d'ajouter l'intersection avec un Plan Infini (y = 0). Ce sera l'eau.
Prompt : "Ajoute une fonction hit_plane pour un plan horizontal infini à y=0."

13:30 - 15:30 : Phase 3 - La Scène "Chihiro" (Building the World)
Objectif : Remplacer les formes de test par la gare.
👤 Membre A : L'Eau (Le Miroir)
Focus : La réflexion.
Prompt IA : "Implémente la réflexion spéculaire parfaite. Si un rayon touche le plan y=0 (l'eau), il doit rebondir (angle incident = angle réfléchi) et aller chercher la couleur du ciel ou des objets. Limite la récursion à 3 rebonds."
👤 Membre B : Les Objets (Train & Poteaux)
Focus : Placer les formes géométriques.
Prompt IA : *"Je veux construire une scène. Crée une fonction scene_intersect.
Place des cylindres verticaux (poteaux) tous les 20 unités sur l'axe Z.
Place un long parallélépipède (Box) pour faire le quai de gare à fleur d'eau.
Utilise des mathématiques simples (pas de maillage)."*
👤 Membre C : L'Atmosphère (Ciel & Lumière)
Focus : Les couleurs Ghibli.
Prompt IA : "Crée une fonction get_sky_color(ray_dir). Fais un dégradé vertical (Lerp) : Orange Sunset à l'horizon vers Violet Profond au zénith. C'est pour une ambiance 'Voyage de Chihiro'."

15:30 - 17:00 : Phase 4 - Le Mouvement (Cinématique vs Interactif)
Objectif : Ça bouge tout seul (mode démo) ou on contrôle (mode jeu).
L'Implémentation du "Dual Mode"
Variable globale : bool auto_mode = true;
Prompt IA pour Membre A : "Dans la boucle principale, ajoute une logique de caméra :
Si auto_mode == true : La caméra avance toute seule sur l'axe Z (cam.z += 0.1) et oscille légèrement en Y pour simuler la respiration.
Si j'appuie sur ESPACE : auto_mode = false.
Si auto_mode == false : Les touches W/S avancent/reculent, A/D strafe gauche/droite."
L'Astuce de fluidité (CRUCIAL)
Le Raytracing CPU est lent.
Prompt IA : "Optimise le rendu. Quand la caméra bouge (mode auto ou interactif), rends l'image en basse résolution (divise la résolution par 4). Quand la caméra s'arrête (ou sur demande), rends en HD."

17:00 - 18:00 : Phase 5 - Polish & "Vibe" Final
Ajustement des couleurs : Tweaker le code pour que l'eau soit plus sombre et le ciel plus vibrant.
Effet d'eau : Ajouter une petite perturbation sinus (sin(x)*0.1) sur la normale de l'eau pour faire des vaguelettes.
Nettoyage du code : Ajouter des commentaires explicatifs générés par l'IA pour la soutenance.


!!! Vous devez découper votre avancement avec Git : une feature -> un commit

video exemple: https://www.youtube.com/watch?v=VbRmFSQYeac&list=RDVbRmFSQYeac&start_radio=1

