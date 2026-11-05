# PROGETTOPAA — Gioco Strategico a Turni

Progetto finale per il corso **PAA (Programmazione ad Oggetti / Advanced)** — A.A. 2024/2025.
Gioco strategico a turni **1 vs 1 (Giocatore umano contro AI)** sviluppato in **Unreal Engine 5.4** e **C++**.

## Descrizione

Due fazioni si affrontano su una griglia **25×25** con ostacoli (montagne e alberi).
Ogni fazione schiera due unità, **Sniper** e **Brawler**, da posizionare e muovere a turni
fino all'eliminazione delle unità avversarie.

### Unità

| Unità   | Movimento | Tipo attacco   | Range | Danno  | Vita |
|---------|-----------|----------------|-------|--------|------|
| Sniper  | 3 celle   | A distanza     | 10    | 4 – 8  | 20   |
| Brawler | 6 celle   | Corto raggio   | 1     | 1 – 6  | 40   |

- Movimento scalare (orizzontale/verticale, **no diagonale**), senza attraversare ostacoli o unità.
- L'attacco a distanza ignora gli ostacoli; il corto raggio no.
- **Danno da contrattacco**: se lo Sniper attacca un altro Sniper, o un Brawler a distanza 1,
  subisce un danno di ritorno (1 – 3).

## Requisiti implementati

1. ✅ Compila correttamente, codice strutturato con polimorfismo ed ereditarietà (`ABaseUnit` → `ASniper`, `ABrawler`).
2. ✅ Griglia iniziale corretta e interamente visibile (vista dall'alto).
3. ✅ Posizionamento alternato delle unità dopo il lancio della moneta.
4. ✅ AI con azioni random (scelta di bersaglio e movimento).
5. ✅ Gioco a turni con condizione di vittoria/sconfitta.
6. ✅ Interfaccia (UMG) con turno corrente e vita di ogni unità.
7. ✅ Range di movimento evidenziato al click sull'unità (nascosto al secondo click).
8. ✅ Meccanismo del danno da contrattacco.
9. ✅ Storico delle mosse eseguite.
10. ✅ AI con pathfinding ottimizzato **A\*** (min-heap).

## Struttura del codice

```
Source/PROGETTOPAA/
├── GameManager.{h,cpp}   # GameMode: fasi di gioco, turni, input, AI, camera top-down
├── GridManager.{h,cpp}   # Griglia, ostacoli, pathfinding (BFS, A*), evidenziazioni
├── BaseUnit.{h,cpp}      # Classe base astratta delle unità (attacco, danno, movimento)
├── Sniper.{h,cpp}        # Unità a distanza
├── Brawler.{h,cpp}       # Unità corto raggio
└── MyUI.{h,cpp}          # Widget UMG (turno, vita, storico mosse, fine partita)
```

Note tecniche:
- **Range di movimento**: calcolato con una singola BFS flood-fill `O(N)` (`GetDistanceMap`).
- **A\***: coda a priorità (min-heap) con lazy deletion, `O(E log V)`.

## Requisiti di sistema

- **Unreal Engine 5.4**
- **Visual Studio 2022** con il workload *Game development with C++*
- Windows 10/11

## Come compilare ed eseguire

1. Clonare il repository.
2. Click destro su `PROGETTOPAA.uproject` → **Generate Visual Studio project files**.
3. Aprire la soluzione e compilare la configurazione **Development Editor | Win64**
   (oppure aprire direttamente il `.uproject`: l'editor proporrà di ricompilare i moduli).
4. Avviare l'editor, aprire la mappa `Content/Livello_Ufficiale` e premere **Play**.

## Comandi di gioco

- **Click** su una cella libera → posiziona / muove l'unità selezionata.
- **Click** su una propria unità → la seleziona e mostra il range di movimento; secondo click → nasconde.
- **Click** su un nemico in range → attacca.
- Pulsante **Termina Turno** → passa il turno all'AI.

## Note

Progetto individuale sviluppato con Unreal Engine 5.4 e C++; Blueprint usati solo dove
strettamente necessario per l'interfaccia UMG.
