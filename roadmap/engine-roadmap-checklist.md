# Roadmap de Builds de ILEN Engine

## Fase 1: Motor de Físicas 2D
- [ ] **Resolución de Contactos:** Separar internamente la detección matemática de la resolución de colisiones y el almacenamiento de los puntos de contacto.
- [ ] **Filtering & Layers:** Implementar un sistema de máscaras y capas (Bitmasks) para definir qué objetos colisionan entre sí.
- [ ] **Queries Espaciales:** Añadir funciones de consulta (`OverlapBox`, `Raycast`) y detección robusta de suelo/paredes.
- [ ] **Prevención de Tunnelling:** Implementar *Swept AABB* o raycasts predictivos continuos para evitar que entidades a alta velocidad atraviesen geometría estática.
- [ ] **Callbacks de Físicas:** Garantizar que los eventos (`CollisionEnter2D`, `TriggerExit2D`, etc.) se emitan de forma precisa y una sola vez por estado.
- [ ] **Debug Visual:** Renderizar cajas delimitadoras (colliders), normales de impacto y puntos de contacto usando un pipeline de líneas.

## Fase 2: Input, Animación y Character Controller
- [ ] **Sistema de Input Unificado:** Crear abstracción de acciones (`Move`, `Jump`, `Pause`) que soporte teclado, gamepad y controles táctiles.
- [ ] **Refinamiento de Input:** Añadir normalización de sticks analógicos, *dead zones* y buffers de entrada (para no perder el salto entre frames).
- [ ] **Máquina de Animación (`SpriteAnimatorComponent`):** Implementar reproducción de clips, control de velocidad, loops y eventos de animación sincronizados.
- [ ] **Controlador Cinemático (`CharacterController2DSystem`):** Procesar el movimiento y la aceleración/fricción durante el `FixedUpdate`, sin afectar el `Transform` directamente desde el gameplay.
- [ ] **Mecánicas de Plataformeo (Splash/Lemon):** Implementar lógicas específicas de *coyote time*, *jump buffering*, control de altura de salto y detección de techo.
- [ ] **Interpolación Visual:** Interpolar el renderizado de los sprites entre los *ticks* de las físicas para que el movimiento se vea fluido independientemente del framerate.

## Fase 3: Entornos (Tilemaps) y Renderizado
- [ ] **Estructura de Datos de Tilemaps:** Implementar `TileSetAsset` y `TileMapAsset` soportando múltiples capas (background, solid, foreground).
- [ ] **Pipeline de Importación:** Leer archivos generados (JSON/CSV) a través de `ilen-modules` y dividir el mapa en *chunks* espaciales.
- [ ] **Optimización de Renderizado:** Dibujar exclusivamente los chunks visibles por la cámara aprovechando *batching* o *instancing* en la API gráfica (Metal).
- [ ] **Colisiones Geométricas Optimizadas:** Algoritmo para fusionar colliders adyacentes (*Merged Colliders*) por chunk, evitando tener cientos de cajas de colisión individuales por cada bloque de suelo.

## Fase 4: Arquitectura Base y Estabilidad (ECS)
*Nota: Se consolidan las bases después de la construcción de los componentes centrales para testear estrés y refinar arquitectura con implementaciones tangibles.*
- [ ] **Refactor de Entidades:** Asegurar que la creación, destrucción y reciclaje de entidades/componentes ocurra exclusivamente de forma segura en el hilo principal.
- [ ] **Limpieza de Memoria:** Garantizar que al destruir entidades se limpien correctamente sus nombres, referencias y jerarquías sin dejar punteros colgantes.
- [ ] **Estabilización del Game Loop:** Definir y testear el orden estricto de las fases de ejecución (`FixedUpdate`, `Update`, `LateUpdate`, `PreRender`, `Render`).
- [ ] **Manejo de Compilación:** Corregir y unificar las macros y condiciones de compilación por plataforma (especialmente para CMake y Xcode).
- [ ] **Herramientas de Profiling Base:** Implementar contadores en pantalla (métricas de FPS, conteo de entidades activas, draw calls y tiempo ms del hilo de físicas).
- [ ] **Optimización de Estructuras (C++):** Migrar mapas de hash no ordenados a vectores de pares para minimizar los ciclos de CPU en el core system.
- [ ] **Prueba de Estrés (EngineStabilityTest):** Crear escena para instanciar/destruir entidades masivamente y testear carga/descarga de escenas y prefabs de forma determinista.

## Fase 5: UI, Audio y Post-Procesado (Bloom)
- [ ] **Core de Interfaz (UI Canvas):** Implementar sistema de jerarquías con `RectTransform`, anclajes y escalado que respete dinámicamente las *safe areas* de iOS.
- [ ] **Renderizado de Texto y UI:** Generar atlas de fuentes desde `ilen-modules` e implementar *hit testing* en botones para navegación táctil/gamepad.
- [ ] **Backend de Audio Nativo:** Conectar el `IAudioDevice` con las APIs de Apple para manejar streaming de música, cargar SFX cortos en memoria y gestionar el ciclo de interrupciones del OS.
- [ ] **Audio Routing:** Crear buses independientes (`Master`, `Music`, `SFX`, `UI`) con soporte para pitch, loop y control de volumen.
- [ ] **Pipeline HDR:** Convertir el framebuffer principal a formato de alto rango dinámico para soportar intensidades emisivas mayores a 1.0.
- [ ] **Pass de Bloom:** Implementar la cadena de post-procesado (extracción de luminancia > *downsample* > *blur* > *upsample* > composición final), asegurando que la UI se dibuje por encima de este efecto.

## Fase 6: Ensamble del Nivel Jugable (Demo de Lemon's Conspiracy)
- [ ] **Game Loop de Nivel:** Configurar checkpoints, mecánicas de reinicio fluido, cámara de seguimiento (con *deadzones* y *damping*) y transiciones de escena del Acto 1.
- [ ] **Integración de Gameplay:** Unir tilemaps, prefabs interactivos, triggers, partículas y el controlador del personaje en una única escena consolidada.
- [ ] **Build Final y Testing:** Perfilar la demo para mantener 60 FPS consistentes, verificando la ausencia de bloqueos de memoria durante las cargas de recursos y transiciones de nivel.