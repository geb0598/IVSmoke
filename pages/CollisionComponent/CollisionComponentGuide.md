# Collision Component Guide

`UIVSmokeCollisionComponent` is a dynamic primitive component attached to the `AIVSmokeVoxelVolume`. It translates the abstract voxel grid data into actual **physics geometry** that the Unreal Engine physics system (Chaos/PhysX) can interact with.

## Overview

Unlike standard static meshes, the shape of the smoke changes every frame during expansion and dissipation. This component generates collision bodies dynamically to match the current smoke shape.

### How It Works: Greedy Meshing

To maintain high performance, the component **does not** create a collision box for every single voxel. Instead, it utilizes a **Binary Greedy Meshing** algorithm.

1. It scans the voxel grid.
2. It identifies adjacent active voxels along the X, Y, and Z axes.
3. It merges them into the largest possible `FKBoxElem` (Box Collider).

**Result:** A smoke cloud containing 1,000 active voxels might only require 100-200 large physics boxes to represent its volume, significantly reducing physics simulation overhead.

---

## Common Use Cases

### 1. Blocking AI Line of Sight (Default)

By default, the component is configured to **Block the Visibility channel** while ignoring everything else.

* **Behavior:** AI perception systems using Line of Sight traces will be blocked by the smoke.
* **Player:** Players and projectiles will pass through the smoke freely.

### 2. Physical Blockage

You can change the Collision Profile to **BlockAll** or **Pawn** to make the smoke physically solid.

* **Behavior:** Players cannot walk through the smoke; it acts as a solid wall.
* **Context:** Useful for magic spells ("Ice Wall") or sci-fi force fields, rather than traditional gas smoke.

---

## Configuration

The component supports standard Unreal Engine collision settings in the **Details Panel**, along with specific performance tuning parameters.

### Performance Throttling

Rebuilding physics geometry is an expensive CPU operation. To prevent framerate drops, the component includes a built-in throttling system that skips updates if the changes are insignificant.

| Property | Description | Recommendation |
| --- | --- | --- |
| **Collision Enabled** | Master switch. If false, no physics bodies are generated. | Disable if you only need visual smoke (e.g., background ambience). |
| **Min Update Interval** | The minimum time (in seconds) between physics rebuilds. | **0.2s - 0.5s**. Frequent updates (e.g., 0.0s) will cause stuttering. |
| **Min Update Voxel Num** | The minimum number of voxels that must spawn/die to trigger a rebuild. | **50 - 100**. Prevents rebuilding when the smoke is barely changing. |

---

## Debugging

You can visualize the optimized physics geometry to verify that the Greedy Meshing is working effectively.

1. Enable `bDebugEnabled` on the parent `AIVSmokeVoxelVolume`.
2. Enable `bDebugEnabled` on this `UIVSmokeCollisionComponent`.

**Visuals:**

* **Colored Wireframes:** Represents the generated `FKBoxElem` bounds.
* **Color Variance:** Each distinct box is assigned a random color to show how the voxels were merged.

---

> **Note: Interaction with Voxel Simulation**
> This component does **not** affect the expansion of the smoke itself.
> * **Smoke Expansion:** Controlled by `VoxelCollisionChannel` in the Voxel Volume actor (Trace checks).
> * **Physics/AI Blocking:** Controlled by the Collision Profile of this component (Body Instance).

> **Warning: Performance Cost**
> While Greedy Meshing is efficient, regenerating physics bodies forces the physics engine to rebuild its acceleration structures.
> **Avoid setting `MinCollisionUpdateInterval` to 0.0.**
> If the smoke is purely cosmetic and does not need to block AI vision, set `bCollisionEnabled = false` to save significant CPU performance.

---

*Copyright (c) 2026, Team SDB. All rights reserved.*