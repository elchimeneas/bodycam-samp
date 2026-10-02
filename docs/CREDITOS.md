# Referencias y créditos

Proyecto mantenido por [elchimeneas](https://github.com/elchimeneas).

La idea partió de un mod de rótulo de bodycam compartido por el usuario. Esta implementación nativa incorpora una cámara corporal y un renderizador propios. Desde 0.4.4, a petición del usuario, la descarga pública incluye el mismo logotipo amarillo de su instalación de referencia. La etiqueta y los datos del jugador se generan desde el INI.

## Recurso gráfico

`resources/axon.png` procede del archivo `Bodycam(1).rar` aportado por el usuario. Se conserva la imagen de referencia sin modificar y solo se dibuja su región de logotipo amarillo. Se integra como recurso en el ASI; el jugador no necesita un PNG externo. No se distribuye el script Lua original.

El código y la documentación propios están bajo MIT. Esa licencia no se aplica al recurso gráfico de referencia ni concede derechos sobre la marca AXON. El logotipo y las marcas de terceros pertenecen a sus respectivos titulares; este proyecto no está afiliado a AXON.

Referencias técnicas para las estructuras y funciones del juego:

- [plugin-sdk](https://github.com/DK22Pac/plugin-sdk), documentación comunitaria de clases y direcciones de GTA.
- [gta-reversed](https://github.com/gta-reversed/gta-reversed), referencia del funcionamiento del motor original.
- [Microsoft Direct3D 9](https://learn.microsoft.com/en-us/windows/win32/direct3d9), API de dibujo y gestión de estados.
- [Ultimate ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader), cargador recomendado cuando no hay uno instalado. Se obtiene por separado y conserva su propia licencia.

El código usa directamente las API de Windows, Windows Imaging Component y Direct3D 9, sin integrar bibliotecas de plugin-sdk ni gta-reversed. No se redistribuyen archivos de GTA ni SA-MP. Las marcas y nombres de juegos y plataformas pertenecen a sus titulares; el proyecto no está afiliado a ellos.
