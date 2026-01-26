# Publicación de Juegos para Windows

El editor RayMap puede publicar juegos para Windows de tres formas diferentes:

## 1. Ejecutable Autónomo (Recomendado) ✨

**Archivo generado:** `NombreJuego.exe`

Un **único archivo .exe** que contiene todo embebido:
- Motor BennuGD (bgdi.exe)
- Código del juego (.dcb)
- Información de versión

**Ventajas:**
- ✓ Un solo archivo, fácil de distribuir
- ✓ No requiere instalación
- ✓ Se auto-extrae en temporal y ejecuta
- ✓ Limpieza automática al cerrar

**Requisitos:**
```bash
sudo apt install mingw-w64
```

**Cómo funciona:**
1. El launcher extrae bgdi.exe y el .dcb a una carpeta temporal
2. Ejecuta el juego
3. Limpia los archivos temporales al salir

---

## 2. Auto-Extraíble 7-Zip (Alternativa)

**Archivo generado:** `NombreJuego_win64.exe`

Un ejecutable que se auto-extrae y ejecuta el juego.

**Requisitos:**
```bash
sudo apt install p7zip-full
```

**Ventajas:**
- ✓ Compresión alta (archivos más pequeños)
- ✓ Incluye todos los assets
- ✓ Ejecución automática después de extraer

---

## 3. Carpeta + Launchers (Siempre disponible)

**Archivos generados:**
- `NombreJuego_win64/` - Carpeta con todo
- `NombreJuego.bat` - Launcher con consola
- `NombreJuego.vbs` - Launcher sin consola (recomendado)
- `README.txt` - Instrucciones

**Contenido:**
- `bgdi.exe` - Motor BennuGD
- `NombreJuego.dcb` - Código compilado
- `*.dll` - Librerías necesarias
- `assets/` - Recursos del juego

**Ventajas:**
- ✓ No requiere herramientas especiales
- ✓ Fácil de modificar/debuggear
- ✓ Compatible con cualquier sistema

---

## Instalación de Dependencias

### Ubuntu/Debian:
```bash
# Para ejecutable autónomo (recomendado)
sudo apt install mingw-w64

# Para auto-extraíble (opcional)
sudo apt install p7zip-full

# Para ZIP (opcional)
sudo apt install zip
```

### Arch Linux:
```bash
sudo pacman -S mingw-w64-gcc p7zip zip
```

---

## Notas Importantes

1. **Binarios de Windows:** Asegúrate de tener los binarios de BennuGD para Windows en:
   ```
   runtime/win64/bgdi.exe
   runtime/win64/*.dll
   ```

2. **Compilación previa:** El proyecto debe estar compilado antes de publicar.

3. **Assets:** Los assets se copian automáticamente desde la carpeta `assets/` del proyecto.

4. **Distribución:** El ejecutable autónomo es ideal para distribución online (itch.io, Steam, etc.)

---

## Troubleshooting

### "MinGW cross-compiler not found"
Instala mingw-w64: `sudo apt install mingw-w64`

### "7z SFX module not found"
Instala p7zip-full: `sudo apt install p7zip-full`

### "No se encontró bgdi.exe"
Coloca los binarios de Windows en `runtime/win64/`

### El ejecutable no funciona en Windows
Verifica que todas las DLLs necesarias estén en la carpeta runtime/win64/
