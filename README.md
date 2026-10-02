# Bodycam para SA-MP

Una cámara corporal para GTA San Andreas clásico: se selecciona con **V**, muestra los brazos y el arma del personaje y añade un rótulo con fecha, hora, nombre y placa. El FOV, la posición y el efecto de grabación se ajustan desde un INI.

**[Descargar para jugar](https://github.com/elchimeneas/bodycam-samp/releases/tag/v0.4.1)** · [Configuración explicada](Bodycam.ini) · [Compilar el código](docs/DESARROLLO.md) · [Cambios](CHANGELOG.md)

## Descarga e instalación

Necesitas **GTA San Andreas clásico para PC, US 1.0, de 32 bits**, SA-MP y un cargador ASI. La referencia de uso es **SA-MP 0.3.DL-R1**. Definitive Edition y otros ejecutables no están soportados; otras versiones de SA-MP no se han validado.

1. En la página de la versión, descarga **`Bodycam-SA-MP-0.4.1.zip`**. Los enlaces de GitHub «Source code» contienen el código para desarrollar, no la descarga lista para jugar.
2. Extrae **todo** el ZIP en una carpeta temporal, fuera de la carpeta de GTA. Cierra GTA.
3. Si ya usas mods ASI, conserva el cargador que tienes. Si no tienes uno, instala la versión **x86/32 bits** de [Ultimate ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader) siguiendo su guía. Revisa los archivos existentes antes de sustituir una DLL. El ZIP de Bodycam no incluye el cargador ni archivos del juego.
4. Ejecuta **`INSTALAR.cmd`** y escribe la ruta de la carpeta que contiene `gta_sa.exe` y `samp.dll`. Usa la misma instalación que abre tu cliente SA-MP. Normalmente no necesitas ejecutar como administrador.
5. El instalador comprueba la versión del ejecutable y las huellas del paquete, guarda los archivos anteriores de Bodycam y copia `Bodycam.asi` y `Bodycam.ini` junto a `gta_sa.exe`.
6. Abre SA-MP, entra al servidor y pulsa V para recorrer las vistas. La bodycam va después de la vista más cercana al personaje.

Personaliza el INI **después de instalar**, dentro de GTA. El instalador rechaza un INI modificado en el ZIP extraído porque verifica que el paquete esté intacto. Conserva la carpeta extraída para poder ejecutar `RETIRAR.cmd`.

### Qué incluye cada archivo

| Archivo | Para qué sirve |
| --- | --- |
| `Bodycam.asi` | El plugin compilado de 32 bits que carga el juego. Es el único binario del mod. |
| `Bodycam.ini` | Opciones del jugador, explicadas una por una en español. |
| `INSTALAR.cmd` | Abre el instalador con respaldo y verificación. |
| `RETIRAR.cmd` | Recupera los archivos que había antes de instalar este paquete. |
| `Gestionar.ps1` | Script que utilizan los dos accesos anteriores. |
| `manifest.json` | Versión y huellas SHA-256 de los dos archivos que se instalan. |
| `LEEME.txt` | Guía rápida que puedes leer sin conexión. |
| `README.md`, `docs/`, `CHANGELOG.md`, `LICENSE` | Guía completa, límites conocidos, cambios y licencia. |

El juego genera `Bodycam.log`, un registro breve de carga. El instalador crea `Bodycam.estado.json` y `Bodycam_Backups`; consérvalos para restaurar. No instala ENB, CLEO, MoonLoader ni texturas externas.

### Instalación manual

Con GTA cerrado, guarda una copia de tus anteriores `Bodycam.asi` y `Bodycam.ini`, si existen. Copia los dos archivos nuevos junto a `gta_sa.exe`. Hace falta un cargador ASI. Esta vía no crea el registro del instalador: para deshacerla, retira esos dos archivos y recupera tu copia manual. Evita tener otra copia del mismo ASI en `scripts`, `plugins` o `modloader`.

## Controles

| Control predeterminado | Acción |
| --- | --- |
| **V** | A pie: lejana → media → cercana → bodycam → lejana. Respeta la tecla que tengas asignada a cambiar vista en GTA. |
| **Ctrl + Shift + B** | Oculta o muestra el rótulo. La cámara y el efecto permanecen como estaban. |
| **Ctrl + Shift + R** | Recarga el INI que está junto al ASI. |

Al entrar al juego se conserva la vista normal. Cambiar de personaje o entrar/salir de un vehículo reinicia la selección; vuelve a recorrer las vistas. En vehículos se inserta después de la vista cercana y luego continúa el ciclo nativo. El ciclo inverso del mando permite salir hacia la vista cercana; la entrada se realiza con el ciclo hacia delante.

El rótulo y el efecto aparecen cuando la bodycam está aplicada. Fecha y hora proceden del PC. **No graba vídeo**: para guardar una grabación necesitas tu programa de captura habitual.

## Ajusta la cámara a tu personaje

Abre el [Bodycam.ini](Bodycam.ini) instalado con el Bloc de notas. Guarda y pulsa **Ctrl + Shift + R** en el juego. Usa punto decimal; escribe los comentarios en líneas independientes que empiecen por `;`.

| Opción | Ajuste |
| --- | --- |
| `ChestForward=0.20` | Aumentar adelanta la cámara y puede sacar la lente de la cara. |
| `ChestSide=0.01` | Bajar el valor la mueve hacia la izquierda; subirlo, hacia la derecha. |
| `ChestHeight=0.16` | Aumentar sube la cámara respecto al pecho. |
| `AimForwardOffset=-0.06` | Corrección del avance, solo al apuntar a pie. |
| `AimHeightOffset=-0.10` | Corrección de la altura, solo al apuntar a pie. |
| `CameraNearClip=0.04` | Plano de recorte cercano; ya está en el mínimo admitido. |
| `CameraFOV=100` | Campo de visión. Admite 60–120. |
| `MarginRight=260` | Aumentar mueve el rótulo a la izquierda, dejando espacio para el HUD. |
| `RecordingEffect=1` | Cambiar a `0` desactiva el grano y la viñeta. |
| `EffectIntensity=0.25` | Intensidad suave; admite 0–1. |

La posición al apuntar suma la base y la corrección: por defecto, avance `0.20 - 0.06 = 0.14` y altura `0.16 - 0.10 = 0.06`. Si adelantas la base `0.01` y quieres conservar el apuntado, reduce `AimForwardOffset` en `0.01`, respetando sus límites. Cambia de poco en poco y comprueba ambas vistas.

Los brazos y el arma pertenecen al modelo real del personaje. Algunas skins y animaciones pueden mostrar la cara, cortes o un arma demasiado grande. El mod no incluye brazos ni animaciones dedicados a primera persona; una posición que funciona con una skin puede necesitar ajustes con otra.

## Actualizar o retirar

**Si instalaste con `INSTALAR.cmd`:** cierra GTA, guarda tus valores personales del INI y ejecuta `RETIRAR.cmd` de la versión anterior. Esto restaura lo que había antes de aquella instalación; también conserva tu configuración actual en una carpeta `Bodycam_Backups/retirada-*`. Después extrae e instala el nuevo ZIP. Copia tus valores al INI nuevo para conservar los comentarios y las opciones nuevas.

**Si instalaste a mano:** con GTA cerrado, respalda el ASI y el INI actuales y sustituye ambos por los nuevos. Traslada tus ajustes al INI nuevo. Para retirar, elimina los dos archivos del mod y restaura el respaldo que quieras conservar.

Cambiar el ASI exige reiniciar GTA. No borres el cargador ASI para retirar Bodycam: otros mods pueden utilizarlo.

## Si algo no funciona

- **No aparece la vista:** comprueba que modificaste la instalación que realmente abre SA-MP, que el cargador ASI funciona y que `CameraEnabled=1`. Revisa `Bodycam.log` junto al ASI.
- **Firma no compatible:** el ejecutable o los puntos de cámara/HUD no coinciden con los esperados. No fuerces la instalación cambiando el verificador.
- **La cámara entra en la cara:** aumenta poco a poco `ChestForward`. Si ocurre solo al apuntar, prueba `AimForwardOffset`; comprueba también la altura. El INI explica los límites.
- **El rótulo tapa el HUD:** aumenta `MarginRight`, o reduce `Scale`. Los márgenes se escalan con la altura de pantalla: a 2160p son el doble que a 1080p con `Scale=1`.
- **Bajan los FPS:** prueba `RecordingEffect=0` y un FOV algo menor. El coste depende del equipo, la escena y los otros mods; no se garantizan 60 FPS.
- **Cierre o conflicto:** restaura el respaldo y abre un [issue](https://github.com/elchimeneas/bodycam-samp/issues) con versión de GTA/SA-MP, otros mods de cámara, pasos, módulo/dirección del fallo y `Bodycam.log`. Revisa y oculta los datos personales antes de compartir capturas o registros.

## Estado y alcance

**0.4.1 es la primera versión pública, en desarrollo.** La base se ha usado en GTA SA US 1.0 con SA-MP 0.3.DL-R1; la vista y el apuntado se ajustaron durante esas pruebas. El paquete público cambia el icono, usa identificadores genéricos y reúne la configuración final. Sus comprobaciones automáticas se describen en [VALIDACION.md](docs/VALIDACION.md); no equivalen a probar todas las skins, armas, servidores o sesiones largas.

No se valida compatibilidad universal con ENB, ReShade u otros mods que cambien la cámara o el renderizado. El servidor puede tener sus propias reglas sobre mods de cámara. El plugin no necesita configuración del servidor ni envía datos por red.

## Código y licencia

C++17, Windows x86 y Direct3D 9. [Guía de compilación y estructura](docs/DESARROLLO.md). Código, documentación e icono original bajo [licencia MIT](LICENSE). No se distribuyen archivos de GTA, SA-MP, el cargador ASI ni recursos del mod descargado que inspiró la idea. Consulta [referencias y créditos](docs/CREDITOS.md).
