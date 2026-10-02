# Halos visibles a través de paredes: diagnóstico

## Observación y medición

El jugador comunicó el 3 de octubre de 2026 que, al seleccionar la bodycam, aparecían halos de luces sobre edificios que debían ocultarlos. La inspección de la partida fue de solo lectura, con GTA SA US 1.0 y SA-MP 0.3.DL-R1.

En la vista corporal se midió:

| Parámetro | Valor aproximado |
| --- | --- |
| FOV visual | 100 |
| Plano cercano de `RwCamera`, usado para la geometría | 0,04 |
| `CDraw::ms_fNearClipZ`, usado para los sprites de luces | 0,30 |
| Plano lejano | 837,33 |

La diferencia de plano cercano cambia cómo cada tipo de dibujo convierte la distancia en profundidad. Con esos valores, un halo detrás de una pared puede recibir una profundidad que lo coloca delante de ella. El código del mod cambiaba el plano de `RwCamera` pero dejaba intacta la copia de `CDraw`.

Se contrastaron las direcciones con [CDraw en plugin-sdk](https://github.com/DK22Pac/plugin-sdk/blob/master/plugin_sa/game_sa/CDraw.cpp) y el cálculo con [CSprite en gta-reversed](https://github.com/gta-reversed/gta-reversed/blob/master/source/game_sa/Sprite.cpp). En el proceso instalado, las instrucciones del dibujo de sprites también consultan `0xC3EFA0` y `0xC3EF9C` para el plano cercano y lejano.

## Corrección de 0.4.4

Al aplicar la bodycam, el mod sincroniza `CDraw::ms_fNearClipZ` con el plano cercano que escribe en `RwCamera`. Guarda el valor anterior y lo restaura al salir, siempre que siga siendo el último valor escrito por Bodycam. Si otro mod lo modifica después, respeta ese cambio. Un valor inaccesible o inválido impide aplicar la vista.

No añade opciones al INI ni cambia el FOV, el encuadre, el cálculo de puntería o las luces instaladas.

## Comprobaciones

- La prueba nueva reproduce la inversión de profundidad con los valores medidos. Falla antes de corregir la sincronización y pasa después.
- Comprueba un halo detrás y otro delante de una pared con distintas distancias y planos de recorte. Es una prueba numérica del cálculo de profundidad; no sustituye la comprobación visual en GTA.
- Comprueba la restauración, el respeto a cambios posteriores de otros mods, valores inválidos y 1.000 ciclos sin acumular cambios.
- Pasan 1.669 comprobaciones de cámara, incluidas las de puntería, y 1.431 de configuración y renderizado al preparar la corrección de profundidad. La compilación pública final 0.4.4 pasa 1.448 tras añadir las comprobaciones del logotipo. La variante local con el rótulo original pasa 1.430 de configuración y renderizado.

La compilación de prueba local se identifica como `0.4.4-test1-local`. Se instaló con respaldo del ASI anterior y sin cambiar el INI del jugador. Tras entrar de nuevo a SA-MP y seleccionar la bodycam, 30 muestras consecutivas registraron FOV 100 y plano cercano 0,04 tanto en `RwCamera` como en `CDraw`. El jugador dio por buena la corrección y continuó con la revisión del logotipo de la descarga pública. El mismo código de cámara se incluye en 0.4.4.
