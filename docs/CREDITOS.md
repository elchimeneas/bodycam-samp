# Referencias y créditos

Proyecto mantenido por [elchimeneas](https://github.com/elchimeneas).

La idea partió de un mod de rótulo de bodycam compartido por el usuario. Esta implementación nativa incorpora una cámara corporal y un renderizador propios. El paquete público no contiene el Lua, la imagen ni el logotipo de aquel archivo: utiliza un icono de cámara generado mediante código y la etiqueta genérica BODYCAM.

Referencias técnicas para las estructuras y funciones del juego:

- [plugin-sdk](https://github.com/DK22Pac/plugin-sdk), documentación comunitaria de clases y direcciones de GTA.
- [gta-reversed](https://github.com/gta-reversed/gta-reversed), referencia del funcionamiento del motor original.
- [Microsoft Direct3D 9](https://learn.microsoft.com/en-us/windows/win32/direct3d9), API de dibujo y gestión de estados.
- [Ultimate ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader), cargador recomendado cuando no hay uno instalado. Se obtiene por separado y conserva su propia licencia.

El código usa directamente las API de Windows y Direct3D 9, sin integrar bibliotecas de plugin-sdk ni gta-reversed. No se redistribuyen archivos de GTA, SA-MP ni recursos de terceros. Las marcas y nombres de juegos y plataformas pertenecen a sus titulares; el proyecto no está afiliado a ellos.
