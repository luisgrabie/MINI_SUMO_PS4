#include <Bluepad32.h>

ControllerPtr ctl = nullptr;

// Pines TB6612/DRV8871
#define PWMA 14
#define PWMB 25
#define AIN1 27
#define AIN2 26
#define BIN1 33
#define BIN2 32
#define STBY 12

#define DEADZONE        20
#define MAXPWM         255
#define BOOSTVAL        30

// --- Rampa ---
#define RAMPA_SUBIDA    18
#define RAMPA_BAJADA    30

// --- Anti-volteo ---
#define ANTIVOLTEO_MS      1500
#define ANTIVOLTEO_GIRO_MS  250

// --- Botones PS4 ---
#define BTN_CRUZ     0x0001   // ✕ → sumo
#define BTN_CIRCULO  0x0002   // ○ → boost adelante
#define BTN_CUADRADO 0x0004   // □ → boost atrás
#define BTN_L1       0x0010   // L1 → giro izquierda
#define BTN_R1       0x0020   // R1 → giro derecha

#define SUMO_MS 500

// ── LEDC PWM config (evita zumbido audible) ──
#define PWM_FREQ    25000   // 25 kHz → inaudible
#define PWM_RES         8   // 8 bits → valores 0-255
#define CH_A            0   // canal LEDC para motor A
#define CH_B            1   // canal LEDC para motor B

// --- Estado ---
bool          sumo_activo       = false;
unsigned long sumo_inicio       = 0;
bool          antivolteo_activo = false;
unsigned long antivolteo_inicio = 0;
unsigned long tiempo_vel_max    = 0;
float         rampaIzq          = 0;
float         rampaDer          = 0;

// ─────────────── Motores ───────────────
void setMotor(int left, int right) {
  digitalWrite(AIN1, left >= 0);
  digitalWrite(AIN2, left < 0);
  ledcWrite(CH_A, min(abs(left), 255));

  digitalWrite(BIN1, right >= 0);
  digitalWrite(BIN2, right < 0);
  ledcWrite(CH_B, min(abs(right), 255));
}

void stopMotors() {
  ledcWrite(CH_A, 0);
  ledcWrite(CH_B, 0);
  rampaIzq = 0;
  rampaDer = 0;
  tiempo_vel_max = 0;
}

float acercar(float actual, float target) {
  float diff = target - actual;
  if (abs(diff) < 1) return target;
  bool mismaDireccion = (actual == 0) || ((actual > 0) == (target > 0));
  float paso = mismaDireccion ? RAMPA_SUBIDA : RAMPA_BAJADA;
  if (abs(diff) < paso) return target;
  return actual + (diff > 0 ? paso : -paso);
}

void driveRampa(int targetIzq, int targetDer) {
  targetIzq = constrain(targetIzq, -255, 255);
  targetDer = constrain(targetDer, -255, 255);
  rampaIzq = acercar(rampaIzq, targetIzq);
  rampaDer = acercar(rampaDer, targetDer);
  setMotor((int)rampaIzq, (int)rampaDer);
}

void driveDirecto(int izq, int der) {
  rampaIzq = izq;
  rampaDer = der;
  setMotor(izq, der);
}

// ─────────────── Callbacks ───────────────
void onConnectedController(ControllerPtr c) {
  ctl = c;
  Serial.println("🎮 PS4 conectado");
}
void onDisconnectedController(ControllerPtr c) {
  ctl = nullptr;
  stopMotors();
  Serial.println("❌ Desconectado");
}

// ─────────────── Setup ───────────────
void setup() {
  Serial.begin(115200);
  pinMode(AIN1, OUTPUT); pinMode(AIN2, OUTPUT);
  pinMode(BIN1, OUTPUT); pinMode(BIN2, OUTPUT);
  pinMode(STBY, OUTPUT);
  digitalWrite(STBY, HIGH);

  // Configura LEDC a 25 kHz para eliminar zumbido audible
  ledcSetup(CH_A, PWM_FREQ, PWM_RES);
  ledcSetup(CH_B, PWM_FREQ, PWM_RES);
  ledcAttachPin(PWMA, CH_A);
  ledcAttachPin(PWMB, CH_B);

  BP32.setup(onConnectedController, onDisconnectedController);
  Serial.println("🚀 Listo");
  Serial.println("   ✕=sumo | ○=boost+ | □=boost- | R1=giro der | L1=giro izq");
}

// ─────────────── Loop ───────────────
void loop() {
  BP32.update();
  if (!ctl || !ctl->isConnected()) { stopMotors(); return; }

  uint16_t btns = ctl->buttons();

  // ── Sumo con ✕ ──
  if ((btns & BTN_CRUZ) && !sumo_activo) {
    sumo_activo = true;
    sumo_inicio = millis();
    Serial.println("💥 SUMO 0.5s");
  }
  if (sumo_activo && millis() - sumo_inicio >= SUMO_MS) {
    sumo_activo = false;
  }

  // ── Giro brusco R1/L1 ──
  if (btns & BTN_R1) {
    driveDirecto( 255, -255);
  } else if (btns & BTN_L1) {
    driveDirecto(-255,  255);
  } else {

    // ── Anti-volteo ──
    bool motors_a_tope = (abs(rampaIzq) >= MAXPWM * 0.95) &&
                         (abs(rampaDer) >= MAXPWM * 0.95);
    if (!antivolteo_activo) {
      if (motors_a_tope) {
        if (tiempo_vel_max == 0) tiempo_vel_max = millis();
        else if (millis() - tiempo_vel_max > ANTIVOLTEO_MS) {
          antivolteo_activo = true;
          antivolteo_inicio = millis();
          Serial.println("🔄 Anti-volteo");
        }
      } else {
        tiempo_vel_max = 0;
      }
    }

    if (antivolteo_activo) {
      driveDirecto(255, -255);
      if (millis() - antivolteo_inicio > ANTIVOLTEO_GIRO_MS) {
        antivolteo_activo = false;
        tiempo_vel_max    = 0;
        Serial.println("✅ Anti-volteo fin");
      }
    } else {

      // ── Cruceta ──
      uint8_t d = ctl->dpad();
      if      (d & DPAD_UP)    { driveRampa(-MAXPWM, -MAXPWM); }
      else if (d & DPAD_DOWN)  { driveRampa( MAXPWM,  MAXPWM); }
      else if (d & DPAD_LEFT)  { driveRampa(-MAXPWM,  MAXPWM); }
      else if (d & DPAD_RIGHT) { driveRampa( MAXPWM, -MAXPWM); }
      else {

        // ── Joysticks proporcionales ──
        int y = ctl->axisY();
        int x = ctl->axisRX();

        if (abs(y) < DEADZONE) y = 0;
        if (abs(x) < DEADZONE) x = 0;

        int forwardVal = map(y, -512, 512, -MAXPWM, MAXPWM);
        int turn       = map(x, -512, 512, -MAXPWM, MAXPWM);

        int leftM  = forwardVal + turn;
        int rightM = forwardVal - turn;

        // ── Boost ○=adelante □=atrás ──
        if (btns & BTN_CIRCULO)  { leftM -= BOOSTVAL; rightM -= BOOSTVAL; }
        if (btns & BTN_CUADRADO) { leftM += BOOSTVAL; rightM += BOOSTVAL; }

        // Sin input → stop con rampa
        if (y == 0 && x == 0 && !(btns & (BTN_CIRCULO | BTN_CUADRADO))) {
          driveRampa(0, 0);
        } else {
          driveRampa(leftM, rightM);
        }
      }
    }
  }

  delay(10);
}
