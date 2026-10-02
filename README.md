# Bodycam para SA-MP

Una cámara corporal para GTA San Andreas clásico: se selecciona con **V**, muestra los brazos y el arma del personaje y añade un rótulo con fecha, hora, nombre y placa. El FOV, la posición y el efecto de grabación se ajustan desde un INI.

**[Descargar para jugar](https://github.com/elchimeneas/bodycam-samp/releases/tag/v0.4.4)** · [Configuración explicada](Bodycam.ini) · [Código fuente](https://github.com/elchimeneas/bodycam-samp)

**Versión 0.4.4:** incorpora el logotipo amarillo dentro del ASI y corrige los halos de luces que se veían a través de paredes en la vista corporal. Conserva la corrección de puntería y el FOV predeterminado de 100. Puedes mantener tu INI de 0.4.2 o 0.4.3.

## Descarga e instalación

Necesitas **GTA San Andreas clásico para PC, US 1.0, de 32 bits**, SA-MP y un cargador ASI. La referencia de uso es **SA-MP 0.3.DL-R1**. Definitive Edition y otros ejecutables no están soportados; otras versiones de SA-MP no se han validado.

1. En la página de la versión, descarga **`Bodycam-SA-MP-0.4.4.zip`**. Los enlaces de GitHub «Source code» contienen el código para desarrollar, no la descarga lista para jugar.
2. Cierra GTA y extrae el ZIP. Si ya tienes `Bodycam.asi` y `Bodycam.ini` instalados, guarda una copia antes de sustituirlos.
3. Si ya usas mods ASI, conserva el cargador que tienes. Si no tienes uno, instala la versión **x86/32 bits** de [Ultimate ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader) siguiendo su guía. Revisa los archivos existentes antes de sustituir una DLL. El ZIP de Bodycam no incluye el cargador ni archivos del juego.
4. Copia **`Bodycam.asi` y `Bodycam.ini` junto a `gta_sa.exe`**, en la misma carpeta que utiliza tu cliente SA-MP. Evita tener otra copia de `Bodycam.asi` en `scripts`, `plugins` o `modloader`.
5. Abre `Bodycam.ini` con el Bloc de notas si quieres cambiar el nombre, la placa o los ajustes. `README.md` es esta guía y no necesita copiarse a GTA.
6. Abre SA-MP, entra al servidor y pulsa V para recorrer las vistas. La bodycam va después de la vista más cercana al personaje.

### Qué incluye cada archivo

| Archivo | Para qué sirve |
| --- | --- |
| `Bodycam.asi` | El plugin compilado de 32 bits que carga el juego. Incluye también el logotipo amarillo; no necesita imágenes externas. |
| `Bodycam.ini` | Opciones del jugador, explicadas una por una en español. |
| `README.md` | Esta guía de instalación, controles, ajustes y licencia. |

El ZIP contiene únicamente esos tres archivos. Al cargar el mod, se genera `Bodycam.log` junto al ASI con información breve de inicio.

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
| `ShowLogo=1` | Muestra el logotipo amarillo; usa `0` para ocultarlo. |
| `MarginRight=260` | Aumentar mueve el rótulo a la izquierda, dejando espacio para el HUD. |
| `RecordingEffect=1` | Cambiar a `0` desactiva el grano y la viñeta. |
| `EffectIntensity=0.25` | Intensidad suave; admite 0–1. |

La posición al apuntar suma la base y la corrección: por defecto, avance `0.20 - 0.06 = 0.14` y altura `0.16 - 0.10 = 0.06`. Si adelantas la base `0.01` y quieres conservar el apuntado, reduce `AimForwardOffset` en `0.01`, respetando sus límites. Cambia de poco en poco y comprueba ambas vistas.

Los brazos y el arma pertenecen al modelo real del personaje. Algunas skins y animaciones pueden mostrar la cara, cortes o un arma demasiado grande. El mod no incluye brazos ni animaciones dedicados a primera persona; una posición que funciona con una skin puede necesitar ajustes con otra.

## Actualizar o retirar

**Actualizar:** cierra GTA, respalda el ASI y el INI actuales y sustituye `Bodycam.asi` por el nuevo. Si vienes de 0.4.2 o 0.4.3, conserva tu `Bodycam.ini` personalizado: las opciones son las mismas. Para otras versiones, compara el INI nuevo y traslada tus valores antes de copiarlo.

**Retirar:** con GTA cerrado, quita `Bodycam.asi` y `Bodycam.ini`. Puedes borrar también `Bodycam.log`. Para volver a una versión anterior, recupera tu copia del ASI y del INI.

Si usaste el instalador de 0.4.1 o 0.4.2 y quieres recuperar los archivos que había antes de aquella instalación, utiliza `RETIRAR.cmd` de ese paquete antiguo antes de instalar manualmente la nueva versión. Guarda primero tu INI personalizado; conserva también los respaldos anteriores.

Cambiar el ASI exige reiniciar GTA. No borres el cargador ASI para retirar Bodycam: otros mods pueden utilizarlo.

## Si algo no funciona

- **No aparece la vista:** comprueba que modificaste la instalación que realmente abre SA-MP, que el cargador ASI funciona y que `CameraEnabled=1`. Revisa `Bodycam.log` junto al ASI.
- **Firma no compatible en el registro:** el ejecutable o los puntos de cámara/HUD no coinciden con los esperados. Comprueba que utilizas GTA SA clásico US 1.0 y revisa otros mods que cambien esos puntos.
- **La cámara entra en la cara:** aumenta poco a poco `ChestForward`. Si ocurre solo al apuntar, prueba `AimForwardOffset`; comprueba también la altura. El INI explica los límites.
- **El rótulo tapa el HUD:** aumenta `MarginRight`, o reduce `Scale`. Los márgenes se escalan con la altura de pantalla: a 2160p son el doble que a 1080p con `Scale=1`.
- **Bajan los FPS:** prueba `RecordingEffect=0` y un FOV algo menor. El coste depende del equipo, la escena y los otros mods; no se garantizan 60 FPS.
- **Cierre o conflicto:** restaura el respaldo y abre un [issue](https://github.com/elchimeneas/bodycam-samp/issues) con versión de GTA/SA-MP, otros mods de cámara, pasos, módulo/dirección del fallo y `Bodycam.log`. Revisa y oculta los datos personales antes de compartir capturas o registros.

## Estado y alcance

**0.4.4 es la versión recomendada.** La vista corporal y la corrección de puntería se han comprobado en GTA SA US 1.0 con SA-MP 0.3.DL-R1. El paquete público incluye el logotipo amarillo y mantiene identificadores genéricos configurables. La corrección de profundidad de las luces también fue confirmada por el jugador; véase [LUCES.md](https://github.com/elchimeneas/bodycam-samp/blob/v0.4.4/docs/LUCES.md). El alcance de las pruebas se describe en [VALIDACION.md](https://github.com/elchimeneas/bodycam-samp/blob/v0.4.4/docs/VALIDACION.md); no equivale a probar todas las skins, armas, servidores o sesiones largas. El diagnóstico de la corrección está en [PUNTERIA.md](https://github.com/elchimeneas/bodycam-samp/blob/v0.4.4/docs/PUNTERIA.md).

No se valida compatibilidad universal con ENB, ReShade u otros mods que cambien la cámara o el renderizado. El servidor puede tener sus propias reglas sobre mods de cámara. El plugin no necesita configuración del servidor ni envía datos por red.

## Código y licencia

C++17, Windows x86 y Direct3D 9. [Guía de compilación y estructura](https://github.com/elchimeneas/bodycam-samp/blob/v0.4.4/docs/DESARROLLO.md) · [Historial de cambios](https://github.com/elchimeneas/bodycam-samp/blob/v0.4.4/CHANGELOG.md) · [Referencias y créditos](https://github.com/elchimeneas/bodycam-samp/blob/v0.4.4/docs/CREDITOS.md).

El código y la documentación del proyecto se distribuyen bajo la licencia MIT incluida a continuación. El logotipo amarillo de AXON procede del recurso gráfico del mod de referencia aportado por el usuario; ese recurso y las marcas de terceros no están cubiertos por la licencia MIT del proyecto. Bodycam para SA-MP no está afiliado a AXON. No se incluyen archivos de GTA, SA-MP ni el cargador ASI.

```text
MIT License

Copyright (c) 2026 elchimeneas

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```
