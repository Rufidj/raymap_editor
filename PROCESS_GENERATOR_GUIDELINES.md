# Raymap Editor Code Generation (ProcessGenerator)

This document outlines the rules and best practices for maintaining and extending the `ProcessGenerator` in the Raymap Editor for BennuGD2.

## 1. Strict Language Separation (C++ vs. BennuGD)
The generator is written in C++ (Qt) and outputs BennuGD source code.
- **NEVER** leak C++ instructions into the output stream. 
- **BAD**: `out << "    out << \" // model_id = ...\";\n";` (Writes C++ code into the .prg)
- **GOOD**: `out << "    model_id = RAY_LOAD_MD3(...);\n";` (Writes BennuGD code into the .prg)

## 2. BennuGD Syntax Standards
- **Precise Types**: Use `double` for coordinates, angles, and speeds.
- **Block Integrity**: Every `begin`, `if`, `while`, `loop`, and `process` **MUST** have a corresponding `end`. Missing `end` tags will cause the BennuGD compiler to report "Unknown identifier PROCESS" for subsequent processes.
- **Initialization & Hooks**: Always generate calls to user hooks:
    - `hook_[processname]_init(id)`
    - `hook_[processname]_update(id)`
  This allows users to extend auto-generated entities in separate files without their changes being overwritten.

## 3. Experimental / "Ray Plus" Features
When adding support for new modules, ensure the following functions are utilized:
- **3D Models**: `RAY_LOAD_MD3(path)` and `RAY_SET_SPRITE_MD2(sprite_id, model_id, texture_id)`.
- **Camera Paths**: 
    - Loading: `RAY_CAMERA_LOAD(path)`
    - Playback: `RAY_CAMERA_PLAY(path_id)`
    - Update: `RAY_CAMERA_PATH_UPDATE(0.0166)` inside loops.
- **Advanced Collision**: `RAY_CHECK_COLLISION_Z(x1, y1, z1, x2, y2)` for 3D sliding collision.

## 4. Resource Path Management
- Use relative paths whenever possible (starting with `assets/`).
- The generator should handle both absolute paths from the editor's file picker and relative paths for runtime portability.

## 5. Sprite Lifecycle
For entities that use raycaster sprites (like models or decals):
- **Create**: `RAY_ADD_SPRITE(x, y, z, ...)`
- **Update**: `RAY_UPDATE_SPRITE_POSITION(sprite_id, x, y, z)`
- **Delete**: `RAY_REMOVE_SPRITE(sprite_id)` in the cleanup section (after the main loop) to prevent memory leaks in the raycasting engine.
