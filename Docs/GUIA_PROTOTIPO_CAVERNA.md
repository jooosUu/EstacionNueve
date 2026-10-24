# Estación Nueve — Prototipo de caverna excavable (todo en Blueprints)

Todo lo nuevo está en `Content/Excavacion/`. No hay C++ nuevo: la lógica está en Blueprints.

## Cómo probarlo
1. Abrir el mapa **`/Game/Excavacion/L_Caverna`** (no es el mapa por defecto).
2. Play (Alt+P). Empiezas afuera, frente a la boca de la cueva.

| Tecla | Acción |
|---|---|
| WASD / Shift / Ctrl / Espacio | Moverse, correr, agacharse, saltar (Semana 1) |
| Clic izquierdo | Picar: cava la roca (si estás a ≤ 3,5 m), pica vetas y ámbar. Suma 15 de ruido |
| L | Prender/apagar la lámpara |
| E | Interactuar: puesto de venta, mejoras, parcela |
| F | Barril explosivo (Semana 1, en `Lvl_FirstPerson`) |

## El bucle de juego
Bajar a la cueva → picar vetas (resina, hierro, carbón) → cavar buscando **ámbar con cría de Carnotaurus** (está enterrado ~2,6 m detrás de vetas de resina: la resina es la pista) → subir → **vender** (resina $6, ámbar $15, fósil $40, forraje $2) → comprar **mejoras** con plata + hierro/carbón → la base crece y cavas mejor → bajar otra vez.

## Blueprints
| Asset | Qué hace |
|---|---|
| `BP_TerrenoExcavable` (hijo de DynamicMeshActor) | **GenerarCaverna**: bloque de roca + túnel por caminata aleatoria con semilla (`Semilla`, `PasosTunel`, `LargoPaso`, `RadioMin/Max`, `Tamano`), restado con un booleano (Geometry Script). Después de 1,5 s reparte vetas con trazos contra las paredes (`ProbVeta`, `ClasesVeta`) y ámbar (`ProbAmbar`). **Excavar**: resta una esfera de radio `RadioExcavar` en `PuntoExcavar` |
| `BP_Veta` + `BP_Veta_Resina/Hierro/Carbon` | Recurso, cantidad por golpe, golpes, material y brillo. Al recibir daño cerca del jugador llama a `AgregarRecurso` |
| `BP_AmbarDino` | Bloque de ámbar (`M_AmbarCristal`) con la Carnotaurus del equipo adentro y luz cálida. 4 golpes: 2 de Ámbar por golpe y 1 Fósil al final |
| `BP_Cyrus` (modificado) | Trazo desde la cámara; si golpea el terreno cerca, lo cava y suma ruido. Mapa `Inventario` (Nombre→Cantidad), función `AgregarRecurso`, componente `Lampara` (L) |
| `UI/WBP_HUDEstacion` | Plata + inventario (arriba derecha) y barra de ruido (abajo). El ruido baja 12/s |
| `BP_GestorEstacion` | Crea el HUD al empezar (va colocado en el nivel) |
| `BP_PuestoVenta` | E: vende lo que esté en `Precios` (hierro y carbón NO se venden: son para construir) |
| `BP_Mejora` | Pedestal configurable (`Titulo`, `CostoPlata`, `RecursoCosto`, `CantidadRecurso`, `Tipo`): 0 pico reforzado (radio de excavación 130→210), 1 lámpara de carburo, 2 generador de vapor (aparece en la base) |
| `BP_Parcela` | E: sembrar helecho → crece en `TiempoCrecer` s → E: cosechar 3 Forraje |

Venta, mejora y parcela tienen además el evento **Interactuar** (hace lo mismo que la E), para reutilizarlo desde un sistema de interacción común.

## Materiales y modelos
- `M_Roca` (ruido en coordenadas de mundo, sin UV), `M_Mineral` + instancias `MI_Resina/Hierro/Carbon/Madera/Laton/Brasa/Tierra/Helecho/Silueta`, `M_AmbarCristal` (parámetros Color, Brillo, Opacidad).
- Modelos importados de `Dino_West_Web/modelos` (CC0 de Quaternius/Kenney + la Carnotaurus propia): `Modelos/`.
- `L_Caverna` tiene un PostProcess con exposición limitada (la cueva es oscura de verdad), viñeta, grano y aberración.

## Tutoriales de referencia
- Excavar túneles en mallas dinámicas: *UE5 Geometry Script pt. 2: Cavando túneles* — Everflame Studio — https://www.youtube.com/watch?v=N8vBfH_EQkk
- Cueva procedural: *Create a Procedural Cave Using PCG and Geometry Scripting* — Procedural Minds — https://www.youtube.com/watch?v=n7bPNo159Hc
- Booleanos en runtime: *Destructible Wall at Runtime (Bool Mesh Operation)* — https://www.youtube.com/watch?v=ITP_8DqFZ-c
- Minería: *UE5: Mining ores* — UE5 Poseidon — https://www.youtube.com/watch?v=1UzGOU9AEpU
- Inventario: *How To Make An Inventory System in UE5* — Unreal University — https://www.youtube.com/watch?v=b2atcZWYi3E
- Tienda: *How to Buy an Item with Currency in UE5* — Gorka Games — https://www.youtube.com/watch?v=xtFbEHtPWRE
- Cultivos: *Sistema de cultivo definitivo en UE5* — Games By Hyper — https://www.youtube.com/watch?v=Ga6tk_rm_GM
- Construcción: *Simple Building System in UE5* — Gorka Games — https://www.youtube.com/watch?v=-aKSc_hYBkE

## Cronograma propuesto (encaja en las entregas de cada jueves)
| Semana | Original | + Estación Nueve |
|---|---|---|
| S2 · 29 oct | Inventario, recoger, llave/puertas | Pico y vetas → inventario (`AgregarRecurso`) |
| S3 · 5 nov | Objetivos, puzle, pausa, blockout | Caverna excavable procedural + blockout de la base |
| S4 · 12 nov | Sonidos, iluminación, notas | Ruido, cueva oscura + lámpara, ámbar con dinos, sonido del pico |
| S5 · 19 nov | Unir y pulir | Venta, mejoras de base, parcela, Carnotaurus por ruido |

## Pendiente / ideas siguientes
- **Carnotaurus por ruido**: si `Noise` ≥ 70 en la superficie de noche, aparece/carga.
- Sonidos: golpe de pico, derrumbe, venta, compra.
- Aceite de la lámpara (se gasta) y refinería (resina → refinada).
- Más mejoras visibles (empalizada, establo, taladro de vapor = excavar más rápido).
- Rendimiento: cada excavación recalcula la colisión de toda la malla; si la cueva crece mucho, partir el terreno en *chunks* de 10×10 m (como pide el Taller, sección 7.5).
- La pared exterior es plana: se puede deformar con `Apply Perlin Noise To Mesh` para que la entrada parezca roca natural.
- El proyecto tiene `r.GenerateMeshDistanceFields=False` (config de Semana 1) con Lumen activo: por eso sale un aviso rojo de Lumen en el editor.
