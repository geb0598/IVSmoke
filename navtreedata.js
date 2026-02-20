/*
 @licstart  The following is the entire license notice for the JavaScript code in this file.

 The MIT License (MIT)

 Copyright (C) 1997-2020 by Dimitri van Heesch

 Permission is hereby granted, free of charge, to any person obtaining a copy of this software
 and associated documentation files (the "Software"), to deal in the Software without restriction,
 including without limitation the rights to use, copy, modify, merge, publish, distribute,
 sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all copies or
 substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
 BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 @licend  The above is the entire license notice for the JavaScript code in this file
*/
var NAVTREE =
[
  [ "IVSmoke", "index.html", [
    [ "IVSmoke Quick Start Guide", "index.html", "index" ],
    [ "Collision Component Guide", "md_pages_2CollisionComponent_2CollisionComponentGuide.html", [
      [ "Overview", "md_pages_2CollisionComponent_2CollisionComponentGuide.html#autotoc_md27", [
        [ "How It Works: Greedy Meshing To maintain high performance, the component <strong>does not</strong> create a collision box for every single voxel. Instead, it utilizes a <strong>Binary Greedy Meshing</strong> algorithm.", "md_pages_2CollisionComponent_2CollisionComponentGuide.html#autotoc_md28", null ]
      ] ],
      [ "Common Use Cases", "md_pages_2CollisionComponent_2CollisionComponentGuide.html#autotoc_md30", [
        [ "1. Blocking AI Line of Sight (Default)", "md_pages_2CollisionComponent_2CollisionComponentGuide.html#autotoc_md31", null ],
        [ "2. Physical Blockage", "md_pages_2CollisionComponent_2CollisionComponentGuide.html#autotoc_md32", null ]
      ] ],
      [ "Configuration", "md_pages_2CollisionComponent_2CollisionComponentGuide.html#autotoc_md34", [
        [ "Performance Throttling", "md_pages_2CollisionComponent_2CollisionComponentGuide.html#autotoc_md35", null ]
      ] ],
      [ "Debugging", "md_pages_2CollisionComponent_2CollisionComponentGuide.html#autotoc_md37", null ],
      [ "</blockquote>", "md_pages_2CollisionComponent_2CollisionComponentGuide.html#autotoc_md39", null ]
    ] ],
    [ "Custom Material Guide", "md_pages_2CustomMaterial_2CustomMaterialGuide.html", [
      [ "Material", "md_pages_2CustomMaterial_2CustomMaterialGuide.html#autotoc_md41", [
        [ "You must define how you want to mix the Scene and Smoke textures.", "md_pages_2CustomMaterial_2CustomMaterialGuide.html#autotoc_md42", null ],
        [ "Demo Example", "md_pages_2CustomMaterial_2CustomMaterialGuide.html#autotoc_md43", null ],
        [ "Expanssion Node", "md_pages_2CustomMaterial_2CustomMaterialGuide.html#autotoc_md44", null ]
      ] ],
      [ "Visual Material Preset", "md_pages_2CustomMaterial_2CustomMaterialGuide.html#autotoc_md46", [
        [ "Create", "md_pages_2CustomMaterial_2CustomMaterialGuide.html#autotoc_md47", null ],
        [ "Configuration", "md_pages_2CustomMaterial_2CustomMaterialGuide.html#autotoc_md48", null ]
      ] ],
      [ "Project Setting Visual Material Preset", "md_pages_2CustomMaterial_2CustomMaterialGuide.html#autotoc_md50", null ],
      [ "- <strong>In the Rendering section, insert it into the Smoke Visual Material Preset slot.</strong>", "md_pages_2CustomMaterial_2CustomMaterialGuide.html#autotoc_md51", null ],
      [ "Example", "md_pages_2CustomMaterial_2CustomMaterialGuide.html#autotoc_md52", null ]
    ] ],
    [ "Global Settings Guide", "md_pages_2GlobalSettings_2GlobalSettingsGuide.html", [
      [ "General", "md_pages_2GlobalSettings_2GlobalSettingsGuide.html#autotoc_md56", null ],
      [ "Quality", "md_pages_2GlobalSettings_2GlobalSettingsGuide.html#autotoc_md58", null ],
      [ "Global Quality Presets", "md_pages_2GlobalSettings_2GlobalSettingsGuide.html#autotoc_md59", null ],
      [ "Custom Quality Mode", "md_pages_2GlobalSettings_2GlobalSettingsGuide.html#autotoc_md60", null ],
      [ "Ray March Quality", "md_pages_2GlobalSettings_2GlobalSettingsGuide.html#autotoc_md61", null ],
      [ "Self-Shadow Quality", "md_pages_2GlobalSettings_2GlobalSettingsGuide.html#autotoc_md62", null ],
      [ "External Shadow Quality", "md_pages_2GlobalSettings_2GlobalSettingsGuide.html#autotoc_md63", null ],
      [ "Performance Impact", "md_pages_2GlobalSettings_2GlobalSettingsGuide.html#autotoc_md64", null ],
      [ "Appearance", "md_pages_2GlobalSettings_2GlobalSettingsGuide.html#autotoc_md66", null ],
      [ "Advanced Options (Show Advanced Options = true)", "md_pages_2GlobalSettings_2GlobalSettingsGuide.html#autotoc_md67", null ],
      [ "Lighting", "md_pages_2GlobalSettings_2GlobalSettingsGuide.html#autotoc_md69", null ],
      [ "Advanced Options (Show Advanced Options = true)", "md_pages_2GlobalSettings_2GlobalSettingsGuide.html#autotoc_md70", null ],
      [ "Shadows", "md_pages_2GlobalSettings_2GlobalSettingsGuide.html#autotoc_md72", null ],
      [ "Self Shadows", "md_pages_2GlobalSettings_2GlobalSettingsGuide.html#autotoc_md73", null ],
      [ "External Shadows", "md_pages_2GlobalSettings_2GlobalSettingsGuide.html#autotoc_md74", null ],
      [ "Post Processing", "md_pages_2GlobalSettings_2GlobalSettingsGuide.html#autotoc_md76", null ],
      [ "Rendering", "md_pages_2GlobalSettings_2GlobalSettingsGuide.html#autotoc_md78", null ],
      [ "Advanced Options (Show Advanced Options = true)", "md_pages_2GlobalSettings_2GlobalSettingsGuide.html#autotoc_md79", null ],
      [ "Depth Write System", "md_pages_2GlobalSettings_2GlobalSettingsGuide.html#autotoc_md80", null ],
      [ "Debug", "md_pages_2GlobalSettings_2GlobalSettingsGuide.html#autotoc_md82", null ],
      [ "Smoke Preset", "md_pages_2GlobalSettings_2GlobalSettingsGuide.html#autotoc_md84", [
        [ "Creating a Smoke Preset", "md_pages_2GlobalSettings_2GlobalSettingsGuide.html#autotoc_md85", null ],
        [ "Preset Properties", "md_pages_2GlobalSettings_2GlobalSettingsGuide.html#autotoc_md86", null ],
        [ "Applying a Preset", "md_pages_2GlobalSettings_2GlobalSettingsGuide.html#autotoc_md87", null ],
        [ "Preset Examples", "md_pages_2GlobalSettings_2GlobalSettingsGuide.html#autotoc_md88", null ]
      ] ],
      [ "Translucent Sorting", "md_pages_2GlobalSettings_2GlobalSettingsGuide.html#autotoc_md90", [
        [ "How It Works", "md_pages_2GlobalSettings_2GlobalSettingsGuide.html#autotoc_md91", null ],
        [ "Automatic Sorting", "md_pages_2GlobalSettings_2GlobalSettingsGuide.html#autotoc_md92", null ],
        [ "Troubleshooting", "md_pages_2GlobalSettings_2GlobalSettingsGuide.html#autotoc_md93", null ],
        [ "Performance", "md_pages_2GlobalSettings_2GlobalSettingsGuide.html#autotoc_md94", null ]
      ] ],
      [ "Editor Preview", "md_pages_2GlobalSettings_2GlobalSettingsGuide.html#autotoc_md96", [
        [ "Starting Preview", "md_pages_2GlobalSettings_2GlobalSettingsGuide.html#autotoc_md97", null ],
        [ "Stopping Preview", "md_pages_2GlobalSettings_2GlobalSettingsGuide.html#autotoc_md98", null ],
        [ "Debug Visualization Options", "md_pages_2GlobalSettings_2GlobalSettingsGuide.html#autotoc_md99", null ],
        [ "Preview Workflow Tips", "md_pages_2GlobalSettings_2GlobalSettingsGuide.html#autotoc_md100", null ],
        [ "Limitations", "md_pages_2GlobalSettings_2GlobalSettingsGuide.html#autotoc_md101", null ]
      ] ],
      [ "Stat IVSmoke", "md_pages_2GlobalSettings_2GlobalSettingsGuide.html#autotoc_md103", [
        [ "Enabling Stats", "md_pages_2GlobalSettings_2GlobalSettingsGuide.html#autotoc_md104", null ],
        [ "Available Statistics", "md_pages_2GlobalSettings_2GlobalSettingsGuide.html#autotoc_md105", null ],
        [ "Memory Stats (GPU VRAM)", "md_pages_2GlobalSettings_2GlobalSettingsGuide.html#autotoc_md106", null ],
        [ "Cycle Stats (CPU/GPU Time)", "md_pages_2GlobalSettings_2GlobalSettingsGuide.html#autotoc_md107", null ],
        [ "Counter Stats", "md_pages_2GlobalSettings_2GlobalSettingsGuide.html#autotoc_md108", null ],
        [ "Using with Unreal Insights", "md_pages_2GlobalSettings_2GlobalSettingsGuide.html#autotoc_md109", null ]
      ] ],
      [ "Best Practices", "md_pages_2GlobalSettings_2GlobalSettingsGuide.html#autotoc_md111", [
        [ "Recommended Level Placement", "md_pages_2GlobalSettings_2GlobalSettingsGuide.html#autotoc_md112", null ],
        [ "Spacing and Distribution", "md_pages_2GlobalSettings_2GlobalSettingsGuide.html#autotoc_md113", null ],
        [ "Volume Sizing", "md_pages_2GlobalSettings_2GlobalSettingsGuide.html#autotoc_md114", null ],
        [ "Performance Tips", "md_pages_2GlobalSettings_2GlobalSettingsGuide.html#autotoc_md115", null ]
      ] ]
    ] ],
    [ "HoleGenerator Component Guide", "md_pages_2HoleGeneratorComponent_2HoleGeneratorComponentGuide.html", [
      [ "Explosion (Grenade)", "md_pages_2HoleGeneratorComponent_2HoleGeneratorComponentGuide.html#autotoc_md118", null ],
      [ "Penetration (Bullet)", "md_pages_2HoleGeneratorComponent_2HoleGeneratorComponentGuide.html#autotoc_md120", null ],
      [ "Create", "md_pages_2HoleGeneratorComponent_2HoleGeneratorComponentGuide.html#autotoc_md123", null ],
      [ "Usage", "md_pages_2HoleGeneratorComponent_2HoleGeneratorComponentGuide.html#autotoc_md125", [
        [ "<strong>Select the Right Interaction Node</strong>", "md_pages_2HoleGeneratorComponent_2HoleGeneratorComponentGuide.html#autotoc_md126", null ],
        [ "Before STARTING, Add UIVSmokeHoleRequestComponent!", "md_pages_2HoleGeneratorComponent_2HoleGeneratorComponentGuide.html#autotoc_md127", null ],
        [ "Case A. Request Penetration Hole", "md_pages_2HoleGeneratorComponent_2HoleGeneratorComponentGuide.html#autotoc_md128", null ],
        [ "<strong>Execution Timing</strong>", "md_pages_2HoleGeneratorComponent_2HoleGeneratorComponentGuide.html#autotoc_md129", null ],
        [ "Case B. Request Explosion Hole", "md_pages_2HoleGeneratorComponent_2HoleGeneratorComponentGuide.html#autotoc_md130", null ],
        [ "<strong>Execution Timing</strong>", "md_pages_2HoleGeneratorComponent_2HoleGeneratorComponentGuide.html#autotoc_md131", null ],
        [ "Case C. Request Dynamic Hole", "md_pages_2HoleGeneratorComponent_2HoleGeneratorComponentGuide.html#autotoc_md132", null ],
        [ "<strong>Execution Timing</strong>", "md_pages_2HoleGeneratorComponent_2HoleGeneratorComponentGuide.html#autotoc_md133", null ]
      ] ]
    ] ],
    [ "Voxel Volume Guide", "md_pages_2VoxelVolume_2VoxelVolumeGuide.html", [
      [ "Core Concepts", "md_pages_2VoxelVolume_2VoxelVolumeGuide.html#autotoc_md136", [
        [ "Volumetric Flood-Fill", "md_pages_2VoxelVolume_2VoxelVolumeGuide.html#autotoc_md137", null ],
        [ "Deterministic Networking", "md_pages_2VoxelVolume_2VoxelVolumeGuide.html#autotoc_md138", null ]
      ] ],
      [ "Simulation Lifecycle", "md_pages_2VoxelVolume_2VoxelVolumeGuide.html#autotoc_md140", [
        [ "1. Expansion Phase", "md_pages_2VoxelVolume_2VoxelVolumeGuide.html#autotoc_md141", null ],
        [ "2. Sustain Phase", "md_pages_2VoxelVolume_2VoxelVolumeGuide.html#autotoc_md142", null ],
        [ "3. Dissipation Phase", "md_pages_2VoxelVolume_2VoxelVolumeGuide.html#autotoc_md143", null ],
        [ "4. Finished", "md_pages_2VoxelVolume_2VoxelVolumeGuide.html#autotoc_md144", null ]
      ] ],
      [ "Configuration Guidelines", "md_pages_2VoxelVolume_2VoxelVolumeGuide.html#autotoc_md146", [
        [ "Performance & Resolution", "md_pages_2VoxelVolume_2VoxelVolumeGuide.html#autotoc_md147", null ],
        [ "Shape Control", "md_pages_2VoxelVolume_2VoxelVolumeGuide.html#autotoc_md148", null ],
        [ "Radii (Aspect Ratio)", "md_pages_2VoxelVolume_2VoxelVolumeGuide.html#autotoc_md149", null ],
        [ "Expansion Noise", "md_pages_2VoxelVolume_2VoxelVolumeGuide.html#autotoc_md150", null ]
      ] ],
      [ "Network Synchronization", "md_pages_2VoxelVolume_2VoxelVolumeGuide.html#autotoc_md152", [
        [ "Late-Join Handling (Fast Forwarding)", "md_pages_2VoxelVolume_2VoxelVolumeGuide.html#autotoc_md153", null ],
        [ "Time Synchronization", "md_pages_2VoxelVolume_2VoxelVolumeGuide.html#autotoc_md154", null ]
      ] ],
      [ "Interaction Components", "md_pages_2VoxelVolume_2VoxelVolumeGuide.html#autotoc_md156", null ],
      [ "Debugging", "md_pages_2VoxelVolume_2VoxelVolumeGuide.html#autotoc_md158", null ],
      [ "</blockquote>", "md_pages_2VoxelVolume_2VoxelVolumeGuide.html#autotoc_md159", [
        [ "Console Commands (CVars)", "md_pages_2VoxelVolume_2VoxelVolumeGuide.html#autotoc_md160", null ]
      ] ]
    ] ],
    [ "Classes", "annotated.html", [
      [ "Class List", "annotated.html", "annotated_dup" ],
      [ "Class Index", "classes.html", null ],
      [ "Class Hierarchy", "hierarchy.html", "hierarchy" ],
      [ "Class Members", "functions.html", [
        [ "All", "functions.html", "functions_dup" ],
        [ "Functions", "functions_func.html", null ],
        [ "Variables", "functions_vars.html", "functions_vars" ]
      ] ]
    ] ],
    [ "Files", "files.html", [
      [ "File List", "files.html", "files_dup" ]
    ] ]
  ] ]
];

var NAVTREEINDEX =
[
"IVSmokeCSMRenderer_8cpp_source.html",
"classUMaterialExpressionIVSmoke__TextureSample.html#a30dcdbd8e8dc69aad06eb0b2a9d58861",
"structFIVSmokeOccupancyResources.html#aed5fac0f3db2e59f8ea9889581c6e5eb"
];

var SYNCONMSG = 'click to disable panel synchronisation';
var SYNCOFFMSG = 'click to enable panel synchronisation';