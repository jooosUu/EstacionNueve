# Roadmap de Prompts para Aura (Laura) — Estación Nueve

> Cada prompt incluye el bloque obligatorio de **REGLAS** al inicio según las directrices del proyecto.

---

## REGLAS (Obligatorias al inicio de cada prompt)
```text
REGLAS DEL PROYECTO "ESTACIÓN NUEVE" (UE 5.8, solo Blueprints):
- Solo 4 ActorComponents: AC_Interactor, AC_Inventario, AC_CompresorMateria y AC_Detector. Una sola interfaz: BPI_Interactuable. Un solo struct: FItemStruct. No crees otros.
- Prohibido Cast To entre actores del mundo. Usa la interfaz, Event Dispatchers, Get Component By Class, Apply Damage con DamageTypes, referencias de Spawn Actor o Instance Editable, Get Actor(s) Of Class y Tags. Solo puedes hacer Cast a clases del motor, a la GameInstance y a los widgets.
- Nada de bucles While: usa Timers o Timelines.
- Compila y guarda al terminar cada grupo de assets, no de a uno. Nunca hagas load/save en bucle. No uses get_asset_meta sobre Behavior Trees ni Blackboards.
- Usa formas básicas y colores planos o emisivos. No busques arte ni pulas materiales.
- No ejecutes PIE ni tomes capturas, salvo que el prompt diga PRUEBA.
- No crees archivos de texto ni documentación.
- No toques Custom Depth ni los agentes de navegación: ya están configurados.
- Todo va en /Game/EstacionNueve/<Core|Jugador|Componentes|Equipo|Mundo|IA|Base|UI|Materiales|Modelos|Mapas>. El input va en /Game/Input.
- Reporte final CORTO: lista de assets creados, errores y warnings, y lo que hayas decidido por tu cuenta. Sin explicaciones.
```

---

## Estado Actual de las Tareas
- [x] **P1:** Core, datos e interfaz
- [x] **P2:** GameInstance, GameMode e input
- [x] **P3:** AC_Inventario y AC_Interactor
- [x] **P4:** AC_Detector y el material de escaneo
- [x] **P5:** Compresor y equipo
- [x] **P6:** BP_Cyrus (PRUEBA)
- [x] **P7:** Mundo A, minería
- [x] **P8:** Mundo B (PRUEBA) — *Actores creados y verificados en compilación; nivel L_Pruebas listo.*
- [ ] **P9: Carnotauro** *(SIGUIENTE PROMPT A EJECUTAR)*
- [ ] **P10:** Montura, bandido y fauna
- [ ] **P11:** Cría y parcelas de la base
- [ ] **P12:** UI
- [ ] **P13:** Test Map y recorrido completo (PRUEBA)

---

## Prompt P9: Carnotauro (Copiar y pegar a Laura)

```text
REGLAS DEL PROYECTO "ESTACIÓN NUEVE" (UE 5.8, solo Blueprints):
- Solo 4 ActorComponents: AC_Interactor, AC_Inventario, AC_CompresorMateria y AC_Detector. Una sola interfaz: BPI_Interactuable. Un solo struct: FItemStruct. No crees otros.
- Prohibido Cast To entre actores del mundo. Usa la interfaz, Event Dispatchers, Get Component By Class, Apply Damage con DamageTypes, referencias de Spawn Actor o Instance Editable, Get Actor(s) Of Class y Tags. Solo puedes hacer Cast a clases del motor, a la GameInstance y a los widgets.
- Nada de bucles While: usa Timers o Timelines.
- Compila y guarda al terminar cada grupo de assets, no de a uno. Nunca hagas load/save en bucle. No uses get_asset_meta sobre Behavior Trees ni Blackboards.
- Usa formas básicas y colores planos o emisivos. No busques arte ni pulas materiales.
- No ejecutes PIE ni tomes capturas, salvo que el prompt diga PRUEBA.
- No crees archivos de texto ni documentación.
- No toques Custom Depth ni los agentes de navegación: ya están configurados.
- Todo va en /Game/EstacionNueve/<Core|Jugador|Componentes|Equipo|Mundo|IA|Base|UI|Materiales|Modelos|Mapas>. El input va en /Game/Input.
- Reporte final CORTO: lista de assets creados, errores y warnings, y lo que hayas decidido por tu cuenta. Sin explicaciones.

TAREA P9: Carnotauro e IA Acústica

1. Importación y Modelos:
   - Importa desde C:\DinoWest\DW_Carnotauro_UE5.fbx a /Game/EstacionNueve/Modelos/Carnotauro/
     Malla esquelética: SKM_Carnotauro con Skeleton SK_Carnotauro.
     Animaciones con prefijo A_Carno_: A_Carno_Idle, A_Carno_Walk, A_Carno_Run, A_Carno_Roar, A_Carno_Attack_HornRam.
     Verifica que exista el socket o hueso Socket_Silla_Montura.
   - Texturas desde C:\DinoWest\: DW_Carnotauro_Piel_NegroRojo.png, DW_Carnotauro_Normales.png, DW_Carnotauro_Ojos.png.
   - Crea los materiales básicos M_Carnotauro y M_Carnotauro_Ojos en /Game/EstacionNueve/Materiales/ y asígnalos.

2. Animation Blueprint:
   - Crea /Game/EstacionNueve/IA/ABP_Carnotauro (Parent AnimInstance, Skeleton SK_Carnotauro).
   - Variables: Speed (Float), IsAttacking (Bool), IsRoaring (Bool).
   - State Machine Locomotion: Idle -> Walk (Speed > 20) -> Run (Speed > 350).
   - Montages: AM_Carno_Roar y AM_Carno_Ataque en DefaultSlot.

3. Actor BP_DinoCarnotauro (/Game/EstacionNueve/IA/BP_DinoCarnotauro):
   - Padre: Character. Mesh: SKM_Carnotauro con ABP_Carnotauro.
   - Cápsula: HalfHeight 120, Radius 90. Collision Preset: Pawn.
   - Componente Scene "PuntoMontura" atacheado a Socket_Silla_Montura.
   - Box Collision "ZonaMontar" (Extent 140, 140, 120) con OverlapOnlyPawn.
   - Implementa BPI_Interactuable: al interactuar monta/desmonta a Cyrus cambiando IsMounted y llamando al dispatcher OnMonturaCambio.
   - Lógica de IA Acústica: suscribe al dispatcher OnNoise de BP_GI_EstacionNueve.
     - Ruido > 18 dB: entra en estado Investigar hacia la Location del ruido.
     - Ruido > 35 dB: entra en Cazar / Perseguir al Instigador.
     - Al alcanzar objetivo: dispara ataque que aplica 25 de daño (DamageType DT_Mordisco).
   - Variable EstadoCarno (Enum E_EstadoCarno: Deambular, Investigar, Cazar).

Compila y guarda todo en /Game/EstacionNueve/IA/ y reporta los assets creados.
```

---

## Prompt P10: Montura, bandido y fauna

```text
REGLAS DEL PROYECTO "ESTACIÓN NUEVE" (UE 5.8, solo Blueprints):
- Solo 4 ActorComponents: AC_Interactor, AC_Inventario, AC_CompresorMateria y AC_Detector. Una sola interfaz: BPI_Interactuable. Un solo struct: FItemStruct. No crees otros.
- Prohibido Cast To entre actores del mundo. Usa la interfaz, Event Dispatchers, Get Component By Class, Apply Damage con DamageTypes, referencias de Spawn Actor o Instance Editable, Get Actor(s) Of Class y Tags. Solo puedes hacer Cast a clases del motor, a la GameInstance y a los widgets.
- Nada de bucles While: usa Timers o Timelines.
- Compila y guarda al terminar cada grupo de assets, no de a uno. Nunca hagas load/save en bucle. No uses get_asset_meta sobre Behavior Trees ni Blackboards.
- Usa formas básicas y colores planos o emisivos. No busques arte ni pulas materiales.
- No ejecutes PIE ni tomes capturas, salvo que el prompt diga PRUEBA.
- No crees archivos de texto ni documentación.
- No toques Custom Depth ni los agentes de navegación: ya están configurados.
- Todo va en /Game/EstacionNueve/<Core|Jugador|Componentes|Equipo|Mundo|IA|Base|UI|Materiales|Modelos|Mapas>. El input va en /Game/Input.
- Reporte final CORTO: lista de assets creados, errores y warnings, y lo que hayas decidido por tu cuenta. Sin explicaciones.

TAREA P10: Montura, bandido y fauna menor

1. En /Game/EstacionNueve/IA/:
   - BP_MonturaDomable: Character con Tag "Montura". Implementa BPI_Interactuable.
     - Velocidad base 600, sprint 1100.
     - Salta rampas (compatible con BP_RampaSalto).
     - Al presionar E en ZonaMontar, monta a Cyrus y transfiere los controles de IA_Move/IA_Look.
   - BP_FaunaMenor: Actor o Character pequeño (MasaKg 4, Tag "Presa"). Deambula aleatoriamente. Al morir dropea recurso o es comprimible con el rayo.
   - AI_BandidoJinete: Character enemigo montado. Patrulla perimetral; persigue al jugador si hace ruido continuo (>50 dB) o al entrar en su radio de visión (1500 UU). Al ser golpeado por roca lanzada (>800 UU/s) se desmonta.

Compila y guarda. Reporte corto.
```

---

## Prompt P11: Cría y parcelas de la base

```text
REGLAS DEL PROYECTO "ESTACIÓN NUEVE" (UE 5.8, solo Blueprints):
- Solo 4 ActorComponents: AC_Interactor, AC_Inventario, AC_CompresorMateria y AC_Detector. Una sola interfaz: BPI_Interactuable. Un solo struct: FItemStruct. No crees otros.
- Prohibido Cast To entre actores del mundo. Usa la interfaz, Event Dispatchers, Get Component By Class, Apply Damage con DamageTypes, referencias de Spawn Actor o Instance Editable, Get Actor(s) Of Class y Tags. Solo puedes hacer Cast a clases del motor, a la GameInstance y a los widgets.
- Nada de bucles While: usa Timers o Timelines.
- Compila y guarda al terminar cada grupo de assets, no de a uno. Nunca hagas load/save en bucle. No uses get_asset_meta sobre Behavior Trees ni Blackboards.
- Usa formas básicas y colores planos o emisivos. No busques arte ni pulas materiales.
- No ejecutes PIE ni tomes capturas, salvo que el prompt diga PRUEBA.
- No crees archivos de texto ni documentación.
- No toques Custom Depth ni los agentes de navegación: ya están configurados.
- Todo va en /Game/EstacionNueve/<Core|Jugador|Componentes|Equipo|Mundo|IA|Base|UI|Materiales|Modelos|Mapas>. El input va en /Game/Input.
- Reporte final CORTO: lista de assets creados, errores y warnings, y lo que hayas decidido por tu cuenta. Sin explicaciones.

TAREA P11: Cría y parcelas de la base

1. En /Game/EstacionNueve/Base/:
   - Parcela_Paddock: Corral con comedero interactivo (BPI_Interactuable).
     - Entregar "HongoCarnoso" suma +1 Alimento.
     - Entregar "FaunaMenor" suma +2 Alimento.
     - Prompt contextual: "Alimentar cría (Alimento actual: X)".
   - BP_DinoCria: Character con malla a escala 0.35. Deambula dentro del corral.
     - Sistema de crecimiento por Timers (NO bucles While): cada 5 s consume 1 de Alimento y aumenta su escala +0.07 mediante FInterpTo hasta alcanzar escala 1.0.
     - Al llegar a 1.0 se transforma / spawnea en BP_MonturaDomada.
   - Parcela_Laboratorio: Estación de refinamiento de resina y tecnología.
   - Parcela_Gallinero / Criadero: Producción periódica de recursos base.

Compila y guarda. Reporte corto.
```

---

## Prompt P12: UI (Widgets UMG)

```text
REGLAS DEL PROYECTO "ESTACIÓN NUEVE" (UE 5.8, solo Blueprints):
- Solo 4 ActorComponents: AC_Interactor, AC_Inventario, AC_CompresorMateria y AC_Detector. Una sola interfaz: BPI_Interactuable. Un solo struct: FItemStruct. No crees otros.
- Prohibido Cast To entre actores del mundo. Usa la interfaz, Event Dispatchers, Get Component By Class, Apply Damage con DamageTypes, referencias de Spawn Actor o Instance Editable, Get Actor(s) Of Class y Tags. Solo puedes hacer Cast a clases del motor, a la GameInstance y a los widgets.
- Nada de bucles While: usa Timers o Timelines.
- Compila y guarda al terminar cada grupo de assets, no de a uno. Nunca hagas load/save en bucle. No uses get_asset_meta sobre Behavior Trees ni Blackboards.
- Usa formas básicas y colores planos o emisivos. No busques arte ni pulas materiales.
- No ejecutes PIE ni tomes capturas, salvo que el prompt diga PRUEBA.
- No crees archivos de texto ni documentación.
- No toques Custom Depth ni los agentes de navegación: ya están configurados.
- Todo va en /Game/EstacionNueve/<Core|Jugador|Componentes|Equipo|Mundo|IA|Base|UI|Materiales|Modelos|Mapas>. El input va en /Game/Input.
- Reporte final CORTO: lista de assets creados, errores y warnings, y lo que hayas decidido por tu cuenta. Sin explicaciones.

TAREA P12: Interfaz de Usuario (UMG)

En /Game/EstacionNueve/UI/:
1. WBP_Crosshair: Retícula central con feedback de interactuables (cambio de color cuando AC_Interactor tiene objetivo).
2. WBP_PromptInteraccion: Letra E en recuadro + texto de acción en pantalla.
3. WBP_MedidorRuido: Barra de decibelios vinculada al dispatcher OnNoise o lectura de AC_Detector. Raya de advertencia a los 35 dB y roja a los 70 dB.
4. WBP_CinturonHotbar: 3 ranuras inferiores indicando herramienta equipada y ranura activa de AC_Cinturon / Cyrus.
5. WBP_Inventario: Mochila con slots y visualización de slots comprimidos del Rayo Compresor. Abrir y cerrar con tecla Tab.
6. WBP_Notificacion: Mensajes flotantes temporales en la esquina inferior izquierda.

Compila y guarda todos los widgets.
```

---

## Prompt P13: Test Map y recorrido completo (PRUEBA)

```text
REGLAS DEL PROYECTO "ESTACIÓN NUEVE" (UE 5.8, solo Blueprints):
- Solo 4 ActorComponents: AC_Interactor, AC_Inventario, AC_CompresorMateria y AC_Detector. Una sola interfaz: BPI_Interactuable. Un solo struct: FItemStruct. No crees otros.
- Prohibido Cast To entre actores del mundo. Usa la interfaz, Event Dispatchers, Get Component By Class, Apply Damage con DamageTypes, referencias de Spawn Actor o Instance Editable, Get Actor(s) Of Class y Tags. Solo puedes hacer Cast a clases del motor, a la GameInstance y a los widgets.
- Nada de bucles While: usa Timers o Timelines.
- Compila y guarda al terminar cada grupo de assets, no de a uno. Nunca hagas load/save en bucle. No uses get_asset_meta sobre Behavior Trees ni Blackboards.
- Usa formas básicas y colores planos o emisivos. No busques arte ni pulas materiales.
- No ejecutes PIE ni tomes capturas, salvo que el prompt diga PRUEBA.
- No crees archivos de texto ni documentación.
- No toques Custom Depth ni los agentes de navegación: ya están configurados.
- Todo va en /Game/EstacionNueve/<Core|Jugador|Componentes|Equipo|Mundo|IA|Base|UI|Materiales|Modelos|Mapas>. El input va en /Game/Input.
- Reporte final CORTO: lista de assets creados, errores y warnings, y lo que hayas decidido por tu cuenta. Sin explicaciones.

TAREA P13: Mapa de Integración Final y PRUEBA

1. En /Game/EstacionNueve/Mapas/, crea L_TestRecorrido:
   - Circuito continuo conectando las 4 zonas:
     - Zona 1: Base de operaciones (Paddock, comedero, mesa de equipamiento, rocas derrumbe).
     - Zona 2: Mina y vetas excavables (sensor, ámbar, gas, peaje de diamantes).
     - Zona 3: Sendero de montura con rampas de salto y puente levadizo.
     - Zona 4: Guarida de la Carnotaurus y cristal colosal.
   - Coloca NavMeshBoundsVolume cubriendo todo el suelo transitable.
   - PostProcessVolume Unbound con M_PP_ResaltadoEscaneo.
   - En World Settings: GameMode Override = BP_GM_EstacionNueve.

2. PRUEBA:
   - Ejecuta verificación de juego completo: spawn, equipar herramientas, minar nodo, comprimir roca, alimentar cría en paddock, montar dinosaurio, saltar rampa y disparar alerta acústica del Carnotauro.
   - Reporte con VERDICT y checklist.
```
