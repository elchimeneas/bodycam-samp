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
```

El empaquetador usa una lista explícita y abre el ZIP terminado para comprobar que contiene exactamente `Bodycam.asi`, `Bodycam.ini` y `README.md`, en la raíz. La guía incluye el aviso MIT completo; no necesita un archivo de licencia adicional dentro del ZIP. El código se distribuye desde el repositorio y los archivos automáticos «Source code» de GitHub.

Resultados: `dist/Bodycam-SA-MP-0.4.3.zip` y `dist/SHA256SUMS.txt`. El checksum queda fuera del ZIP para comprobar la descarga. Antes de publicar, compara los tres archivos extraídos con el ASI compilado, el INI y la guía originales. Después, descarga el ZIP público y comprueba su SHA-256. No edites el ZIP una vez publicado; usa una versión nueva.

`Gestionar.ps1`, los CMD, `LEEME.txt` y `tests/installer_tests.ps1` se conservan como herramientas históricas de 0.4.1/0.4.2. No forman parte del paquete actual ni intervienen en su instalación. Las pruebas del instalador solo se aplican a los paquetes antiguos, que incluían `manifest.json`.

## Estructura

| Archivo | Responsabilidad |
| --- | --- |
| `src/plugin.cpp` | Guardas de versión, enganche del HUD/cámara/ciclo de vista, recarga y ciclo de vida. |
| `src/body_camera.*` | Posición corporal y restauración de los valores de cámara que modifica el plugin. |
| `src/camera_cycle.h` | Inserta la vista en el ciclo nativo. |
| `src/config.*` | INI, límites, fecha/hora y posición del rótulo. |
| `src/renderer.*` | Texto GDI, icono propio y efecto en Direct3D 9, restaurando el estado gráfico. |
| `tests/` | Cámara con motor simulado y renderizado real aislado; también conserva las pruebas del instalador antiguo. |
| `package.ps1` | Genera y comprueba el ZIP de tres archivos para jugadores. |
| `Gestionar.ps1` | Instalador histórico de 0.4.1/0.4.2, conservado en el código fuente. |

## Detalles de integración

Se comprueban firmas del ejecutable US 1.0 antes de instalar los hooks. La cámara se engancha cuando ya existe una vista local jugable, encadenando la llamada instalada por SA-MP. El módulo permanece cargado hasta cerrar GTA para que no queden saltos apuntando a memoria liberada.

La cámara usa el hueso del pecho y la orientación del personaje; conserva el cálculo nativo del ratón y el apuntado. Guarda/restaura posición, FOV interno y visual, plano cercano y proyección. Desde 0.4.2 conserva la conversión proporcional de FOV del ajuste panorámico; véase [PUNTERIA.md](PUNTERIA.md). Excluye cámaras especiales, objetivos ajenos, muerte y estados sin un jugador válido. No altera el modelo del personaje ni escribe en el protocolo de SA-MP. Conviene comprobar cada arma y skin; la confirmación de la corrección no valida todas las combinaciones de mods.

El efecto dibuja dos cuadriláteros con texturas pequeñas creadas una vez; no copia ni procesa la imagen completa en CPU. El texto se actualiza una vez por segundo. El estado D3D9 se captura y restaura en cada dibujo. Las pruebas de coste son aisladas: no predicen los FPS de una partida.

Para una versión nueva, actualiza CMake, el recurso de versión, el mensaje de inicio, `package.ps1`, la cabecera del INI y las guías/enlaces de descarga. Compila, comprueba y publica desde el mismo commit que etiquetas.
