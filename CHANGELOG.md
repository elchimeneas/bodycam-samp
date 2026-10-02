# Cambios

## 0.4.4 — Logotipo amarillo y profundidad de las luces

- La descarga pública muestra el mismo logotipo amarillo que la instalación de referencia. La imagen va integrada en `Bodycam.asi`; el ZIP sigue conteniendo solo el ASI, el INI y el README.
- Comprueba el dibujo del logo desde el recurso del ASI compilado a 1080p y 4K, y su ocultación con `ShowLogo=0`.
- Sincroniza el plano cercano usado por los sprites de luces con el de la cámara corporal, para corregir halos que pasan por delante de paredes que deberían ocultarlos.
- Restaura el valor anterior al salir de la vista y respeta cambios posteriores de otros mods. Conserva la corrección de puntería y las opciones del INI.
- Añade una prueba que reproduce el fallo anterior y comprueba la oclusión y la restauración. Corrección aceptada por el jugador con la versión local de prueba; véase [LUCES.md](docs/LUCES.md).

## 0.4.3 — Descarga simplificada

- El ZIP contiene únicamente `Bodycam.asi`, `Bodycam.ini` y `README.md`, directamente en su raíz.
- Instalación manual: copiar el ASI y el INI junto a `gta_sa.exe`. La guía explica los controles, los ajustes, la actualización y la retirada, e incluye la licencia.
- El empaquetador comprueba los tres nombres para impedir archivos adicionales en futuras descargas.
- Conserva la corrección de puntería de 0.4.2 y las mismas opciones del INI. Al actualizar desde 0.4.2 puedes conservar tu INI personalizado.

## 0.4.2 — Corrección de puntería

- Corrige el desvío entre la mira y el disparo cuando un ajuste panorámico aplica una conversión proporcional entre el FOV interno del juego y el FOV visual.
- Mantiene el FOV visual elegido en el INI, con valor predeterminado de 100. Guarda y restaura por separado los dos valores de FOV.
- Añade pruebas de alineación del rayo del arma con FOV 60/100/120 y formatos 4:3, 16:9 y 21:9, además de comprobaciones de restauración y valores inválidos.
- Corrección confirmada por el jugador en SA-MP 0.3.DL-R1 usando la compilación local de prueba. La versión pública contiene el mismo código de cámara corregido.
- Actualiza las guías y aclara que `CameraFOV` representa el campo de visión visual. No añade opciones ni requiere cambiar los valores del INI.

## 0.4.1 — Primera versión pública

- Cámara corporal dentro del ciclo de vistas de GTA, después de la vista cercana.
- FOV predeterminado de 100, con rango configurable de 60 a 120.
- Ajustes de avance, altura y lateral; correcciones separadas al apuntar a pie.
- Posición inicial con los últimos ajustes de la versión local: avance 0.20, lateral 0.01 y altura 0.16; correcciones de apuntado -0.06 y -0.10.
- Rótulo separado del HUD derecho, nombre y placa editables, fecha/hora local o UTC.
- Grano y viñeta suaves, opcionales y con intensidad configurable.
- Icono original generado por el plugin y rótulo genérico «BODYCAM» para la distribución pública.
- INI comentado en español, instalador con respaldo y retirada, guía de actualización y código bajo MIT.

Las versiones 0.1 a 0.4.0 fueron iteraciones locales. No tienen descargas públicas en este repositorio.
