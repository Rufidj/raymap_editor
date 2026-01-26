# Editor RayMap

![RayMap Editor Screenshot](assets/icon.png)

**RayMap Editor** es un avanzado editor de mapas y niveles 3D diseñado para el motor BennuGD2 (específicamente `mod_ray`). Permite crear mapas basados en sectores geométricos, similar a los juegos clásicos del motor Build (Duke Nukem 3D, Blood), pero con herramientas y flujo de trabajo modernos.

---

## 🇪🇸 Características Principales

### 1. Edición de Sectores Geométricos
*   **Edición en Rejilla**: Dibuja sectores simplemente haciendo clic en una cuadrícula 2D.
*   **Modos de Dibujo**:
    *   **Dibujar Sector**: Creación libre de nuevas habitaciones.
    *   **Rectángulo / Círculo**: Creación rápida de formas estándar.
    *   **Editar Vértices**: Mueve esquinas y rediseña sectores existentes con precisión milimétrica.
*   **Sistema de Portales**:
    *   **Portales Automáticos**: El editor detecta y conecta automáticamente las habitaciones adyacentes.
    *   **Portales Manuales**: Enlaza dos paredes cualquiera manualmente para crear espacios complejos o geometrías no euclidianas.

### 2. Gestión de Texturas y Materiales
*   **Soporte Completo de Texturas**: Asigna texturas distintas para **Suelo**, **Techo** y **Paredes** (Superior, Media, Inferior).
*   **Generador de Atlas de Texturas**: Crea automáticamente atlas optimizados para el renderizado de modelos multi-texturizados.
*   **Importador WLD**: Importa mapas antiguos desde el formato `.wld`.

### 3. Vista Previa 3D (Modo Visual)
*   Pulsa **F3** para alternar entre la Vista de Rejilla 2D y el Modo Vuelo 3D.
*   Previsualización de iluminación y texturas en tiempo real.
*   Navega por tu nivel exactamente como lo verá el jugador.

### 4. Generadores Avanzados
*   **Generador de Modelos MD3**: Crea *props* y elementos 3D complejos directamente en el editor sin necesitar Blender o Maya.
    *   **Rampas y Escaleras**: Genera escaleras transitables con número de escalones configurable.
    *   **Arcos y Pilares**: Crea arquitectura curva fácilmente.
    *   **Puentes**: Genera plataformas elevadas con barandillas opcionales.
    *   **Casas**: Creación rápida de estructuras con varios tipos de techo (Plano, A dos aguas).
*   **Conversor OBJ a MD3**: Importa modelos estándar `.obj` y conviértelos para usarlos en tu mapa.
*   **Editor de Caminos de Cámara**: Define esplines y movimientos de cámara cinemáticos para escenas (cutscenes).
*   **Generador de Efectos**: Crea sistemas de partículas y efectos visuales de forma gráfica.

### 5. Gestión de Proyectos y Compilación
*   **Sistema de Proyectos**: Organiza tus mapas, recursos y scripts en un formato de proyecto estructurado.
*   **Integración con BennuGD2**:
    *   **Compilador Integrado**: Pulsa **F5** para compilar tu mapa y scripts directamente.
    *   **Ejecución Rápida**: Pulsa **F9** para probar el juego inmediatamente.
    *   **Compilaciones Multiplataforma**: Detecta automáticamente tu runtime (Windows/Linux/macOS).
    *   **Generación de Código**: Genera automáticamente el código `.prg` necesario para cargar tu mapa.

### 6. Explorador de Recursos (Asset Browser)
*   Explorador de archivos integrado para gestionar tus texturas, sonidos y modelos.
*   Arrastrar y Soltar recursos directamente en la escena.
*   Previsualización de archivos FPG (librerías de sprites).

## Controles

| Tecla | Acción |
| --- | --- |
| **Clic Izquierdo** | Dibujar / Seleccionar / Mover |
| **Clic Derecho** | Menú Contextual / Cancelar Dibujo |
| **Clic Central / Espacio + Arrastrar** | Mover la Rejilla (Pan) |
| **Rueda** | Zoom Acercar/Alejar |
| **F3** | Alternar Modo Visual 3D |
| **F5** | Compilar Proyecto |
| **F9** | Ejecutar Proyecto |
| **Ctrl+S** | Guardar Mapa |
| **Ctrl+Z** | Deshacer |

## Instalación

### Windows
1. Descarga `RayMapEditor-Windows.zip`.
2. Extráelo en una carpeta.
3. Ejecuta `raymap_editor.exe`.
4. *Opcional*: Si no tienes BennuGD2 instalado, usa el menú **Compilar -> Instalar BennuGD2...** para descargar los runtimes automáticamente.

### Linux
1. Descarga `RayMapEditor-Linux.AppImage`.
2. Hazlo ejecutable: `chmod +x RayMapEditor-Linux.AppImage`.
3. Ejecútalo.

### macOS
1. Descarga `RayMapEditor-macOS.dmg`.
2. Arrástralo a la carpeta Aplicaciones.

---

# 🇬🇧 RayMap Editor

**RayMap Editor** is a sophisticated 3D map and level editor designed for BennuGD2 engines (specifically `mod_ray`). It allows creating geometric sector-based maps, similar to classic Build Engine games (Duke Nukem 3D, Blood), but with modern tools and workflow.

## Features

### 1. Geometric Sector Editing
*   **Grid-Based Editing**: Draw sectors simply by clicking on a 2D grid.
*   **Draw Modes**:
    *   **Draw Sector**: Free-form drawing of new rooms.
    *   **Rectangle / Circle**: Quick creation of standard shapes.
    *   **Edit Vertices**: Move corners and reshape existing sectors perfectly.
*   **Portal System**:
    *   **Automatic Portals**: The editor automatically detects connections between adjacent sectors.
    *   **Manual Portals**: Link any two walls manually for complex non-euclidean spaces or manual fixes.

### 2. Texture & Material Management
*   **Full Texture Support**: Assign distinct textures to **Floors**, **Ceilings**, and **Walls** (Upper, Middle, Lower).
*   **Texture Atlas Generator**: Automatically creates atlases for optimized rendering when using multi-textured models.
*   **WLD Importer**: Import legacy maps from `.wld` format.

### 3. Integrated 3D Preview (Visual Mode)
*   Press **F3** to toggle between 2D Grid View and 3D Fly Mode.
*   Real-time lighting and texture preview.
*   Navigate your level exactly as the player will see it.

### 4. Advanced Generators
*   **MD3 Model Generator**: Create complex 3D props directly within the editor without needing Blender/Maya.
    *   **Ramps & Stairs**: Generate walkable staircases with configurable steps.
    *   **Arches & Pillars**: Create curved architecture.
    *   **Bridges**: Generate platforms with optional railings and arches.
    *   **Houses**: Quick building generation with various roof types (Flat, Gabled).
*   **OBJ to MD3 Converter**: Import standard `.obj` models and convert them for use in your map.
*   **Camera Path Editor**: Define cinematic splines and camera movements for cutscenes.
*   **Effect Generator**: Create particle systems and visual effects visually.

### 5. Project Management & Compilation
*   **Project System**: Organize your maps, assets, and scripts in a structured project format.
*   **BennuGD2 Integration**:
    *   **Built-in Compiler**: Press **F5** to compile your map and scripts directly.
    *   **One-Click Run**: Press **F9** to playtest immediately.
    *   **Cross-Platform Builds**: Detects your runtime automatically (Windows/Linux/macOS).
    *   **Code Generation**: Automatically generates the `.prg` code needed to load your map.

### 6. Asset Browser
*   Built-in file explorer to manage your textures, sounds, and models.
*   Drag & Drop assets into the scene.
*   Preview FPG files (sprite libraries).

## Compilation from Source

**Requirements:**
*   Qt 6.8.0 (Widgets, OpenGL, Network, Shadertools)
*   CMake 3.16+
*   C++17 Compiler

**Build Steps:**
```bash
mkdir build && cd build
cmake ..
make -j4
```

---
*Created by [Rufidj](https://github.com/Rufidj) for the BennuGD2 Community.*
