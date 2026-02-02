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
    [ "Collision Component Guide", "md_pages_2_collision_component_2_collision_component_guide.html", [
      [ "Overview", "md_pages_2_collision_component_2_collision_component_guide.html#autotoc_md25", [
        [ "How It Works: Greedy Meshing To maintain high performance, the component <b>does not</b> create a collision box for every single voxel. Instead, it utilizes a <b>Binary Greedy Meshing</b> algorithm.", "md_pages_2_collision_component_2_collision_component_guide.html#autotoc_md26", null ]
      ] ],
      [ "Common Use Cases", "md_pages_2_collision_component_2_collision_component_guide.html#autotoc_md28", [
        [ "1. Blocking AI Line of Sight (Default)", "md_pages_2_collision_component_2_collision_component_guide.html#autotoc_md29", null ],
        [ "2. Physical Blockage", "md_pages_2_collision_component_2_collision_component_guide.html#autotoc_md30", null ]
      ] ],
      [ "Configuration", "md_pages_2_collision_component_2_collision_component_guide.html#autotoc_md32", [
        [ "Performance Throttling", "md_pages_2_collision_component_2_collision_component_guide.html#autotoc_md33", null ]
      ] ],
      [ "Debugging", "md_pages_2_collision_component_2_collision_component_guide.html#autotoc_md35", null ]
    ] ],
    [ "Custom Material Guide", "md_pages_2_custom_material_2_custom_material_guide.html", [
      [ "Material", "md_pages_2_custom_material_2_custom_material_guide.html#autotoc_md39", [
        [ "You must define how you want to mix the Scene and Smoke textures.", "md_pages_2_custom_material_2_custom_material_guide.html#autotoc_md40", null ],
        [ "Demo Example", "md_pages_2_custom_material_2_custom_material_guide.html#autotoc_md41", null ],
        [ "Expanssion Node", "md_pages_2_custom_material_2_custom_material_guide.html#autotoc_md42", null ]
      ] ],
      [ "Visual Material Preset", "md_pages_2_custom_material_2_custom_material_guide.html#autotoc_md44", [
        [ "Create", "md_pages_2_custom_material_2_custom_material_guide.html#autotoc_md45", null ],
        [ "Configuration", "md_pages_2_custom_material_2_custom_material_guide.html#autotoc_md46", null ]
      ] ],
      [ "Project Setting Visual Material Preset", "md_pages_2_custom_material_2_custom_material_guide.html#autotoc_md48", null ],
      [ "- <b>In the Rendering section, insert it into the Smoke Visual Material Preset slot.</b>", "md_pages_2_custom_material_2_custom_material_guide.html#autotoc_md49", null ],
      [ "Example", "md_pages_2_custom_material_2_custom_material_guide.html#autotoc_md50", null ]
    ] ],
    [ "Global Settings Guide", "md_pages_2_global_settings_2_global_settings_guide.html", [
      [ "General", "md_pages_2_global_settings_2_global_settings_guide.html#autotoc_md54", null ],
      [ "Quality", "md_pages_2_global_settings_2_global_settings_guide.html#autotoc_md56", null ],
      [ "Global Quality Presets", "md_pages_2_global_settings_2_global_settings_guide.html#autotoc_md57", null ],
      [ "Custom Quality Mode", "md_pages_2_global_settings_2_global_settings_guide.html#autotoc_md58", null ],
      [ "Ray March Quality", "md_pages_2_global_settings_2_global_settings_guide.html#autotoc_md59", null ],
      [ "Self-Shadow Quality", "md_pages_2_global_settings_2_global_settings_guide.html#autotoc_md60", null ],
      [ "External Shadow Quality", "md_pages_2_global_settings_2_global_settings_guide.html#autotoc_md61", null ],
      [ "Performance Impact", "md_pages_2_global_settings_2_global_settings_guide.html#autotoc_md62", null ],
      [ "Appearance", "md_pages_2_global_settings_2_global_settings_guide.html#autotoc_md64", null ],
      [ "Advanced Options (Show Advanced Options = true)", "md_pages_2_global_settings_2_global_settings_guide.html#autotoc_md65", null ],
      [ "Lighting", "md_pages_2_global_settings_2_global_settings_guide.html#autotoc_md67", null ],
      [ "Advanced Options (Show Advanced Options = true)", "md_pages_2_global_settings_2_global_settings_guide.html#autotoc_md68", null ],
      [ "Shadows", "md_pages_2_global_settings_2_global_settings_guide.html#autotoc_md70", null ],
      [ "Self Shadows", "md_pages_2_global_settings_2_global_settings_guide.html#autotoc_md71", null ],
      [ "External Shadows", "md_pages_2_global_settings_2_global_settings_guide.html#autotoc_md72", null ],
      [ "Post Processing", "md_pages_2_global_settings_2_global_settings_guide.html#autotoc_md74", null ],
      [ "Rendering", "md_pages_2_global_settings_2_global_settings_guide.html#autotoc_md76", null ],
      [ "Advanced Options (Show Advanced Options = true)", "md_pages_2_global_settings_2_global_settings_guide.html#autotoc_md77", null ],
      [ "Depth Write System", "md_pages_2_global_settings_2_global_settings_guide.html#autotoc_md78", null ],
      [ "Debug", "md_pages_2_global_settings_2_global_settings_guide.html#autotoc_md80", null ],
      [ "Smoke Preset", "md_pages_2_global_settings_2_global_settings_guide.html#autotoc_md82", [
        [ "Creating a Smoke Preset", "md_pages_2_global_settings_2_global_settings_guide.html#autotoc_md83", null ],
        [ "Preset Properties", "md_pages_2_global_settings_2_global_settings_guide.html#autotoc_md84", null ],
        [ "Applying a Preset", "md_pages_2_global_settings_2_global_settings_guide.html#autotoc_md85", null ],
        [ "Preset Examples", "md_pages_2_global_settings_2_global_settings_guide.html#autotoc_md86", null ]
      ] ],
      [ "Translucent Sorting", "md_pages_2_global_settings_2_global_settings_guide.html#autotoc_md88", [
        [ "How It Works", "md_pages_2_global_settings_2_global_settings_guide.html#autotoc_md89", null ],
        [ "Automatic Sorting", "md_pages_2_global_settings_2_global_settings_guide.html#autotoc_md90", null ],
        [ "Troubleshooting", "md_pages_2_global_settings_2_global_settings_guide.html#autotoc_md91", null ],
        [ "Performance", "md_pages_2_global_settings_2_global_settings_guide.html#autotoc_md92", null ]
      ] ],
      [ "Editor Preview", "md_pages_2_global_settings_2_global_settings_guide.html#autotoc_md94", [
        [ "Starting Preview", "md_pages_2_global_settings_2_global_settings_guide.html#autotoc_md95", null ],
        [ "Stopping Preview", "md_pages_2_global_settings_2_global_settings_guide.html#autotoc_md96", null ],
        [ "Debug Visualization Options", "md_pages_2_global_settings_2_global_settings_guide.html#autotoc_md97", null ],
        [ "Preview Workflow Tips", "md_pages_2_global_settings_2_global_settings_guide.html#autotoc_md98", null ],
        [ "Limitations", "md_pages_2_global_settings_2_global_settings_guide.html#autotoc_md99", null ]
      ] ],
      [ "Stat IVSmoke", "md_pages_2_global_settings_2_global_settings_guide.html#autotoc_md101", [
        [ "Enabling Stats", "md_pages_2_global_settings_2_global_settings_guide.html#autotoc_md102", null ],
        [ "Available Statistics", "md_pages_2_global_settings_2_global_settings_guide.html#autotoc_md103", null ],
        [ "Memory Stats (GPU VRAM)", "md_pages_2_global_settings_2_global_settings_guide.html#autotoc_md104", null ],
        [ "Cycle Stats (CPU/GPU Time)", "md_pages_2_global_settings_2_global_settings_guide.html#autotoc_md105", null ],
        [ "Counter Stats", "md_pages_2_global_settings_2_global_settings_guide.html#autotoc_md106", null ],
        [ "Using with Unreal Insights", "md_pages_2_global_settings_2_global_settings_guide.html#autotoc_md107", null ]
      ] ],
      [ "Best Practices", "md_pages_2_global_settings_2_global_settings_guide.html#autotoc_md109", [
        [ "Recommended Level Placement", "md_pages_2_global_settings_2_global_settings_guide.html#autotoc_md110", null ],
        [ "Spacing and Distribution", "md_pages_2_global_settings_2_global_settings_guide.html#autotoc_md111", null ],
        [ "Volume Sizing", "md_pages_2_global_settings_2_global_settings_guide.html#autotoc_md112", null ],
        [ "Performance Tips", "md_pages_2_global_settings_2_global_settings_guide.html#autotoc_md113", null ]
      ] ]
    ] ],
    [ "HoleGenerator Component Guide", "md_pages_2_hole_generator_component_2_hole_generator_component_guide.html", [
      [ "Explosion (Grenade)", "md_pages_2_hole_generator_component_2_hole_generator_component_guide.html#autotoc_md116", null ],
      [ "Penetration (Bullet)", "md_pages_2_hole_generator_component_2_hole_generator_component_guide.html#autotoc_md118", null ],
      [ "Create", "md_pages_2_hole_generator_component_2_hole_generator_component_guide.html#autotoc_md121", null ],
      [ "Usage", "md_pages_2_hole_generator_component_2_hole_generator_component_guide.html#autotoc_md123", [
        [ "<b>Select the Right Interaction Node</b>", "md_pages_2_hole_generator_component_2_hole_generator_component_guide.html#autotoc_md124", null ],
        [ "Before STARTING, Add UIVSmokeHoleRequestComponent!", "md_pages_2_hole_generator_component_2_hole_generator_component_guide.html#autotoc_md125", null ],
        [ "Case A. Request Penetration Hole", "md_pages_2_hole_generator_component_2_hole_generator_component_guide.html#autotoc_md126", null ],
        [ "<b>Execution Timing</b>", "md_pages_2_hole_generator_component_2_hole_generator_component_guide.html#autotoc_md127", null ],
        [ "Case B. Request Explosion Hole", "md_pages_2_hole_generator_component_2_hole_generator_component_guide.html#autotoc_md128", null ],
        [ "<b>Execution Timing</b>", "md_pages_2_hole_generator_component_2_hole_generator_component_guide.html#autotoc_md129", null ],
        [ "Case C. Request Dynamic Hole", "md_pages_2_hole_generator_component_2_hole_generator_component_guide.html#autotoc_md130", null ],
        [ "<b>Execution Timing</b>", "md_pages_2_hole_generator_component_2_hole_generator_component_guide.html#autotoc_md131", null ]
      ] ]
    ] ],
    [ "Voxel Volume Guide", "md_pages_2_voxel_volume_2_voxel_volume_guide.html", [
      [ "Core Concepts", "md_pages_2_voxel_volume_2_voxel_volume_guide.html#autotoc_md134", [
        [ "Volumetric Flood-Fill", "md_pages_2_voxel_volume_2_voxel_volume_guide.html#autotoc_md135", null ],
        [ "Deterministic Networking", "md_pages_2_voxel_volume_2_voxel_volume_guide.html#autotoc_md136", null ]
      ] ],
      [ "Simulation Lifecycle", "md_pages_2_voxel_volume_2_voxel_volume_guide.html#autotoc_md138", [
        [ "1. Expansion Phase", "md_pages_2_voxel_volume_2_voxel_volume_guide.html#autotoc_md139", null ],
        [ "2. Sustain Phase", "md_pages_2_voxel_volume_2_voxel_volume_guide.html#autotoc_md140", null ],
        [ "3. Dissipation Phase", "md_pages_2_voxel_volume_2_voxel_volume_guide.html#autotoc_md141", null ],
        [ "4. Finished", "md_pages_2_voxel_volume_2_voxel_volume_guide.html#autotoc_md142", null ]
      ] ],
      [ "Configuration Guidelines", "md_pages_2_voxel_volume_2_voxel_volume_guide.html#autotoc_md144", [
        [ "Performance &amp; Resolution", "md_pages_2_voxel_volume_2_voxel_volume_guide.html#autotoc_md145", null ],
        [ "Shape Control", "md_pages_2_voxel_volume_2_voxel_volume_guide.html#autotoc_md146", null ],
        [ "Radii (Aspect Ratio)", "md_pages_2_voxel_volume_2_voxel_volume_guide.html#autotoc_md147", null ],
        [ "Expansion Noise", "md_pages_2_voxel_volume_2_voxel_volume_guide.html#autotoc_md148", null ]
      ] ],
      [ "Network Synchronization", "md_pages_2_voxel_volume_2_voxel_volume_guide.html#autotoc_md150", [
        [ "Late-Join Handling (Fast Forwarding)", "md_pages_2_voxel_volume_2_voxel_volume_guide.html#autotoc_md151", null ],
        [ "Time Synchronization", "md_pages_2_voxel_volume_2_voxel_volume_guide.html#autotoc_md152", null ]
      ] ],
      [ "Interaction Components", "md_pages_2_voxel_volume_2_voxel_volume_guide.html#autotoc_md154", null ],
      [ "Debugging", "md_pages_2_voxel_volume_2_voxel_volume_guide.html#autotoc_md156", [
        [ "Console Commands (CVars)", "md_pages_2_voxel_volume_2_voxel_volume_guide.html#autotoc_md158", null ]
      ] ]
    ] ],
    [ "Classes", "annotated.html", [
      [ "Class List", "annotated.html", "annotated_dup" ],
      [ "Class Index", "classes.html", null ],
      [ "Class Hierarchy", "hierarchy.html", "hierarchy" ],
      [ "Class Members", "functions.html", [
        [ "All", "functions.html", "functions_dup" ],
        [ "Functions", "functions_func.html", null ],
        [ "Variables", "functions_vars.html", null ]
      ] ]
    ] ],
    [ "Files", "files.html", [
      [ "File List", "files.html", "files_dup" ]
    ] ]
  ] ]
];

var NAVTREEINDEX =
[
"_i_v_smoke_8_build_8cs_source.html",
"files.html",
"struct_f_i_v_smoke_volume_g_p_u_data.html#abcaaa586b72b4d4ceaebe13421b7a0c5"
];

var SYNCONMSG = 'click to disable panel synchronization';
var SYNCOFFMSG = 'click to enable panel synchronization';
var LISTOFALLMEMBERS = 'List of all members';