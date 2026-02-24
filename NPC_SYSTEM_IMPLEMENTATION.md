# NPC System Implementation Guide

## Overview
This document details the implementation of NPC movement, pathfinding, and rotation logic within the Ray Map Editor and generated BennuGD scripts.

## Core Rotation Logic (Critical)

### The Coordinate System Mismatch
A critical issue identified during development is the mismatch between BennuGD's screen coordinate system and the 3D Renderer/Physics world system:

1.  **BennuGD (Screen):** Y-axis grows **DOWNWARD** (South). Angles are typically Clockwise (CW).
2.  **Renderer/World (MD3/OpenGL):** Y-axis grows **UPWARD** (North). Angles are Counter-Clockwise (CCW).

### The Solution
To correctly orient NPCs to face their movement direction, we must transform the angle calculated by `fget_angle` as follows:

1.  **Negate the Angle:** We use `-fget_angle(x1, y1, x2, y2)` to convert the CW angle (Bennu) to a CCW angle (Renderer).
2.  **Unit Conversion:** `fget_angle` returns **milli-degrees**. The renderer expectation depends on the function, but for internal logic we often need radians or standardized degrees.
    *   **Milli-degrees to Radians:** `angle * 0.0000174533`
    *   **Radians to Milli-degrees:** `radians * 57295.8`
3.  **Smooth Rotation:** We use `near_angle` to interpolate the current angle towards the target angle, creating natural turns instead of instant snaps.

### Implementation Code (ProcessGenerator)
The following code snippet (from `processgenerator.cpp`) illustrates the correct logic:

```c
// Calculate target angle from current position to target position
// Negate fget_angle to fix Axis Mismatch (Y-down vs Y-up)
target_ang = -fget_angle(*cur_x * 1000, *cur_y * 1000, target_x * 1000, target_y * 1000) + angle_offset;

// Normalize angle to [0, 360000)
while (target_ang >= 360000.0) target_ang -= 360000.0; end
while (target_ang < 0.0) target_ang += 360000.0; end

// Smooth rotation using near_angle
// Convert internal angle (radians) to milli-degrees for the function
int cur_milli = *cur_angle * 57295.8;
// Turn speed: 5000 (5 degrees) per frame
int next_milli = near_angle(cur_milli, target_ang, 5000);
// Convert result back to radians for the engine
*cur_angle = next_milli * 0.0000174533;
```

## Path Following Behavior
- The NPC follows a set of waypoints (`path_nodes`).
- **Translation:** Uses standard vector math to move towards the current target node.
- **Rotation:** Uncoupled from translation output but driven by the same target vector. This allows the NPC to turn while moving or rotate in place if needed (though currently implemented as turn-while-move).

## Future AI Behaviors
When implementing more complex AI (e.g., chasing, fleeing, patrolling):
1.  **Always use the Negated Angle logic** for any calculation involving `fget_angle` for 3D orientation.
2.  **Maintain Separation of Concerns:** Keep movement (position update) and rotation (angle update) logic separate to allow for "strafing" or "turret" behaviors where looking direction differs from moving direction.
3.  **State Machines:** Use the existing separate process structure to implement states (Idle, Patrol, Chase).

## Troubleshooting "Backward" Movement
If an NPC appears to move backward:
1.  **Check 3D Model Orientation:** The model itself might differ from the standard "Front = +X" or "Front = +Y" assumption. Use `angle_offset` to correct this PER ENTITY in the editor.
2.  **Verify Axis Logic:** Ensure `target_ang` is calculated with `-fget_angle`. If the axis is inverted (positive `fget_angle`), vertical movement will look correct but horizontal might be flipped, or vice versa.
