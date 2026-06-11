# Roadmap de Builds para Portar la Demo 2D

## Estrategia

Cada hito termina en una build ejecutable. No se comienza el siguiente hasta que su escena de validación cumpla los criterios definidos.

El core seguirá siendo C++ sin dependencias externas. Los frameworks nativos estarán detrás de interfaces de plataforma y las librerías externas solo podrán usarse en `ilen-modules`.

## Fase 1: Base Estable

### Build 0.1 — Motor estabilizado

**Meta:** disponer de una base fiable antes de cambiar física, render o formatos.

**Implementación:**

- Corregir gravedad, límites y reutilización de entidades.
- Eliminar correctamente nombres y componentes al destruir entidades.
- Corregir las condiciones de compilación por plataforma.
- Mover la creación/destrucción de entidades al hilo principal.
- Estabilizar las fases `FixedUpdate`, `Update`, `LateUpdate`, `PreRender` y `Render`.
- Añadir contadores de FPS, entidades, draw calls y tiempo de física.
- Crear pruebas de ECS, transformaciones, escenas y serialización.

**Escena de prueba:** `EngineStabilityTest`

- Crear y destruir entidades repetidamente.
- Cargar y descargar escenas y prefabs.
- Ejecutar durante diez minutos con sprites y rigidbodies.

**Build aceptada cuando:**

- No hay entidades duplicadas, referencias inválidas ni bloqueos.
- Cargar una escena produce siempre el mismo estado.
- La gravedad es consistente a 30, 60 y 120 FPS.
- Xcode compila sin errores y sin nuevas advertencias relevantes.

---

### Build 0.2 — Física 2D básica fiable

**Meta:** tener colisiones y triggers utilizables por gameplay.

**Implementación:**

- Separar detección, resolución y almacenamiento de contactos.
- Añadir capas y máscaras de colisión.
- Implementar eventos:
  - `CollisionEnter2D`, `CollisionStay2D`, `CollisionExit2D`.
  - `TriggerEnter2D`, `TriggerStay2D`, `TriggerExit2D`.
- Añadir `OverlapBox`, `Raycast` y comprobación de suelo.
- Implementar swept AABB para movimientos rápidos.
- Añadir visualización debug de colliders, normales y contactos.

**Escena de prueba:** `Physics2DTest`

- Cajas estáticas y dinámicas.
- Pared, suelo, techo y plataforma estrecha.
- Zona trigger y objeto de alta velocidad.

**Build aceptada cuando:**

- Cada evento `Enter` y `Exit` se emite una sola vez.
- Los triggers no producen separación física.
- Un objeto rápido no atraviesa paredes.
- Las máscaras impiden las colisiones configuradas.
- El resultado es reproducible con distintos frame rates.

## Fase 2: Personaje Jugable

### Build 0.3 — Character Controller 2D

**Meta:** controlar un personaje en un nivel de bloques.

**Implementación:**

- Crear `CharacterController2DComponent` y `CharacterController2DSystem`.
- Movimiento cinemático durante `FixedUpdate`.
- Añadir aceleración, desaceleración, velocidad máxima y salto.
- Detectar suelo, techo y paredes.
- Incorporar coyote time y jump buffering.
- Añadir estados `Idle`, `Run`, `Jump`, `Fall`, `Land` y `Dead`.
- Interpolar la representación visual entre pasos físicos.

**Escena de prueba:** `CharacterControllerTest`

- Suelo, huecos, techo bajo y plataformas.
- Punto de reaparición y zona de muerte.

**Build aceptada cuando:**

- El personaje no atraviesa ni queda atrapado en bloques.
- El salto funciona poco después de abandonar una plataforma.
- Pulsar salto ligeramente antes de aterrizar ejecuta el salto.
- El movimiento se siente igual a 30, 60 y 120 FPS.
- El gameplay no mueve directamente el `Transform`.

---

### Build 0.4 — Input unificado y animación

**Meta:** jugar con teclado, gamepad y pantalla táctil.

**Implementación:**

- Crear acciones `Move`, `Jump`, `Pause`, `Confirm` y `Cancel`.
- Permitir varios bindings por acción.
- Añadir dead zones y normalización de sticks.
- Crear controles táctiles configurables.
- Implementar `SpriteAnimatorComponent`.
- Añadir clips, velocidad, loop y eventos de animación.
- Conectar las animaciones con los estados del personaje.

**Escena de prueba:** `InputAnimationTest`

- Personaje con animaciones completas.
- Visualización de acciones y dispositivo activo.

**Build aceptada cuando:**

- Puede completarse la escena con los tres métodos de entrada.
- Cambiar de dispositivo no requiere reiniciar.
- Las animaciones reflejan correctamente el estado físico.
- No se pierden pulsaciones de salto entre frames.

## Fase 3: Construcción de Niveles

### Build 0.5 — Tilemap funcional

**Meta:** representar el nivel de Unity mediante tiles.

**Implementación:**

- Crear `TileSetAsset`, `TileMapAsset` y capas.
- Importar mapas JSON/CSV desde `ilen-modules`.
- Dividir el mapa en chunks.
- Renderizar cada chunk mediante batching o instancing.
- Generar colliders combinados para tiles sólidos adyacentes.
- Añadir flip, rotación, orden de capas y parallax.
- Mantener triggers y objetos interactivos como prefabs separados.

**Escena de prueba:** `TilemapStressTest`

- Mapa grande con varias capas.
- Tiles sólidos, decorativos y animados.
- Cámara recorriendo todo el mapa.

**Build aceptada cuando:**

- No se crea una entidad o collider por cada tile.
- Solo se procesan y dibujan chunks visibles.
- El personaje recorre el mapa sin huecos en las colisiones.
- El mapa puede regenerarse de forma determinista.
- La build mantiene 60 FPS en el dispositivo iOS objetivo.

---

### Build 0.6 — Primer nivel jugable

**Meta:** obtener el primer corte completo de gameplay.

**Contenido:**

- Tilemap real o una sección representativa de la demo.
- Personaje, cámara, checkpoint y zona de muerte.
- Plataformas, triggers y objetos interactivos.
- Cambio o reinicio de escena.
- Estado de victoria provisional mediante logs o gráficos simples.

**Build aceptada cuando:**

- El nivel puede iniciarse, completarse y reiniciarse.
- Los checkpoints restauran correctamente al jugador.
- No existen bloqueos durante cambios de escena.
- La partida funciona con teclado, gamepad y touch.

## Fase 4: Presentación

### Build 0.7 — Audio

**Meta:** incorporar música y efectos sin contaminar el core.

**Implementación:**

- Definir `IAudioDevice`, `AudioClip`, `AudioSourceComponent` y `AudioListenerComponent`.
- Crear backend Apple con APIs nativas.
- Añadir reproducción, pausa, loop, volumen, pitch y one-shot.
- Crear buses `Master`, `Music`, `SFX` y `UI`.
- Procesar archivos mediante `ilen-modules`.
- Hacer streaming de música y cargar SFX cortos en memoria.

**Escena de prueba:** `AudioTest`

- Música en loop.
- Sonidos de salto, aterrizaje, trigger y botón.
- Controles para volumen y pausa.

**Build aceptada cuando:**

- No hay cortes ni bloqueos durante la carga.
- Pausar o perder el foco produce el comportamiento configurado.
- Cambiar de escena libera o conserva las fuentes correctas.
- Los buses controlan sus sonidos de manera independiente.

---

### Build 0.8 — HUD y menús

**Meta:** reemplazar la UI necesaria de Unity.

**Implementación:**

- Crear canvas renderizado por el motor.
- Añadir `RectTransform`, anclajes, pivotes y escalado.
- Respetar safe areas.
- Implementar imágenes, texto, botones, paneles y barras.
- Generar atlas de fuentes desde `ilen-modules`.
- Añadir hit testing y navegación por touch, mouse y gamepad.

**Escena de prueba:** `UITest`

- HUD, barra de progreso y texto.
- Menú de pausa y pantalla de resultado.
- Diferentes resoluciones y orientaciones.

**Build aceptada cuando:**

- La UI conserva su distribución al cambiar resolución.
- Ningún elemento importante invade la safe area.
- Los menús funcionan con todos los dispositivos de entrada.
- El texto no genera assets o asignaciones costosas cada frame.

---

### Build 0.9 — Bloom

**Meta:** reproducir el aspecto visual principal de la demo.

**Implementación:**

- Convertir el postprocesado en una cadena explícita de pases.
- Renderizar la escena en formato HDR.
- Añadir extracción por luminancia.
- Implementar downsampling, blur, upsampling y composición.
- Exponer `threshold`, `knee`, intensidad y niveles.
- Mantener Motion Blur como efecto independiente.
- Renderizar la UI después de Bloom por defecto.

**Escena de prueba:** `BloomTest`

- Objetos con distintas intensidades emisivas.
- Fondo oscuro y elementos UI.
- Selector visual de parámetros.

**Build aceptada cuando:**

- Solo las zonas luminosas producen Bloom.
- La UI permanece nítida y sin Bloom.
- Activar o desactivar el efecto no rompe la presentación.
- No se crean texturas temporales nuevas cada frame.
- Se mantienen 60 FPS con la configuración objetivo.

## Fase 5: Port Completo

### Build 1.0 — Demo portada

**Meta:** reproducir el flujo completo de la demo de Unity.

**Contenido obligatorio:**

- Todos los niveles requeridos.
- Movimiento, animación y cámara.
- Colisiones y triggers.
- Tilemaps y objetos interactivos.
- Música y efectos.
- HUD, menús y pausa.
- Bloom y configuración gráfica.
- Guardado mínimo de opciones y progreso si la demo lo requiere.

**Build aceptada cuando:**

- La demo puede completarse desde el inicio hasta el final.
- No requiere datos ni runtime de Unity.
- No hay dependencias externas dentro del core.
- No hay cargas síncronas perceptibles durante gameplay.
- Mantiene 60 FPS en el dispositivo iOS objetivo.
- Supera una sesión continua de treinta minutos sin fallos.

## Fase 6: Herramientas Posteriores

### Build 1.1 — Herramientas de diagnóstico

- Inspector runtime del ECS.
- Visualización de colliders, triggers, cámaras y chunks.
- Validación de escenas, assets y referencias.
- Empaquetado reproducible de builds.

### Build 1.2 — Editor 2D mínimo

- Jerarquía de entidades.
- Inspector generado mediante reflexión.
- Vista de escena 2D y selección.
- Gizmos de transformación y colliders.
- Edición de tilemaps.
- Prefabs, guardado y ejecución de la escena.

El editor permanecerá fuera del core y utilizará los mismos formatos de escenas y assets que la build final.
