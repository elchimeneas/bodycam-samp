# Límite de giro horizontal

Disponible desde 0.4.5. El recorrido predeterminado es de 90 grados en total, repartidos en 45 hacia cada lado del torso.

## Configuración

En la sección `[Camera]` de `Bodycam.ini`:

```ini
; Máximo hacia cada lado del torso: 45 equivale a 90 grados en total.
CameraYawLimit=45
```

Admite de 10 a 90 grados por lado. Los INI antiguos usan 45 si falta la opción. Si tienes un INI de prueba con `CameraYawLimit=60`, cambia ese valor a `45` para obtener el recorrido de 90 grados. Guarda el archivo y pulsa Ctrl + Shift + R para recargarlo. No cambia el FOV ni el movimiento vertical. El límite acompaña al cuerpo: para mirar detrás hay que girar al personaje.

## Implementación

Se calcula la diferencia horizontal entre la dirección de la cámara y la matriz del personaje local. Al superar el límite se rotan juntos los tres ejes de la cámara; `CCam::Front` y `CCam::Up`, usados por el apuntado, reciben la misma orientación.

El giro que sobrepasa el tope se descuenta de los ángulos horizontales internos de GTA y se anula su velocidad horizontal. Estos valores de entrada se conservan para el siguiente fotograma; así no se acumula una vuelta oculta mientras el jugador sigue moviendo el ratón hacia fuera. Los valores temporales de renderizado se restauran como antes, respetando cambios posteriores de otros mods.

La referencia es la orientación del modelo del personaje, no un límite fijo respecto al mapa. No se fuerza el giro del personaje ni se modifican sus animaciones.

## Comprobaciones realizadas

- Compilación pública de desarrollo y variante local x86 completadas.
- 5.692 comprobaciones de cámara: límites y valores inválidos del INI, ambos lados del giro, cruce de ±180°, conservación de inclinación y ejes, apuntado hacia una mira descentrada y restauración.
- Simulación de 1.000 fotogramas empujando contra el límite y respuesta al invertir el movimiento.
- Pruebas gráficas: 1.448 comprobaciones para la variante pública final. La variante local de prueba pasó 1.430 comprobaciones gráficas y 5.188 de cámara antes de ampliar la matriz de pruebas al nuevo valor predeterminado de 45 grados.

Las pruebas de cámara usan un motor simulado. Se instaló la variante local `0.4.5-test1-local` con respaldo; después, el jugador eligió el recorrido total de 90 grados y pidió publicar. No se registró una medición en vivo de los topes. Queda por ampliar la validación de ambos lados, inversión del ratón, caminar y girar el cuerpo, apuntar/disparar, cambio de vista y vehículos. La validación previa de puntería y luces no confirma por sí sola todas esas situaciones con el límite nuevo.
