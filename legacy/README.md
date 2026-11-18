# Module `legacy` : moderniser du code ancien

Ce dossier simule ce qu'on trouve dans un logiciel de 25 ans : un petit générateur de rapport écrit "à l'ancienne" (années 2000), puis modernisé **sans changer son comportement**. L'intérêt est dans l'**historique Git**, qui raconte la méthode.

## Le code d'origine

`legacy_report.cpp` (commit `test: add legacy report module...`) calcule, pour un texte CSV `site,timestamp,kw`, une ligne de statistiques par site : nombre de mesures, min, max, moyenne et énergie (kWh).

Il est volontairement écrit dans le style d'époque :

| Défaut | Pourquoi c'est un problème |
|---|---|
| Variables globales (`g_sites`, `g_ns`, `g_buf`) | Les résultats s'accumulent d'un appel à l'autre, le code n'est pas réutilisable ni testable proprement |
| Tableaux de taille fixe (`char name[32]`, `char tmp[512]`, 16 sites, 64 Ko) | Dépassements silencieux ou débordement mémoire |
| `strtok`, `strcpy`, `sprintf` | Fonctions non sûres, `strtok` modifie la chaîne et garde un état caché |
| `atol` dans un `long` | 32 bits sous Windows : bug de l'an 2038 |
| Tout dans une fonction de 70 lignes | Lecture, calcul et mise en forme mélangés |

## La méthode en 3 étapes (3 commits)

1. **Caractériser** (`test: ...`). On écrit des tests qui figent ce que le code fait **aujourd'hui**, y compris ses bizarreries (marquées `QUIRK` dans `tests/LegacyReportTest.cpp`). On ne juge pas, on observe. Ces tests sont le filet de sécurité.
2. **Refactoriser** (`refactor: ...`). On remplace les globales et les tampons par `ReportBuilder` (`std::string`, `std::vector`). Les 15 tests de caractérisation passent **sans aucune modification** : preuve que le comportement est préservé. Les anciennes fonctions C restent comme façade pour ne pas casser les appelants.
3. **Corriger** (`fix: ...`). Un seul comportement est changé, volontairement, dans un commit à part, avec le test correspondant modifié et expliqué : les totaux ne s'accumulent plus entre deux appels.

Pour relire l'histoire : `git log --oneline -- legacy tests/LegacyReportTest.cpp`, puis `git show <commit>`.

## Les bizarreries (`QUIRK`) et leur état

| Comportement | Statut |
|---|---|
| Les totaux s'accumulent entre deux appels | **Corrigé** (3e commit) |
| La ligne d'en-tête `site,ts,kw` est comptée comme une mesure (`ts` et `kw` valent 0) | Conservé, documenté. Le corriger change les résultats des appelants existants : décision à prendre avec l'équipe |
| Noms de site coupés à 31 caractères (fusion de noms proches) | Conservé |
| Seuls les 16 premiers sites sont gardés, les autres disparaissent sans message | Conservé |
| Entrée coupée à 65 535 caractères, même au milieu d'une ligne | Conservé |
| Horodatages désordonnés : énergie négative | Conservé |
| Une ligne qui ne rentre pas dans le tampon de sortie est supprimée | Conservé |

Corrigés **par effet de bord** du refactoring (nouveaux tests dans `tests/ReportBuilderTest.cpp`) : débordement du tampon de 512 octets avec de très grandes valeurs, bug de l'an 2038, entrée nulle, nom de site traité comme chaîne de format.

**Pourquoi conserver des défauts ?** Dans un vieux logiciel, un comportement "faux" peut être utilisé par d'autres parties du code ou par des clients. On sépare le **refactoring** (ne rien changer) des **corrections** (changer volontairement), pour que chaque changement de comportement soit visible, décidé et relisible.
