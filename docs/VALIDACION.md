# Validación de 0.4.1

## Resultado de la compilación pública

Comprobación local del 2 de octubre de 2026: compilación MSVC en Release, Win32/x86, completada.

| Prueba | Resultado |
| --- | --- |
| Renderizado, configuración y carga en un proceso ajeno a GTA | 1.431 comprobaciones superadas. |
| Cámara con motor simulado | 1.172 comprobaciones superadas. |
| Instalación y restauración con PowerShell 5.1 | 20 comprobaciones superadas. |
| Dependencias del ASI publicado | DLL del sistema Windows; runtime C++ estático. |

El dibujo aislado se comprobó a 1080p y 4K. La imagen del nuevo icono y el rótulo se revisó visualmente. El test de efecto, sobre un fondo uniforme, cambió como máximo 6 niveles por canal de color (escala 0–255) con la intensidad inicial. Esto no mide ni garantiza la fluidez de una partida.

## Qué se comprueba automáticamente

- INI: caracteres UTF-8, valores inválidos y límites; coincidencia entre la posición inicial del INI distribuido y los valores del código.
- Cámara: ciclo de vistas, transición a apuntado, exclusión de objetivos ajenos y cámaras especiales, restauración de estado y 1.000 ciclos sin acumulación de la proyección. Se usa un motor simulado.
- Gráficos: dibujo real con Direct3D 9 en una ventana oculta, 1080p y 4K, cambio de resolución, restauración de estados, liberación de recursos GDI y 1.200 dibujos repetidos.
- Efecto: desactivarlo deja intactos los píxeles fuera del rótulo; intensidad cero omite su dibujo; puede funcionar con el rótulo oculto.
- Instalador: instalación y restauración en una carpeta temporal, verificación de huellas, bloqueo ante paquete modificado, ejecutable incompatible y juego abierto. No se inicia GTA.

Las cifras detalladas se generan en `build/validation*.json` al ejecutar las pruebas. Las mediciones de tiempo son del equipo de prueba, fuera de GTA; no se publican como una promesa de FPS.

## Juego real y límites pendientes

La versión local anterior se utilizó en GTA SA US 1.0 con SA-MP 0.3.DL-R1. Se observó la entrada y salida de la vista corporal y se ajustaron encuadre y posición al apuntar con las capturas del jugador. Eso acredita ese entorno concreto.

El binario público 0.4.1 cambia el icono y el rótulo y adopta los últimos valores del INI. No se ha hecho una sesión larga en SA-MP con este binario de publicación. Quedan por comprobar distintas skins, armas, interiores, vehículos, puntería, sesiones prolongadas y combinaciones con otros mods. Otras versiones de SA-MP no están validadas.

Las pruebas automáticas no sustituyen esas comprobaciones dentro del juego. Una skin o una animación pueden requerir otros desplazamientos de cámara; el mod usa el modelo del personaje y no incluye brazos específicos de primera persona.
