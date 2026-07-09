# IVSmoke Quick Start Guide {#mainpage}

**IVSmoke** (Interactive Volumetric Smoke) is a real-time voxel-based volumetric smoke plugin for Unreal Engine 5.7. This guide outlines the installation process, basic usage via the provided Blueprint actor, and serves as a roadmap to the detailed documentation modules.

---

## 1. Installation

1.  Copy the `IVSmoke` folder into your project's `Plugins` directory (e.g., `MyProject/Plugins/`).
2.  Regenerate your project files (Right-click `.uproject` file → **Generate Visual Studio project files**).
3.  Open your project in Unreal Editor.
4.  Navigate to **Edit > Plugins** and ensure **IVSmoke** is enabled. Restart the editor if prompted.

> **Note**
> This plugin requires **Unreal Engine 5.7** and a Windows 64-bit platform supporting **DirectX 11.3+** (Shader Model 5).

<hr>

## 2. Basic Usage

The plugin includes a pre-configured Blueprint actor that contains all necessary components (`IVSmokeHoleGenerator`, `IVSmokeCollisionComponent`) for immediate use.

### Placing the Actor

1.  In the **Content Browser**, navigate to the **IVSmoke Content > Blueprints** folder.
    *(If you do not see the folder, enable "Show Plugin Content" in the Content Browser settings).*
2.  Drag and drop the `B_IVSmoke_Smoke` actor into your level.

### Simulation Controls

The actor provides two methods for viewing the smoke simulation:

**1. Play In Editor (PIE)**
Ensure the `bAutoPlay` flag is enabled in the Details panel. When you press **Play** or **Simulate**, the smoke will automatically cycle through its lifecycle (Expansion → Sustain → Dissipation).

**2. Editor Preview (No Play Required)**
To preview the simulation without entering Play mode:
1.  Select the `B_IVSmoke_Smoke` actor.
2.  In the Details panel, locate the **IVSmoke|Debug** category.
3.  Enable `bDebugEnabled`.
4.  Click the **Start Preview Simulation** button to visualize the smoke expansion directly in the viewport.

---

## 3. Documentation Roadmap

For detailed configuration and implementation guides, refer to the specific documents linked below.

### Core Concepts & Simulation Logic
Understand the voxel flood-fill algorithm, simulation phases (Expansion, Sustain, Dissipation), and network determinism.
* [Read the Voxel Volume Guide](../VoxelVolume/VoxelVolumeGuide.md)

### Gameplay Interaction (Holes & Effects)
Implement smoke interaction such as bullet penetration, grenade explosions, and dynamic object tracking using the `IVSmokeHoleGeneratorComponent`.
* [Read the Hole Generator Component Guide](../HoleGeneratorComponent/HoleGeneratorComponentGuide.md)

### Collision & AI Blocking
Configure the smoke to block AI Line of Sight (LoS) or create physical barriers using the `IVSmokeCollisionComponent`.
* [Read the Collision Component Guide](../CollisionComponent/CollisionComponentGuide.md)

### Visual Customization
Learn how to apply custom materials, manage translucency sorting order, and utilize Custom Depth for correct rendering with particles.
* [Read the Custom Material Guide](../CustomMaterial/CustomMaterialGuide.md)

### Global Settings & Optimization
Configure global quality levels (Ray Marching steps), shadow settings, and performance profiling.
* [Read the Global Settings Guide](../GlobalSettings/GlobalSettingsGuide.md)

---

## 4. API Reference

For specific class definitions, function parameters, and variable types, please refer to the auto-generated Doxygen **Class List** and **File List**.

* `AIVSmokeVoxelVolume`: The core C++ actor handling the voxel simulation.
* `UIVSmokeHoleGeneratorComponent`: Component responsible for managing smoke holes.
* `UIVSmokeCollisionComponent`: Component responsible for dynamic physics body generation.

---

## 5. Release Notes

### Version 1.0.1 (Current)

**Bug Fixes**
* **Smoke Penetration:** Fixed an issue where smoke particles would clip through walls or static geometry.
* **Packaging:** Fixed a shader compilation error that caused packaging failures in certain build configurations.

*Copyright (c) 2026, Team SDB. All rights reserved.*