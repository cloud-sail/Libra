# Libra

## Features
- Procedual Map Generation
- Dijkstra's path finding
- Data-driven tilemaps
- Sprite Animations
- 2D particle system

## Gallery
> Demo  
> ![](Docs/Libra.gif)

## Controls
```
How to control
- P [Xbox Start] to start new game, or toggle pause in-game
- ESC [Xbox Back] to pause & leave game, and again to quit
- E,S,D,F [Xbox left stick] to drive forward & turn toward direction
- I,J,K,L [Xbox right stick] to aim turret toward direction
- Space [Xbox right trigger] to shoot bullets

Developer cheats
- F1 [Xbox UP]: toggle debug draw overlays
- F2 [Xbox DOWN]: toggle god mode (IsInvisible) - white ring
- F3 [Xbox LEFT]: toggle noclip (disable player physics) - black ring
- F4 [Xbox RIGHT]: toggle whole-map view
- F8: hard restart (delete/new game)
- T [Xbox Left Shoulder]: hold to slow time to 10%
- Y [Xbox Left Trigger]: hold to speed time to 4x
- T+Y [Xbox Left Shoulder and Trigger]: hold both to speed time to 8x
- O: step single update and pause
```

## How to run
Go to `PROJECT_NAME/Run/` and Run `PROJECT_NAME_Release_x64.exe`

## How to build
1. Clone Project
```bash
git clone --recurse-submodules https://github.com/cloud-sail/ChessDX.git
```
2. Open Solution `PROJECT_NAME.sln` file
- In Project Property Pages
  - Debugging->Command: `$(TargetFileName)`
  - Debugging->Working Directory: `$(SolutionDir)Run/`
