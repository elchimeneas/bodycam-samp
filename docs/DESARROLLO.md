# Desarrollo

## Compilar

Requisitos: Windows, Visual Studio 2022 o Build Tools, herramientas C++ de escritorio x86, Windows SDK y CMake 3.21 o posterior. El plugin es **Win32/x86**, incluso cuando Windows sea de 64 bits. El runtime de C++ se enlaza estáticamente.

Desde PowerShell, en la raíz del repositorio:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\build.ps1
```

Compila `Release` y ejecuta las pruebas de cámara y gráficas. El resultado está en `build/Release/Bodycam.asi`. Las pruebas gráficas necesitan un dispositivo Direct3D 9 operativo; abren una ventana de prueba oculta y no inician GTA.

En un entorno sin GPU puedes usar `-SkipGraphicsTests`. Esto solo valida la parte de cámara; antes de publicar hay que ejecutar también las pruebas gráficas en un equipo adecuado. `-Configuration RelWithDebInfo` permite depurar; no se usa para el paquete de jugadores.

También puedes usar CMake directamente:

```powershell
cmake -S . -B build -A Win32
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```

## Crear y comprobar el paquete

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\package.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File .\tests\installer_tests.ps1 -PackageDir .\dist\stage-REEMPLAZAR\Bodycam-SA-MP-0.4.2
```

Usa la ruta «Installer test input» que imprime el empaquetador. La prueba del instalador crea un ejecutable PE sintético en una carpeta temporal: no necesita ni inicia GTA, y no toca una instalación real. Simula el estado del proceso para comprobar los bloqueos. El paquete usa una lista explícita de archivos; no incluye PDB, EXE de pruebas, fuentes, registros ni archivos de juego. El código se distribuye desde el repositorio y los archivos automáticos «Source code» de GitHub.

Resultados: `dist/Bodycam-SA-MP-0.4.2.zip` y `dist/SHA256SUMS.txt`. Comprueba también el ZIP extraído y las huellas de `manifest.json` antes de publicar. No edites el ZIP una vez publicado; usa una versión nueva.

## Estructura

| Archivo | Responsabilidad |
| --- | --- |
| `src/plugin.cpp` | Guardas de versión, enganche del HUD/cámara/ciclo de vista, recarga y ciclo de vida. |
| `src/body_camera.*` | Posición corporal y restauración de los valores de cámara que modifica el plugin. |
| `src/camera_cycle.h` | Inserta la vista en el ciclo nativo. |
| `src/config.*` | INI, límites, fecha/hora y posición del rótulo. |
| `src/renderer.*` | Texto GDI, icono propio y efecto en Direct3D 9, restaurando el estado gráfico. |
| `tests/` | Cámara con motor simulado, renderizado real aislado e instalador en una carpeta temporal. |
| `Gestionar.ps1` | Instalación, verificación y restauración. |

## Detalles de integración

Se comprueban firmas del ejecutable US 1.0 antes de instalar los hooks. La cámara se engancha cuando ya existe una vista local jugable, encadenando la llamada instalada por SA-MP. El módulo permanece cargado hasta cerrar GTA para que no queden saltos apuntando a memoria liberada.

La cámara usa el hueso del pecho y la orientación del personaje; conserva el cálculo nativo del ratón y el apuntado. Guarda/restaura posición, FOV interno y visual, plano cercano y proyección. Desde 0.4.2 conserva la conversión proporcional de FOV del ajuste panorámico; véase [PUNTERIA.md](PUNTERIA.md). Excluye cámaras especiales, objetivos ajenos, muerte y estados sin un jugador válido. No altera el modelo del personaje ni escribe en el protocolo de SA-MP. Conviene comprobar cada arma y skin; la confirmación de la corrección no valida todas las combinaciones de mods.

El efecto dibuja dos cuadriláteros con texturas pequeñas creadas una vez; no copia ni procesa la imagen completa en CPU. El texto se actualiza una vez por segundo. El estado D3D9 se captura y restaura en cada dibujo. Las pruebas de coste son aisladas: no predicen los FPS de una partida.

Para una versión nueva, actualiza CMake, el recurso de versión, el mensaje de inicio, `package.ps1`, la cabecera del INI y las guías/enlaces de descarga. Compila, comprueba y publica desde el mismo commit que etiquetas.
