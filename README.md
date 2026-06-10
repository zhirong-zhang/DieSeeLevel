# Die, See, Level

![Unreal Engine](https://img.shields.io/badge/Unreal_Engine-5.7-black?logo=unrealengine&logoColor=white)
![C++](https://img.shields.io/badge/C%2B%2B-blue?logo=cplusplus&logoColor=white)
![Platform](https://img.shields.io/badge/Windows-10%2F11-0078d4?logo=windows&logoColor=white)
![Controller](https://img.shields.io/badge/Controller-DualSense-003087?logo=playstation&logoColor=white)

A first-person action prototype built in Unreal Engine 5.7. You physically rotate a PS5 DualSense controller to control an in-game level indicator (Level) and change the world's gravity. Touching an egg spawns an enemy, and you need to keep rotating gravity to dodge their attacks.

---

## Gameplay Video: https://youtu.be/CIPvtUEF5VE

---

## How It Works

1. Hold the designated button — your character stops moving and a visual gravity indicator (GravityPlane) appears in the scene
2. Physically tilt the controller — the indicator rotates with it, showing you where gravity will go
3. Release — the indicator hides and the character smoothly rotates to that orientation
4. Once aligned (angle diff < 1°), gravity is committed to the CMC and the character drops toward the new floor
5. That same frame fires a level refresh: raycasts shoot out in random directions, and wherever one hits a wall tagged `SpawnableWall`, a vent and an escape portal spawn
6. The vent ejects a Boss that immediately syncs to the same gravity and starts chasing
7. Reach the portal or flip gravity again to shake it off

---

## Under the Hood

### Part 1 · Gyro-Driven Gravity System

**Getting DualSense Input into UE**

UE doesn't natively support DualSense, and the built-in Raw Input plugin can't touch gyroscope data either. The `WindowsDualsense_ds5w` plugin bypasses all that by talking directly to the hardware SDK and registering the IMU output as float vector input channels. Enhanced Input picks those up and routes them to the character.

**Driving the Gravity Indicator (`FPSCharacter.cpp · Tick`)**

While the button is held, each frame converts gyro angular velocity into a delta angle. To keep noise down, only the dominant axis is accepted on any given frame — if Pitch has a bigger absolute value than Roll, only Pitch accumulates, and vice versa. Anything below 0.01° gets ignored. The accumulated delta is applied to the indicator's starting orientation via quaternion multiplication, which avoids the 180° flip discontinuity and gimbal lock you'd run into with Euler angle addition.

```
delta per frame   =  gyro angular velocity × DeltaTime × sensitivity
single-axis lock  →  only the larger-abs axis accumulates each frame
quaternion compose →  NewQuat = InitialPlaneQuat * QPitch * QRoll
```

**Snapping to the Target**

On release, the indicator's current Up and Forward vectors each snap to the nearest world axis, and `MakeFromXZ` builds the target rotation. The character then interpolates there with `RInterpTo`. The issue with `RInterpTo` is that steps get smaller and smaller as it closes in — floating point drift means it can take dozens of frames to settle, and the "done" condition never cleanly triggers. Fix is an early exit: once the quaternion `AngularDistance` drops below 1°, it hard-snaps to the target, writes gravity into the CMC, and sets `MOVE_Falling` so the character falls toward the new floor rather than hanging in the air.

**Movement and Camera After a Gravity Flip**

When Up is no longer world Z, UE's Controller rotation system gives wrong results — movement drifts diagonally, camera locks up. The fix is cutting the Controller coupling entirely, disabled in both the constructor and `BeginPlay` (some UE systems quietly re-enable it during init, so one isn't enough):

- **Movement** — uses `GetActorForwardVector()` / `GetActorRightVector()`, which are local to the Actor and automatically correct after it rotates to the new orientation
- **Yaw** — quaternion rotation around the Actor's local Up axis via `AddActorLocalRotation`, so turning left/right works on any surface
- **Pitch** — `CurrentCameraPitch` accumulates manually and writes directly to the camera's relative rotation, completely bypassing Controller space

All movement and look input returns early during the transition (`bIsShiftingGravity || bIsSmoothRotating`) so nothing fights the alignment.

---

### Part 2 · Boss AI in a Multi-Gravity World

**Gravity Sync**

Each frame, the Boss reads the player CMC's `GetGravityDirection()` rather than the player's Up vector — the latter wobbles during the rotation transition and causes the Boss to jitter. When there's a mismatch, it syncs immediately and does two things to avoid physics explosions:

1. `SetMovementMode(MOVE_Falling)` — breaks free from the old floor's surface snapping before the new gravity kicks in
2. `AddActorWorldOffset(GetActorUpVector() * 60.0f)` — nudges the capsule away from the incoming surface so the physics engine doesn't catch it clipping through and fire a depenetration teleport

**Orientation**

`bOrientRotationToMovement` is unreliable once gravity isn't pointing down — it twitches. Instead, each frame constructs a target quaternion with `MakeFromZX` (Z = new up, X = direction to player) and interpolates with `QInterpTo`. The Boss won't start moving until its orientation is within 2° of the target, so it doesn't chase sideways while it's still flipping over.

**No NavMesh**

NavMesh is baked for a flat Z-up floor. After any gravity flip, there's no valid path and the AI just stands there. Rebuilding it dynamically on every switch is too slow to be practical. The Boss skips pathfinding entirely: each frame it computes the vector to the player, projects it onto the current gravity plane, and calls `AddMovementInput`. Works on floors, walls, ceilings — anything.

`AutoPossessAI = PlacedInWorldOrSpawned` ensures dynamically spawned Bosses get a controller. `AirControl = 1.0` stops them from freezing mid-air right after a gravity switch.

---

### Part 3 · DynamicGravityComponent

A component you attach to any physics-simulated Actor to make it follow the player's gravity.

On `BeginPlay` it disables Chaos's built-in gravity on the mesh and caches a reference to the player. Each tick it reads the current gravity direction. If it changed, `WakeRigidBody()` fires before the force is applied — Chaos puts idle rigid bodies to sleep, and sleeping bodies drop applied forces on the floor. Without the wake call, the prop just sits there until something bumps it, then snaps suddenly. After waking, `AddForce(..., bAccelChange = true)` applies 980 cm/s² of acceleration. The `bAccelChange` flag makes the engine multiply by mass internally, so every prop falls at the same rate regardless of how heavy it is.

---

### Part 4 · Dynamic Level Spawning

**When to spawn**

`SpawnRoomObjects()` is called at the end of the angle convergence check — right after `bIsSmoothRotating` is set to false. At that point the coordinate system is stable and the raycast directions are trustworthy. Setting the flag false first prevents a double-trigger on the same gravity switch.

**Finding a wall**

Raycasts fire outward from the player in up to 10 random directions. Each hit checks the Actor for a `SpawnableWall` tag — no tag, try again. If all 10 miss, nothing spawns that round. Level designers just tag the surfaces they want vents and portals to show up on.

**Vent orientation**

The vent's rotation is built from the raycast's `ImpactNormal` as the X axis (`MakeFromX`). That makes it face inward automatically, whether the ray hit a floor, ceiling, or side wall. No manual rotation corrections needed.

**Boss spawn position**

VentSpawner's `SpawnArea` (a BoxComponent) is offset 100 units along the vent's forward axis. The Boss spawns at a random point inside that box, so it always starts in open air in front of the wall rather than inside it.

---

## Project Structure

```
Source/G_004/
├── FPSCharacter.cpp/h               # core player — gyro input, gravity switching, level spawning
├── BossAI.cpp/h                     # boss — gravity sync, quaternion orientation, vector pathfinding
├── DynamicGravityComponent.cpp/h    # attach to any physics prop to make it follow player gravity
├── VentSpawner.cpp/h                # wall-mounted spawner that ejects bosses
├── Variant_Combat/                  # melee combat (combo/charged attacks, StateTree AI)
├── Variant_Platforming/             # platformer (double jump, wall jump, coyote time, dash)
└── Variant_SideScrolling/           # side-scroller (moving platforms, jump pads, NPCs)

Plugins/Controll1a9e9fae517dV6/      # DualSense Windows plugin (ds5w)
codethrough.html                     # annotated walkthrough of each system's problems and solutions
```

---

## Requirements

- Unreal Engine 5.7
- Windows 10/11 (the DualSense plugin uses the Windows HID API)
- PS5 DualSense controller (USB or Bluetooth)
- Visual Studio 2022

## Getting Started

1. Clone the repo
2. Right-click `G_004.uproject` → **Generate Visual Studio project files**
3. Build the `Development Editor` configuration in Visual Studio
4. Open the editor, connect the DualSense, and hit **Play In Editor**
5. Tag any walls where vents and portals should appear with the `SpawnableWall` Actor Tag — nothing spawns on untagged surfaces
