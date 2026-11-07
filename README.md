# Owen — controlador de robot (Arduino)

Código del robot **Owen**, hecho para el Proyecto Informático I (4º año, Técnico en Computación). Corre en un Arduino con un driver de motores **TB6612** y recibe órdenes por puerto serie (9600 baudios), por ejemplo desde una app en el celular o la compu.

Integrantes:
- Santiago Jimenez
- Juan Carratu
- Tomas San Martin

## Qué hace

El robot tiene tres modos de competencia, que se eligen con un comando `MODO...`:

| Modo | Para qué sirve |
|---|---|
| `CARRERA` | Conducción rápida; los giros son más ágiles. |
| `SUMO` | Más control y tracción. Tiene `BOOST` (empujón corto de velocidad) y `LOCK` (frenazo con un pequeño impulso para clavar al rival). |
| `GARRA` | Activa los tres servos (base, brazo y pinza) para manejar la garra. |

## Comandos por serie

Cada comando es una línea de texto terminada en `\n`. Se pueden combinar con coma, por ejemplo `F,BOOST`.

| Comando | Acción |
|---|---|
| `F`, `B`, `L`, `R` | Adelante, atrás, giro a la izquierda y giro a la derecha |
| `FL`, `FR`, `BL`, `BR` | Diagonales: un lado a velocidad plena y el otro reducido |
| `S` | Detener |
| `V<0-100>` | Velocidad (por ejemplo `V70`) |
| `MODOCARRERA`, `MODOSUMO`, `MODOGARRA` | Cambiar de modo |
| `BOOST`, `LOCK` | Solo en modo `SUMO` |
| `GW`, `GS`, `GD`, `GA` | Mover la garra (solo en modo `GARRA`) |
| `ABRIR`, `CERRAR` | Abrir y cerrar la pinza |

## Detalles de diseño

- La velocidad de 0 a 100 se convierte a un PWM real con un umbral de arranque (los motores no arrancan por debajo de cierto valor).
- Los servos se conectan solo en modo `GARRA` y se liberan en los demás, para evitar zumbidos y ahorrar corriente.
- La app envía la velocidad cada pocos milisegundos, por eso no se imprime cada cambio por serie.

## Conexiones

| Función | Pines |
|---|---|
| Motor izquierdo (A) | PWM 5, dirección 4 y 2 |
| Motor derecho (B) | PWM 6, dirección 7 y 13 |
| Standby del TB6612 | 8 |
| Servos (base, brazo, pinza) | 9, 10 y 11 |

## Cómo cargarlo

1. Abrí `robot_owen.ino` en el Arduino IDE (usa la librería `Servo`, incluida).
2. Elegí la placa y el puerto, y subilo.
3. Abrí el monitor serie a 9600 baudios: debe aparecer `OWEN LISTO`.
