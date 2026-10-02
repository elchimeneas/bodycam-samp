# Corrección de puntería: 0.4.2

Un jugador comunicó que los disparos se desviaban de la mira al apuntar en la vista bodycam, y que la cámara normal apuntaba correctamente. El problema afectaba a su instalación local 0.4.0; el mismo cálculo estaba presente en la versión pública 0.4.1. La versión 0.4.2 incorpora la corrección.

## Evidencia

Las lecturas del juego en ejecución mostraron:

- Cámara normal: FOV interno de `CCam` 70 y FOV visual de `CDraw` 88,55.
- Bodycam anterior: ambos campos forzados a 100 y proyección visual de 100.
- La instalación tiene un ajuste panorámico que multiplica el FOV de apuntado por 1,265. La función de cálculo del objetivo usa el equivalente a un semiángulo de 63,25 grados cuando recibe 100, mientras la imagen utiliza 50 grados.
- Los vectores de dirección de cámara y el FOV guardados por SA-MP no mostraron una diferencia que explicase ese factor. No se modifican las estructuras de SA-MP para esta corrección.

La correspondencia entre mira y rayo del arma depende de esos valores. Es incorrecto escribir el mismo FOV en ambos campos cuando el juego tiene una conversión entre ellos.

## Cambio

Se conserva la conversión observada al terminar el procesamiento nativo de cámara. Para FOV visual 100 y multiplicador 1,265, el FOV interno pasa a aproximadamente 79,0514; el cálculo del arma vuelve a obtener un ángulo efectivo de 100. Se guardan y restauran los dos campos de forma independiente. Posición, controles y valores del INI permanecen iguales.

Esta compensación cubre el ajuste proporcional observado. No demuestra compatibilidad con transformaciones no lineales, mods que sustituyan el cálculo completo del disparo o cambios de cámara posteriores.

## Pruebas y estado

La nueva prueba geométrica falla con el código anterior y pasa con la corrección. Proyecta el rayo del arma sobre la imagen para FOV 60/100/120, formatos 4:3/16:9/21:9 y conversiones 1/1,265. También comprueba la restauración, las modificaciones posteriores de otros mods y el rechazo de FOV inválidos. La suite gráfica anterior sigue pasando.

**Comprobación en partida:** el 2 de octubre de 2026, tras instalar `0.4.2-test1-local`, el jugador confirmó que la puntería funcionaba correctamente en su instalación de GTA SA US 1.0 con SA-MP 0.3.DL-R1. No se registró una matriz exhaustiva de armas ni distancias.

El código de cámara de esa compilación de prueba es idéntico al de 0.4.2. El paquete público conserva su icono propio y sus datos de ejemplo, y se compila en Release x86. La confirmación del jugador corresponde a la compilación local; el binario público final se somete a las pruebas automáticas de cámara, gráficos e instalación. Queda por ampliar la comprobación a otras combinaciones de mods, armas y sesiones largas.

## Referencias

- [CCamera::Find3rdPersonCamTargetVector](https://github.com/gta-reversed/gta-reversed/blob/master/source/game_sa/Camera.cpp): cálculo del rayo a partir de dirección, FOV y posición de la mira.
- [SAMP-API: AimStuff de 0.3.DL-1](https://github.com/BlastHackNet/SAMP-API/blob/multiver/src/sampapi/0.3.DL-1/AimStuff.cpp): referencia para contrastar los datos guardados por SA-MP, mediante lecturas sin modificaciones.
