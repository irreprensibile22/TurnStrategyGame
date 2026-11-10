# TurnStrategy — Turn-Based Strategy Game

A personal project of mine: a turn-based strategy game, **1 vs 1 (human player vs AI)**,
built with **Unreal Engine 5.4** and **C++**.

## Overview

Two factions face off on a **25×25** grid with obstacles (mountains and trees).
Each faction fields two units, a **Sniper** and a **Brawler**, which are placed and then moved
turn by turn until one side's units are wiped out.

### Units

| Unit    | Movement | Attack type | Range | Damage | Health |
|---------|----------|-------------|-------|--------|--------|
| Sniper  | 3 cells  | Ranged      | 10    | 4 – 8  | 20     |
| Brawler | 6 cells  | Melee       | 1     | 1 – 6  | 40     |

- Grid movement (horizontal/vertical, **no diagonals**), without passing through obstacles or units.
- Ranged attacks ignore obstacles; melee attacks don't.
- **Counterattack damage**: if the Sniper attacks another Sniper, or a Brawler at distance 1,
  it takes return damage (1 – 3).

## Features

- Game grid with procedurally generated obstacles (configurable percentage), always fully connected.
- Alternating unit placement after a coin toss.
- Turn-based play with win/lose conditions.
- UMG interface showing the current turn, each unit's health and the move history.
- Movement range highlighted when clicking a unit (hidden on a second click).
- Counterattack damage mechanic.
- AI with random actions and optimized **A\*** pathfinding (min-heap).
- During the AI turn, the movement range is shown while its units are still idle; after a short pause, the units make their moves.
- The coin toss result is also shown in the move history.

## Code structure

```
Source/TurnStrategy/
├── GameManager.{h,cpp}   # GameMode: game phases, turns, input, AI, top-down camera
├── GridManager.{h,cpp}   # Grid, obstacles, pathfinding (BFS, A*), highlights
├── BaseUnit.{h,cpp}      # Abstract base class for units (attack, damage, movement)
├── Sniper.{h,cpp}        # Ranged unit
├── Brawler.{h,cpp}       # Melee unit
└── MyUI.{h,cpp}          # UMG widget (turn, health, move history, end of game)
```

Technical notes:
- **Movement range**: computed with a single flood-fill BFS, `O(N)` (`GetDistanceMap`).
- **A\***: priority queue (min-heap) with lazy deletion, `O(E log V)`.

## System requirements

- **Unreal Engine 5.4**
- **Visual Studio 2022** with the *Game development with C++* workload
- Windows 10/11

## Building and running

1. Clone the repository.
2. Right-click `TurnStrategy.uproject` → **Generate Visual Studio project files**.
3. Open the solution and build the **Development Editor | Win64** configuration
   (or just open the `.uproject` directly: the editor will offer to rebuild the modules).
4. Launch the editor, open the `Content/Livello_Ufficiale` map and press **Play**.

## Controls

- **Click** on a free cell → place / move the selected unit.
- **Click** on one of your units → select it and show its movement range; click again → hide it.
- **Click** on an enemy in range → attack.
- **Termina Turno** (End Turn) button → hand the turn over to the AI.
- **Esc** → quit the game.

## Building the executable (packaging)

1. (Optional) In `Edit → Project Settings → Platforms → Windows`, set
   `Build/Windows/Application.ico` as the **Game Icon**.
2. `Platforms → Windows → Build Configuration → Shipping` (or **Development** for testing).
3. `Platforms → Windows → Package Project` and pick the output folder.
4. Result: `Windows/TurnStrategy.exe` (to distribute it, copy the **entire** `Windows/` folder).

The game starts in windowed mode (with title bar and X); you can also close it with **Esc** or **Alt+F4**.
