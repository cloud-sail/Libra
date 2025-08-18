# SD1 Libra

# How to build
- Please prepare both Engine and Libra
- Open the Starship folder and open the Starship.sln
- Change debugging settings TargetFileName $(SolutionDir)Run/
- Press F5

# Run release exe
- Open Libra\Run\Libra_Release_x64.exe

# How to control
- P [Xbox Start] to start new game, or toggle pause in-game
- ESC [Xbox Back] to pause & leave game, and again to quit
- E,S,D,F [Xbox left stick] to drive forward & turn toward direction
- I,J,K,L [Xbox right stick] to aim turret toward direction
- Space [Xbox right trigger] to shoot bullets

# Developer cheats
- F1 [Xbox UP]: toggle debug draw overlays
- F2 [Xbox DOWN]: toggle god mode (IsInvisible) - white ring
- F3 [Xbox LEFT]: toggle noclip (disable player physics) - black ring
- F4 [Xbox RIGHT]: toggle whole-map view
- F8: hard restart (delete/new game)
- T [Xbox Left Shoulder]: hold to slow time to 10%
- Y [Xbox Left Trigger]: hold to speed time to 4x
- T+Y [Xbox Left Shoulder and Trigger]: hold both to speed time to 8x
- O: step single update and pause


# Known issues
App Close (Alt+F4 or X-out or right click -> Close) emits an intentional error (then closes)