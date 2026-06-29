#include <Servo.h>

// ============================================================
// PINES
// ============================================================
#define PWMA     5    // PWM canal izquierdo
#define MOTOR_A1 4
#define MOTOR_A2 2

#define PWMB     6    // PWM canal derecho
#define MOTOR_B1 7
#define MOTOR_B2 13

#define STBY     8    // Standby TB6612 (HIGH = activo)

#define SERVO_BASE   9
#define SERVO_BRAZO  10
#define SERVO_PINZA  11

// ============================================================
// CONSTANTES
// ============================================================
#define VELOCIDAD_MIN_PWM  50   // Por debajo de este PWM los motores no arrancan
#define SERVO_PASO         8    // Grados por comando de garra (antes 10, más fluido)
#define BOOST_DURACION_MS  250  // Cuánto dura el boost en SUMO
#define SERIAL_BAUD        9600

// ============================================================
// ESTADO GLOBAL
// ============================================================
int   velocidad   = 60;       // 0-100 (arranca con algo de velocidad)
String modo       = "CARRERA";
String lastCmd    = "";

// Servos
Servo servoBase;
Servo servoBrazo;
Servo servoPinza;

int anguloBase  = 90;
int anguloBrazo = 90;
int anguloPinza = 90;

// Boost SUMO
unsigned long boostTimer = 0;
bool boostActivo = false;

// ============================================================
// SETUP
// ============================================================
void setup() {
  Serial.begin(SERIAL_BAUD);

  // Pines motores
  pinMode(PWMA,     OUTPUT);
  pinMode(MOTOR_A1, OUTPUT);
  pinMode(MOTOR_A2, OUTPUT);
  pinMode(PWMB,     OUTPUT);
  pinMode(MOTOR_B1, OUTPUT);
  pinMode(MOTOR_B2, OUTPUT);
  pinMode(STBY,     OUTPUT);

  // Activar TB6612
  digitalWrite(STBY, HIGH);

  // Servos en posición central (no adjuntar hasta competencia GARRA)
  // Se adjuntan dinámicamente cuando llega "MODOGARRA"
  servoBase.write(anguloBase);
  servoBrazo.write(anguloBrazo);
  servoPinza.write(anguloPinza);

  detener();

  Serial.println("OWEN LISTO");
  Serial.print("Modo: ");
  Serial.println(modo);
}

// ============================================================
// LOOP
// ============================================================
void loop() {
  // Vencer boost si expiró
  if (boostActivo && (millis() - boostTimer > BOOST_DURACION_MS)) {
    boostActivo = false;
  }

  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    if (cmd.length() > 0) {
      procesarComando(cmd);
    }
  }
}

// ============================================================
// PROCESADOR DE COMANDOS
// ============================================================
void procesarComando(String cmd) {
  // Comando combinado (ej: "F,BOOST")
  int coma = cmd.indexOf(',');
  if (coma != -1) {
    procesarComando(cmd.substring(0, coma));
    procesarComando(cmd.substring(coma + 1));
    return;
  }

  // --- VELOCIDAD ---
  if (cmd.startsWith("V")) {
    int v = cmd.substring(1).toInt();
    velocidad = constrain(v, 0, 100);
    return; // No imprimir cada cambio de velocidad (llega cada 25ms)
  }

  // --- MODO ---
  if (cmd.startsWith("MODO")) {
    String nuevoModo = cmd.substring(4);
    if (nuevoModo != modo) {
      modo = nuevoModo;
      detener();

      if (modo == "GARRA") {
        adjuntarServos();
      } else {
        // Liberar servos cuando no se usan (evita zumbido y ahorra corriente)
        servoBase.detach();
        servoBrazo.detach();
        servoPinza.detach();
      }

      Serial.print("Modo: ");
      Serial.println(modo);
    }
    return;
  }

  // --- MOVIMIENTO ---
  if (cmd == "F")  { moverAdelante();         return; }
  if (cmd == "B")  { moverAtras();            return; }
  if (cmd == "L")  { girarIzquierda();        return; }
  if (cmd == "R")  { girarDerecha();          return; }
  if (cmd == "FL") { moverAdelanteIzquierda(); return; }
  if (cmd == "FR") { moverAdelanteDerecha();  return; }
  if (cmd == "BL") { moverAtrasIzquierda();   return; }
  if (cmd == "BR") { moverAtrasDerecha();     return; }
  if (cmd == "S")  { detener();               return; }

  // --- SUMO ESPECIALES ---
  if (cmd == "BOOST" && modo == "SUMO") {
    boostActivo = true;
    boostTimer  = millis();
    // El boost aumenta la velocidad efectiva; el movimiento
    // viene del comando de dirección que llega junto (F,BOOST)
    Serial.println("BOOST");
    return;
  }

  if (cmd == "LOCK" && modo == "SUMO") {
    // Frenazo brusco + pequeño impulso adelante para "clavar"
    detener();
    delay(30);
    setMotores(40, 40, true, true);
    delay(80);
    detener();
    Serial.println("LOCK");
    return;
  }

  // --- GARRA ---
  if (cmd == "GW") {
    anguloBrazo = constrain(anguloBrazo + SERVO_PASO, 0, 180);
    if (servoBrazo.attached()) servoBrazo.write(anguloBrazo);
    return;
  }
  if (cmd == "GS") {
    anguloBrazo = constrain(anguloBrazo - SERVO_PASO, 0, 180);
    if (servoBrazo.attached()) servoBrazo.write(anguloBrazo);
    return;
  }
  if (cmd == "GD") {
    anguloBase = constrain(anguloBase + SERVO_PASO, 0, 180);
    if (servoBase.attached()) servoBase.write(anguloBase);
    return;
  }
  if (cmd == "GA") {
    anguloBase = constrain(anguloBase - SERVO_PASO, 0, 180);
    if (servoBase.attached()) servoBase.write(anguloBase);
    return;
  }
  if (cmd == "STOP") {
    // La app manda STOP al soltar un botón de garra.
    // No detiene los motores de movimiento, solo "para" la garra
    // (los servos ya mantienen posición solos al dejar de escribir).
    return;
  }
  if (cmd == "ABRIR") {
    anguloPinza = 180;
    if (servoPinza.attached()) servoPinza.write(anguloPinza);
    Serial.println("Pinza: ABIERTA");
    return;
  }
  if (cmd == "CERRAR") {
    anguloPinza = 0;
    if (servoPinza.attached()) servoPinza.write(anguloPinza);
    Serial.println("Pinza: CERRADA");
    return;
  }
}

// ============================================================
// FUNCIONES DE MOVIMIENTO
// ============================================================

// Convierte velocidad 0-100 a PWM real con umbral de arranque
int velAPWM(int vel) {
  if (vel <= 0) return 0;
  // Mapear 1-100 al rango VELOCIDAD_MIN_PWM..255
  return map(constrain(vel, 1, 100), 1, 100, VELOCIDAD_MIN_PWM, 255);
}

// Núcleo: escribe directamente a los dos canales
// pwmA, pwmB: ya en escala 0-255
// adelA, adelB: true = adelante, false = atrás
void setMotores(int pwmA, int pwmB, bool adelA, bool adelB) {
  analogWrite(PWMA, constrain(pwmA, 0, 255));
  analogWrite(PWMB, constrain(pwmB, 0, 255));

  digitalWrite(MOTOR_A1, adelA ? HIGH : LOW);
  digitalWrite(MOTOR_A2, adelA ? LOW  : HIGH);

  digitalWrite(MOTOR_B1, adelB ? HIGH : LOW);
  digitalWrite(MOTOR_B2, adelB ? LOW  : HIGH);
}

// Velocidad efectiva considerando boost
int velEfectiva() {
  if (boostActivo && modo == "SUMO") {
    return constrain(velocidad + 30, 0, 100);
  }
  return velocidad;
}

void moverAdelante() {
  int pwm = velAPWM(velEfectiva());
  setMotores(pwm, pwm, true, true);
}

void moverAtras() {
  int pwm = velAPWM(velEfectiva());
  setMotores(pwm, pwm, false, false);
}

void girarIzquierda() {
  // Giro en punto: izq. atrás, der. adelante
  // En CARRERA: más rápido (factor 0.7 del original era 0.5, subo un poco)
  // En SUMO: más lento para no perder adherencia
  float factor = (modo == "CARRERA") ? 0.6 : 0.85;
  int pwm = velAPWM(velEfectiva() * factor);
  setMotores(pwm, pwm, false, true);
}

void girarDerecha() {
  float factor = (modo == "CARRERA") ? 0.6 : 0.85;
  int pwm = velAPWM(velEfectiva() * factor);
  setMotores(pwm, pwm, true, false);
}

// Diagonales: un lado va a velocidad plena, el otro reducida
// Cuanto más reducido el factor, más cerrada la curva
void moverAdelanteIzquierda() {
  int pwmFull    = velAPWM(velEfectiva());
  int pwmReducido = velAPWM(velEfectiva() * 0.45);
  setMotores(pwmReducido, pwmFull, true, true);  // izq. más lento
}

void moverAdelanteDerecha() {
  int pwmFull    = velAPWM(velEfectiva());
  int pwmReducido = velAPWM(velEfectiva() * 0.45);
  setMotores(pwmFull, pwmReducido, true, true);  // der. más lento
}

void moverAtrasIzquierda() {
  int pwmFull    = velAPWM(velEfectiva());
  int pwmReducido = velAPWM(velEfectiva() * 0.45);
  setMotores(pwmReducido, pwmFull, false, false);
}

void moverAtrasDerecha() {
  int pwmFull    = velAPWM(velEfectiva());
  int pwmReducido = velAPWM(velEfectiva() * 0.45);
  setMotores(pwmFull, pwmReducido, false, false);
}

void detener() {
  analogWrite(PWMA, 0);
  analogWrite(PWMB, 0);
  digitalWrite(MOTOR_A1, LOW);
  digitalWrite(MOTOR_A2, LOW);
  digitalWrite(MOTOR_B1, LOW);
  digitalWrite(MOTOR_B2, LOW);
}

// ============================================================
// SERVOS
// ============================================================
void adjuntarServos() {
  servoBase.attach(SERVO_BASE);
  servoBrazo.attach(SERVO_BRAZO);
  servoPinza.attach(SERVO_PINZA);

  // Restaurar posición guardada
  servoBase.write(anguloBase);
  servoBrazo.write(anguloBrazo);
  servoPinza.write(anguloPinza);

  Serial.println("Servos: ACTIVOS");
}
