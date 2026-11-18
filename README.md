# EnergyScope

Mini logiciel **client-serveur** pour visualiser des mesures de production d'énergie.
Projet d'apprentissage qui reprend, en petit, la stack d'un logiciel industriel :
C++, MFC, OpenGL, SQL, CMake, Jenkins, Git.

## Vocabulaire simple

| Mot | Explication |
|---|---|
| **Client-serveur** | Deux programmes. Le *serveur* garde les données. Le *client* est la fenêtre que l'utilisateur voit et il demande les données au serveur. |
| **Client lourd** | Un vrai programme installé sur le PC (pas un site web). |
| **MFC** | Bibliothèque Microsoft (années 90) pour fabriquer des fenêtres, boutons et menus Windows en C++. Très présente dans les vieux logiciels. |
| **OpenGL** | Bibliothèque pour dessiner avec la carte graphique : courbes, formes, 3D. |
| **SQL / SQLite** | Langage pour interroger une base de données. SQLite est une base contenue dans un seul fichier. |
| **CMake** | Outil qui décrit comment compiler le projet, sur n'importe quelle machine. |
| **Jenkins** | Robot qui compile et teste le projet automatiquement à chaque modification. |
| **Test unitaire** | Petit programme qui vérifie qu'un morceau de code fait bien son travail. |

## Avancement

- [x] Étape 1 : bibliothèque `core` (modèle de mesure + stockage SQLite) et tests
- [x] Étape 2 : serveur réseau (protocole texte sur TCP, `energy_server`)
- [x] Étape 3 : client MFC (fenêtre Windows, tableau des mesures, `energy_client`)
- [x] Étape 4 : vue OpenGL (courbe de puissance, axes, grille, texte)
- [x] Étape 5 : export PDF vectoriel et réduction du poids (1,4 Mo -> 24 Ko)
- [ ] Étape 6 : Jenkinsfile et module legacy refactoré

## Compiler, tester, lancer (Windows, Visual Studio)

Dans PowerShell, à la racine du projet :

```
.\scripts\build.ps1     # compile et lance les tests
.\scripts\run.ps1       # démarre le serveur puis la fenêtre
```

`run.ps1` arrête le serveur quand tu fermes la fenêtre. Les données sont dans `energy.sqlite`.

Sans les scripts, depuis un terminal "x64 Native Tools Command Prompt for VS" :

```
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
```

## Lancer le serveur

```
build\server\energy_server.exe 5555 energy.sqlite
```

Le serveur n'écoute que sur `127.0.0.1` (ce PC). Protocole : une ligne par demande.

| Demande | Réponse |
|---|---|
| `ADD solar-1 1700000000 12.5` | `OK` |
| `GET solar-1` | `OK 2` puis une ligne `<heure> <kW>` par mesure |
| autre chose | `ERR <raison>` |

### Comment ça marche (simple)

Un **socket** est une "prise" réseau. Le serveur crée une prise (`socket`), la fixe à un
numéro de port (`bind`), puis attend (`listen`, `accept`). Un client se branche sur le
même port. Ensuite les deux s'envoient du texte, comme au téléphone.
Le TCP garantit que les octets arrivent dans l'ordre, mais pas qu'ils arrivent par
paquets entiers : le serveur lit donc jusqu'au retour à la ligne `\n` (`readLine`).

### Protections

- Écoute sur localhost uniquement.
- Ligne limitée à 4096 octets, délai de 5 s si le client ne dit rien.
- Valeurs vérifiées (heure entière, puissance finie, nom de site ≤ 64 caractères).
- La logique du protocole (`handleRequest`) est séparée du réseau : testable sans socket.

## Lancer le client MFC

1. Démarrer le serveur (voir plus haut), port 5555.
2. Lancer `build\app\energy_client.exe`.
3. Menu **Données > Ajouter des données d'exemple...** : crée 24 mesures pour un site (ex. `solar-1`) et les affiche.
4. Menu **Données > Charger un site...** (Ctrl+L) : affiche les mesures d'un site déjà enregistré.

Prérequis : Visual Studio avec le composant **MFC C++**.

### MFC en termes simples

MFC organise une application en 4 objets qui travaillent ensemble (modèle *Document/Vue*) :

| Objet | Rôle | Dans ce projet |
|---|---|---|
| **Application** (`CWinApp`) | Démarre le programme, crée la fenêtre | `EnergyApp.cpp` |
| **Frame** (`CFrameWnd`) | La fenêtre : barre de titre, menu, bordure | `MainFrame.h` |
| **Document** (`CDocument`) | Les **données** et la logique (sans dessin) | `EnergyDoc.*` |
| **Vue** (`CView`) | L'**affichage** des données du document | `MeasurementView.*` (un tableau) |

Autres notions utiles :
- **Message map** (`BEGIN_MESSAGE_MAP`) : table qui dit "quand l'utilisateur clique sur ce menu, appelle cette fonction".
- **Fichier de ressources** (`EnergyScope.rc`) : décrit menus, boîtes de dialogue et textes, intégrés dans le `.exe`.
- **Tableau virtuel** (`LVS_OWNERDATA`) : le tableau demande le texte des seules lignes visibles. Il reste fluide même avec des centaines de milliers de mesures.
- Quand le document change, `UpdateAllViews()` prévient chaque vue, qui se redessine. Cette séparation permettra d'ajouter la vue OpenGL (étape 4) sans toucher aux données.

### OpenGL en termes simples

La fenêtre est coupée en deux par un **séparateur** (`CSplitterWnd`) : le tableau à gauche, le graphique à droite.
Les deux sont des **vues du même document** : charger un site met à jour les deux d'un coup.

Pour dessiner avec OpenGL dans une fenêtre Windows (`ChartView.cpp`) :

1. **Format de pixels** (`ChoosePixelFormat`) : on dit à Windows quel type de pixels on veut (couleurs RGBA, double tampon).
2. **Contexte OpenGL** (`wglCreateContext`) : l'"atelier" de dessin attaché à la fenêtre. Créé à la naissance de la fenêtre, détruit à sa mort.
3. **Double tampon** : on dessine dans une image cachée, puis `SwapBuffers` l'affiche d'un coup. Pas de scintillement.
4. **Projection** (`glOrtho`) : on dit à OpenGL quelle zone de données (temps × kW) correspond à la zone de la fenêtre. Les points de la courbe sont ensuite envoyés tels quels, sans conversion en pixels.
5. **Courbe** : un tableau de points envoyé d'un coup (`glDrawArrays`, `GL_LINE_STRIP`), rapide même avec beaucoup de points.
6. **Texte** : OpenGL ne sait pas écrire. On fabrique des listes d'affichage à partir d'une police Windows (`wglUseFontBitmaps`).

Le calcul des axes (`ChartScale.cpp` : bornes, "graduations rondes" 1-2-5) est séparé du dessin, donc testé sans fenêtre.

## Export PDF et réduction du poids

Menu **Fichier > Exporter en PDF...** (grisé tant qu'aucune donnée n'est chargée), ou en ligne de commande :

```
build\tools\energy_export.exe solar-1 courbe.pdf            # version optimisée
build\tools\energy_export.exe solar-1 courbe_lourde.pdf --naive   # pour comparer
```

Le PDF est **vectoriel** : la courbe est stockée comme une suite de points reliés par des traits (et non comme une image), donc nette à tout niveau de zoom. Le fichier est écrit à la main (`PdfWriter.cpp`) : en-tête, objets, table des positions (`xref`) et fin. Aucun outil externe sauf `zlib` pour la compression.

### Le gain, levier par levier

Mesuré par le test `PdfChart.EachLeverMakesTheFileNoHeavier` : 20 160 points (7 jours, une mesure toutes les 30 s).

| Levier (cumulatif) | Taille | Idée |
|---|---:|---|
| Version "naïve" | 1 399 979 o | chaque segment est un tracé séparé, avec sa couleur et son épaisseur, 6 décimales |
| + un seul tracé pour toute la courbe | 468 820 o | on n'écrit la couleur et l'épaisseur qu'une fois |
| + 2 décimales au lieu de 6 | 307 308 o | 0,01 point (1/7200 de pouce) est invisible à l'œil |
| + simplification (0,1 pt) | 82 841 o | on retire les points qui n'ajoutent aucun détail visible |
| + compression Flate | **23 994 o** | le texte répétitif se compresse très bien |

Résultat : environ **58 fois plus léger**. En plus : pas de police embarquée (on utilise Helvetica, déjà connue de tout lecteur PDF).

### La simplification (algorithme de Ramer-Douglas-Peucker)

On trace une corde entre le premier et le dernier point. Si le point le plus éloigné de cette corde est à moins de la tolérance (0,1 pt), tous les points intermédiaires sont inutiles : on les supprime. Sinon on garde ce point et on recommence de chaque côté. Garantie testée : **aucun point d'origine n'est à plus de la tolérance de la courbe simplifiée**. Une version intermédiaire mesurait la distance à la *droite* au lieu du *segment* et cassait cette garantie ; un test l'a détecté.

Vérification visuelle : le rendu optimisé diffère de la version lourde sur 0,2 % des pixels (lissage des bords uniquement).

Limite connue : les heures du PDF sont en UTC (le PDF est produit par le code commun, indépendant du fuseau du PC).

Limite connue : les appels réseau bloquent la fenêtre jusqu'à 5 s au maximum (ils tournent dans le thread de l'interface).

## Structure

```
core/    logique métier : base SQLite + protocole texte (sans interface)
net/     petits outils Winsock communs au serveur et au client
server/  programme serveur
client/  bibliothèque cliente EnergyClient (sans MFC, testée)
tools/   energy_export : export PDF en ligne de commande
app/     application MFC (Application, Frame, Document, vues tableau + OpenGL, dialogue)
scripts/ build.ps1 (compiler + tester), run.ps1 (lancer)
tests/   tests unitaires et de bout en bout (GoogleTest)
```

Le client MFC n'utilise jamais Winsock directement : il passe par `EnergyClient`.
Raison technique : `winsock2.h` et `afxwin.h` (MFC) ne se mélangent pas dans un même fichier.

## Choix techniques

- Requêtes SQL **paramétrées** (`?1`, `?2`) : le texte saisi n'est jamais collé dans la requête, ce qui empêche les injections SQL. Un test le vérifie.
- Ressources (base, requêtes) libérées automatiquement (RAII) : pas de fuite mémoire.
- Erreurs signalées par exceptions `std::runtime_error` avec le message SQLite.
