# Validación de 0.4.6

## Resultado de la compilación local

Comprobación local del 5 de octubre de 2026: compilación MSVC en Release, Win32/x86, completada.

| Prueba | Resultado |
| --- | --- |
| Renderizado, configuración y carga en un proceso ajeno a GTA | 1.458 comprobaciones superadas. |
| Cámara con motor simulado | 5.692 comprobaciones superadas. |
| Contenido del ZIP | Exactamente `Bodycam.asi`, `Bodycam.ini` y `README.md`; contenido idéntico a los archivos de origen. |
| Dependencias del ASI compilado | DLL del sistema Windows; runtime C++ estático. |

El dibujo aislado se comprobó a 1080p y 4K. El logotipo amarillo se carga desde el recurso del ASI final. Se comprobó su presencia en los píxeles del dibujo a 1080p y 4K, su ocultación con `ShowLogo=0` y su aspecto en las previsualizaciones. El test de efecto, sobre un fondo uniforme, cambió como máximo 6 niveles por canal de color (escala 0–255) con la intensidad inicial. Esto no mide ni garantiza la fluidez de una partida.

## Qué se comprueba automáticamente

- INI: caracteres UTF-8, valores inválidos y límites; coincidencia entre la posición inicial del INI distribuido y los valores del código.
- Rótulo: `CameraLabel` por defecto en INI antiguos, texto UTF-8, límite de longitud y título/identificador vacíos. Una recarga con un texto de la misma longitud cambia los píxeles de la segunda línea: comprueba que se actualiza el dibujo incluso si el tamaño de la textura no cambia.
- Cámara: ciclo de vistas, transición a apuntado, exclusión de objetivos ajenos y cámaras especiales, restauración de estado y 1.000 ciclos sin acumulación de la proyección. Se usa un motor simulado.
- Puntería: proyección del rayo del arma sobre la mira con FOV 60/100/120, formatos 4:3/16:9/21:9 y conversión panorámica 1/1,265. La nueva prueba falla con 0.4.1 y pasa con la corrección.
- Giro: límites de 10/45/60/90 grados por lado, cruce de ±180°, inclinación conservada, ejes de cámara y arma alineados, 1.000 fotogramas contra el tope y respuesta inmediata al invertir el movimiento. Los valores por defecto del código y del INI coinciden.
- Gráficos: dibujo real con Direct3D 9 en una ventana oculta, 1080p y 4K, cambio de resolución, restauración de estados, liberación de recursos GDI y 1.200 dibujos repetidos.
- Luces: sincronización del plano cercano de sprites y geometría, halo delante/detrás de una pared, restauración y valores inválidos. La prueba reproduce el fallo del código anterior.
- Logotipo: recurso presente en el ASI, dibujo amarillo a 1080p/4K y ocultación con el INI.
- Efecto: desactivarlo deja intactos los píxeles fuera del rótulo; intensidad cero omite su dibujo; puede funcionar con el rótulo oculto.
- Paquete: integridad del ZIP, tres nombres exactos en la raíz, coincidencia con el ASI compilado y los archivos originales, y comprobación de la huella SHA-256. La instalación actual consiste en copiar el ASI y el INI con GTA cerrado.

Las cifras detalladas se generan en `build/validation*.json` al ejecutar las pruebas. Las mediciones de tiempo son del equipo de prueba, fuera de GTA; no se publican como una promesa de FPS.

## Juego real y límites pendientes

La versión local se utilizó en GTA SA US 1.0 con SA-MP 0.3.DL-R1. Se observó la entrada y salida de la vista corporal y se ajustaron encuadre y posición al apuntar con las capturas del jugador. El 2 de octubre de 2026, tras instalar la corrección `0.4.2-test1-local`, el jugador confirmó que la puntería funcionaba correctamente. Eso acredita ese entorno concreto; no se registró una matriz exhaustiva de armas ni distancias.

El 3 de octubre de 2026 se instaló `0.4.4-test1-local` para corregir los halos visibles a través de paredes. Se verificó en 30 muestras de la bodycam que los planos cercanos coincidían en 0,04. El jugador dio por buena la corrección y pidió que la descarga pública usara también su logotipo amarillo.

Después se instaló `0.4.5-test1-local` con límite de giro. El jugador pidió 90° de recorrido total; se configuró `CameraYawLimit=45` conservando sus otros ajustes y pidió publicar. No se registró una medición en vivo de ambos topes ni una nueva matriz de armas o vehículos.

El paquete 0.4.6 contiene el mismo código de cámara de esa última prueba local y el mismo recurso de logo amarillo, con los valores genéricos del INI público. El valor por defecto del giro se ha ajustado a 45° por lado, también cuando falta la opción en un INI antiguo. Las pruebas automáticas se ejecutan sobre la compilación final Release x86. No se ha hecho una sesión larga en SA-MP con ese binario final. Quedan por ampliar las comprobaciones de skins, armas, interiores, vehículos, sesiones prolongadas y combinaciones con otros mods. Otras versiones de SA-MP no están validadas.

La versión 0.4.6 permite configurar el texto del modelo sin modificar el código de cámara. Se comprobó el rótulo predeterminado en la previsualización a 1080p y su recarga mediante una comparación de píxeles. Falta comprobar esta compilación en una partida.

Las pruebas automáticas no sustituyen esas comprobaciones dentro del juego. Una skin o una animación pueden requerir otros desplazamientos de cámara; el mod usa el modelo del personaje y no incluye brazos específicos de primera persona.
