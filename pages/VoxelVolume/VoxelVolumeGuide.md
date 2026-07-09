# Voxel Volume Guide

`AIVSmokeVoxelVolume` is the central actor of the IVSmoke plugin. It is responsible for the simulation, management, and replication of physically aware volumetric smoke.

Unlike traditional particle-based smoke, this actor utilizes a **voxel-based deterministic flood-fill algorithm**. This allows the smoke to naturally expand into spaces, flow around corners, and be blocked by walls, creating high-fidelity tactical concealment without clipping through geometry.

## Core Concepts

### Volumetric Flood-Fill

The volume generates a 3D grid of voxels. Starting from the center, the smoke performs a Lazy Theta*-based pathfinding expansion.

* **Obstacle Awareness:** Before a voxel is spawned, the simulation performs collision checks (`VoxelCollisionChannel`) against the world geometry.
* **Volumetric Flow:** If a direct path is blocked by a wall, the smoke will "flow" around the obstacle to fill the space behind it, provided there is an open path.
* **Natural Shape:** By applying `ExpansionNoise` to the pathfinding cost, the smoke creates irregular, organic shapes rather than perfect spheres.

### Deterministic Networking

To maintain high performance and low bandwidth usage, **voxel data is never replicated**.

Instead, the simulation is fully deterministic:

1. **Server Authority:** The server selects a `RandomSeed` and a synchronized `StartTime`.
2. **Replication:** Only the simulation *state* (Seed, Time, Phase) is sent to clients.
3. **Local Simulation:** Clients run the exact same flood-fill algorithm locally using the replicated seed. This ensures every client sees the exact same smoke shape without sending array data across the network.

> **CRITICAL: Network Determinism & Collision Channels**
>
> The simulation is **deterministic**, meaning clients calculate the smoke shape locally to save bandwidth. However, they rely on the world geometry being identical to the server.
>
> If you set the `VoxelCollisionChannel` to detect **Dynamic Objects** (e.g., destructible walls, moving doors, or vehicles), you **MUST** ensure those objects are perfectly synchronized (replicated) across the network.
>
> * **Risk:** If a door is open on the Server but closed on a Client (due to lag or lack of replication), the smoke will flow through the door on the Server but be blocked on the Client.
> * **Result:** This causes a "Desync," where players see smoke in different locations, breaking competitive integrity.
> * **Recommendation:** It is safest to use `ECC_WorldStatic` or a custom channel dedicated to static level geometry.

<hr>

## Simulation Lifecycle

The actor operates on a state machine (`EIVSmokeVoxelVolumeState`) that progresses based on duration settings.

### 1. Expansion Phase

* **Behavior:** The smoke actively spawns new voxels, expanding outward from the center.
* **Control:** The rate of expansion is controlled by the `ExpansionCurve` and `ExpansionDuration`.
* **Visuals:** Voxels fade in based on `FadeInDuration`.

### 2. Sustain Phase

* **Behavior:** The smoke has reached its maximum size (or `MaxVoxelNum` limit). It maintains its shape and density.
* **Duration:** Controlled by `SustainDuration`. If `bIsInfinite` is true, it remains in this state indefinitely.

### 3. Dissipation Phase

* **Behavior:** The smoke begins to recede. Voxels are removed effectively in the reverse order of their generation (or based on noise), simulating the smoke thinning out.
* **Control:** The rate of removal is controlled by `DissipationCurve` and `DissipationDuration`.

### 4. Finished

* **Behavior:** All voxels are removed.
* **Cleanup:** If `bDestroyOnFinish` is enabled, the actor is destroyed. Otherwise, it resets to an Idle state.

---

## Configuration Guidelines

### Performance & Resolution

The visual fidelity and performance cost are managed by balancing the physical size of the voxels against the total number of voxels allowed.

| **Property** | **Description** | **Impact** |
| --- | --- | --- |
| **Voxel Size** | The physical scale of a single voxel (in cm). | **Visual Resolution.** Changing this *does not* affect performance (CPU/GPU) directly; it simply scales the resulting mesh.<br>• **Large Value:** Covers a large area cheaply, but looks "blocky."<br>• **Small Value:** Finer detail. To cover a large area with small voxels, you must increase `MaxVoxelNum`. |
| **Max Voxel Num** | The hard limit on the number of active voxels. | **Performance Budget.** This is the primary factor for CPU simulation cost. |
| **Volume Extent** | The grid index radius. | **Memory Usage.** Defines the maximum memory buffer size. Keep this optimized (e.g., 8-16) to minimize memory footprint. |

### Shape Control

The overall shape of the smoke cloud is defined by the **Radii** and **Noise** settings.

### Radii (Aspect Ratio)

`Radii` defines the **relative shape ratio** of the expansion per axis. These are not absolute units; they function as weights for the flood-fill propagation.

**Examples:**

* `(1.0, 1.0, 1.0)` : Creates a **Sphere**.
* `(2.0, 2.0, 2.0)` : Also creates a **Sphere** (since the ratios are identical).
* `(2.0, 1.0, 1.0)` : Creates an **Ellipsoid** that stretches twice as far along the X-axis compared to Y and Z.
* `(1.0, 1.0, 0.5)` : Creates a flattened "pancake" shape.



### Expansion Noise

Adds randomness to the pathfinding cost.

* **Low Value:** The smoke expands in a smooth, predictable geometric shape.
* **High Value:** The smoke becomes jagged and irregular, seeping into cracks and forming a more organic cloud.

---

## Network Synchronization

### Late-Join Handling (Fast Forwarding)

A common challenge with deterministic simulations is handling clients who join the game after the smoke has already started.

`AIVSmokeVoxelVolume` handles this automatically:

* When a client receives the state, it checks the server's `Generation` count.
* If the client is behind, it triggers a **Fast-Forward** routine.
* The client internally simulates the elapsed time in a tight loop to catch up to the server's current state instantly.

### Time Synchronization

The simulation relies on `GetSyncWorldTimeSeconds()` to ensure that the expansion animation plays at the exact same world-time timestamp on all clients, compensating for network latency.

---

## Interaction Components

This actor coordinates with helper components to handle gameplay interactions:

1. **Hole Generator** (`UIVSmokeHoleGeneratorComponent`):
    * Handles dynamic modifications to the smoke texture (e.g., when a grenade explodes inside the smoke or a projectile passes through).
    * This is purely visual and does not alter the voxel grid topology.
2. **Collision Component** (`UIVSmokeCollisionComponent`):
    * Generates physics collision bodies based on the active voxels.
    * **Usage:** Primarily used for **AI Perception blocking**. It allows AI to understand that they cannot see through the smoke volume.

> **Interaction vs. Simulation**
>
> Do not confuse `VoxelCollisionChannel` (Settings) with `UIVSmokeCollisionComponent` (Component).
>
> * **VoxelCollisionChannel:** Used by the *simulation* to detect walls and stop expansion.
> * **UIVSmokeCollisionComponent:** Created by the *simulation* to block other actors (like AI Line of Sight).

<hr>

## Debugging

The class includes robust editor and runtime debugging tools located under `Debug Settings`.

* **Preview Simulation:** In the Editor Details panel, use `StartPreviewSimulation` to visualize the smoke expansion without entering Play Mode.
* **Visualizers:**
    * **Wireframe:** Draws the bounds of active voxels.
    * **Heatmap:** Colors voxels based on their generation order (useful for debugging expansion logic).
    * **Status Text:** Displays the current State, Active Voxel Count, and the deterministic Checksum.

> **Note on Checksums:** The debug text displays a CRC32 Checksum. If you suspect a desync issue, compare this number between the Server and Client. They must match exactly.

<hr>

### Console Commands (CVars)

You can control debug visualizations at runtime using the console (`~` key). These commands affect **all** active `AIVSmokeVoxelVolume` actors in the world simultaneously.

All commands share the prefix `IVSmoke.Volume.`

| Command | Arguments | Description |
| --- | --- | --- |
| `IVSmoke.Volume.Debug` | `0` or `1` | **Master Switch.** Toggles the entire debug system on or off. Setting this to 0 hides all visualizations immediately. |
| `IVSmoke.Volume.ShowWireframe` | `0` or `1` | Toggles the lightweight wireframe cube visualization for active voxels. |
| `IVSmoke.Volume.ShowMesh` | `0` or `1` | Toggles the Instanced Static Mesh visualization. **Warning:** Rendering thousands of meshes can be expensive. |
| `IVSmoke.Volume.ShowStatus` | `0` or `1` | Toggles the floating status text (State, Voxel Count, Checksum) above the actor. |
| `IVSmoke.Volume.SetViewMode` | `Mode ID` | Changes visualization color. Use `0` for Solid Color (default) or `1` for Heatmap (gradient based on generation order). |

---

*Copyright (c) 2026, Team SDB. All rights reserved.*