# Editor temporal de escenas

Compilar el target `ILEN_TEMPORAL_SCENE_EDITOR` de `ilen-modules` y ejecutar:

```sh
ILEN_TEMPORAL_SCENE_EDITOR nivel.txt
ILEN_TEMPORAL_SCENE_EDITOR nivel.txt --base-scene project_data/scenes/gravity_test.meta_ilenscene
```

Escribe `nivel.meta_ilenscene` junto al `.txt`. `Level_Name` determina `scene_name`.
`[]` está vacío; `[G]` crea Tierra. `[R]` crea la misma Tierra que `[G]`, pero
con rigidbody dinámico (`isStatic: false`) y el script `ilen::game::RotateGround`.
`[P]` genera un marcador llamado `Player Spawn` con UUID, metadatos activos,
la posición XY de su casilla y Z=10, y el script temporal `ilen::game::Ground`.
No lleva render, físicas, collider ni cámara. Solo se admite un `[P]` por nivel.
El script será responsable de instanciar el prefab del jugador y su cámara.
La escala del marcador queda en el valor predeterminado del transform.
Otros valores producen un error con fila y columna, sin modificar archivos.

Cada casilla ocupa 3×3 unidades del layout. Los centros se calculan como
`x = (columna - (ancho - 1) / 2) * 3`, `y = (alto - 1 - fila) * 3`,
con índices desde cero y `z = 100`, como el Ground de la escena de gravedad.
La escala de Ground es `[3,3,3]`: el collider por defecto de 1×1 pasa a medir
3×3 en el mundo, llenando la casilla y haciendo contiguos los bloques del suelo.

La escena base es opcional. Conserva sus campos y entidades excepto aquellas cuyo
script sea `ilen::game::Ground` o `ilen::game::RotateGround`, que sustituye por el
layout. Asigna nuevos UUIDs
uint64 positivos y distintos a todas las entidades exportadas. No modifica la base.
Si hay `[P]`, reemplaza también el jugador `Cube`, `GravityCamera` y cualquier
cámara principal de la base para evitar duplicados. Sin `[P]`, conserva los otros
objetos de la base como antes. Sin base, exporta Grounds y el marcador de aparición
cuando hay `[P]`.

Un grid inválido requiere confirmación para regenerar el `.txt` vacío. Reemplazar
una escena de salida existente también requiere confirmación. EOF cancela.

`project_data/scenes/temporal_ground_test.meta_ilenscene` es un ejemplo
con siete Grounds y los otros tres objetos de `gravity_test`. El proceso normal de
binarización indexa la nueva escena como `TemporalGroundTest`; el arranque actual
sigue cargando la escena con ID 1. La escena base incluye los scripts de prueba
existentes, incluido Cube, que carga prefabs y ModelTest al inicializarse.

Pruebas (el binarizador es opcional):

```sh
python3 external/ilen-modules/tests/temporal_scene_editor_test.py /ruta/ILEN_TEMPORAL_SCENE_EDITOR /ruta/ILEN_SCENE_BINARIZER
```
