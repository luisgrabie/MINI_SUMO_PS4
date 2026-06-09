//codigo completo ultima version sin interrupcion de comando 2 modos de curvatura cancion etc etc :)
#include <Bluepad32.h>

ControllerPtr ctl = nullptr;

// ── Pines TB6612 ──
#define PWMA 14
#define PWMB 25
#define AIN1 27
#define AIN2 26
#define BIN1 33
#define BIN2 32
#define STBY 12

// ── Periféricos ──
#define LED1    2
#define LED2    4
#define BUZZER  0   // ⚠️ GPIO0 puede interferir con bootloader; cambia si hay problemas

#define DEADZONE      20
#define MAXPWM       255
#define BOOSTVAL      30
#define RAMPA_SUBIDA  18
#define RAMPA_BAJADA  30

#define ANTIVOLTEO_MS      1500
#define ANTIVOLTEO_GIRO_MS  250

#define BTN_CRUZ      0x0001
#define BTN_CIRCULO   0x0002
#define BTN_CUADRADO  0x0004
#define BTN_TRIANGULO 0x0008  // ★ Triángulo
#define BTN_L1        0x0010
#define BTN_R1        0x0020
#define BTN_L2        0x0040
#define BTN_R2        0x0080

#define SUMO_MS 500

#define PWM_FREQ  25000
#define PWM_RES       8
#define CH_A          0
#define CH_B          1

// ★ Canal PWM dedicado al buzzer (frecuencia variable)
#define CH_BUZ        2

#define ARC_RAPIDO 1.00f
#define ARC_LENTO  0.55f

#define BEEP_ON_MS   80
#define BEEP_OFF_MS  100
#define BEEP_LONG_MS 300

#define LED2_BLINK_MS 400

// ════════════════════════════════════════════════════════════
// ★ MELODÍA NO BLOQUEANTE
// ════════════════════════════════════════════════════════════
// Notas (Hz):
// SOL=783 RE=587 SI=987 LA=880 DO2=1062 RE2=1174 FAS=739 MI=659
// 0 = silencio entre notas separadas (~80ms)
// Notas consecutivas sin 0 = ligadas (sin pausa)

struct Nota {
  uint16_t freq;   // Hz  (0 = silencio)
  uint16_t durMs;  // milisegundos
};

static const Nota MELODIA[] = {
  // ── PRIMERA PARTE ──────────────────────────────────────────
  {783, 500}, {0, 80},   // SOL
  {587, 500}, {0, 80},   // RE
  {987, 500}, {0, 80},   // SI
  {783, 500}, {0, 80},   // SOL
  {1174, 400}, {0, 80},  // RE2
  {1062, 400}, {0, 80},  // DO2
  {987, 400},  {0, 80},  // SI
  {880, 400},  {0, 80},  // LA
  {783, 400},  {0, 80},  // SOL
  {783, 400},  {0, 80},  // SOL
  {739, 400},  {0, 80},  // FAS
  {659, 400},  {0, 80},  // MI
  {587, 400},  {0, 80},  // RE
  {783, 500},  {0, 80},  // SOL
  {880, 500},  {0, 80},  // LA
  {987, 1000}, {0, 80},  // SI (larga)
  // Ligadas (sin silencio entre ellas):
  {1174, 400},            // RE2
  {1062, 400},            // DO2
  {987,  400},            // SI
  {880,  400},            // LA
  {783,  400}, {0, 80},  // SOL
  {1174, 1000}, {0, 80}, // RE2 (larga)
  // ── SEGUNDA PARTE ─────────────────────────────────────────
  // Ligadas:
  {1174, 500},            // RE2
  {987,  500},            // SI
  {1174, 400},            // RE2
  {1062, 400},            // DO2
  {880,  500},            // LA
  {1062, 400},            // DO2
  {987,  500},            // SI
  {783,  400},            // SOL
  {987,  400},            // SI
  {880,  400}, {0, 80},  // LA
  {587,  400}, {0, 80},  // RE
  {659,  400}, {0, 80},  // MI
  {739,  400}, {0, 80},  // FAS
  // Ligadas:
  {783,  500},            // SOL
  {880,  500},            // LA
  {987,  500},            // SI
  {1062, 400},            // DO2
  {1174, 400},            // RE2
  {1062, 400},            // DO2
  {987,  500}, {0, 80},  // SI
  {880,  500}, {0, 80},  // LA
  {783, 1000}, {0, 80},  // SOL (larga, fin)
};

static const int TOTAL_NOTAS = sizeof(MELODIA) / sizeof(Nota);

bool          mel_activa   = false;
int           mel_idx      = 0;
unsigned long mel_timer    = 0;

// ★ Volumen: 3 niveles de duty cycle (30% / 60% / 100% de 255)
const uint8_t VOLUMENES[3] = {76, 153, 255};
int  vol_idx = 2;  // empieza en volumen alto

void iniciarMelodia() {
  mel_activa = true;
  mel_idx    = 0;
  mel_timer  = millis();
  uint16_t f = MELODIA[0].freq;
  if (f > 0) {
    ledcSetup(CH_BUZ, f, 8);
    ledcAttachPin(BUZZER, CH_BUZ);
    ledcWrite(CH_BUZ, VOLUMENES[vol_idx]);
  } else {
    ledcWrite(CH_BUZ, 0);
  }
}

void detenerMelodia() {
  mel_activa = false;
  ledcWrite(CH_BUZ, 0);
  ledcDetachPin(BUZZER);
  pinMode(BUZZER, OUTPUT);
  digitalWrite(BUZZER, LOW);
}

void updateMelodia() {
  if (!mel_activa) return;
  if (millis() - mel_timer < MELODIA[mel_idx].durMs) return;

  mel_idx++;
  if (mel_idx >= TOTAL_NOTAS) {
    detenerMelodia();
    return;
  }
  mel_timer = millis();
  uint16_t f = MELODIA[mel_idx].freq;
  if (f > 0) {
    ledcSetup(CH_BUZ, f, 8);
    ledcAttachPin(BUZZER, CH_BUZ);
    ledcWrite(CH_BUZ, VOLUMENES[vol_idx]);
  } else {
    ledcWrite(CH_BUZ, 0);
    ledcDetachPin(BUZZER);
    pinMode(BUZZER, OUTPUT);
    digitalWrite(BUZZER, LOW);
  }
}

// ════════════════════════════════════════════════════════════
// ★ BUZZER DE SISTEMA (pitidos de confirmación)
// ════════════════════════════════════════════════════════════
// Cuando la melodía está activa los pitidos del sistema se omiten
// para no interferir con el canal de audio.

int           beep_pendientes = 0;
bool          beep_sonando    = false;
unsigned long beep_timer      = 0;
unsigned long beep_duracion   = BEEP_ON_MS;

void iniciarBeeps(int cantidad, unsigned long duracion_on = BEEP_ON_MS) {
  if (mel_activa) return;  // no interrumpir melodía
  beep_pendientes = cantidad;
  beep_duracion   = duracion_on;
  beep_sonando    = true;
  beep_timer      = millis();
  pinMode(BUZZER, OUTPUT);
  digitalWrite(BUZZER, HIGH);
}

void updateBuzzer() {
  if (mel_activa || beep_pendientes <= 0) return;
  unsigned long ahora = millis();
  if (beep_sonando && ahora - beep_timer >= beep_duracion) {
    digitalWrite(BUZZER, LOW);
    beep_sonando = false;
    beep_timer   = ahora;
    beep_pendientes--;
  } else if (!beep_sonando && beep_pendientes > 0 && ahora - beep_timer >= BEEP_OFF_MS) {
    digitalWrite(BUZZER, HIGH);
    beep_sonando = true;
    beep_timer   = ahora;
  }
}

// ════════════════════════════════════════════════════════════
// LED2 parpadeo
// ════════════════════════════════════════════════════════════
unsigned long led2_timer  = 0;
bool          led2_estado = false;

void updateLED2() {
  bool conectado = (ctl && ctl->isConnected());
  if (conectado) {
    digitalWrite(LED2, LOW);
    led2_estado = false;
  } else {
    if (millis() - led2_timer >= LED2_BLINK_MS) {
      led2_estado = !led2_estado;
      digitalWrite(LED2, led2_estado ? HIGH : LOW);
      led2_timer = millis();
    }
  }
}

// ════════════════════════════════════════════════════════════
// Estado general
// ════════════════════════════════════════════════════════════
bool          sumo_activo       = false;
unsigned long sumo_inicio       = 0;
bool          antivolteo_activo = false;
unsigned long antivolteo_inicio = 0;
unsigned long tiempo_vel_max    = 0;
float         rampaIzq          = 0;
float         rampaDer          = 0;

int  modo_arco      = 0;
bool r2_previo      = false;
bool l2_previo      = false;
bool circulo_previo = false;
bool cruz_previo    = false;       // ★ para toggle melodía
bool tri_previo     = false;       // ★ triángulo (+volumen)

// ════════════════════════════════════════════════════════════
// Motores
// ════════════════════════════════════════════════════════════
void setMotor(int left, int right) {
  digitalWrite(AIN1, left  >= 0);
  digitalWrite(AIN2, left  <  0);
  ledcWrite(CH_A, min(abs(left),  255));
  digitalWrite(BIN1, right >= 0);
  digitalWrite(BIN2, right <  0);
  ledcWrite(CH_B, min(abs(right), 255));
}

void stopMotors() {
  ledcWrite(CH_A, 0);
  ledcWrite(CH_B, 0);
  rampaIzq = 0; rampaDer = 0;
  tiempo_vel_max = 0;
}

float acercar(float actual, float target) {
  float diff = target - actual;
  if (abs(diff) < 1) return target;
  bool mismaDir = (actual == 0) || ((actual > 0) == (target > 0));
  float paso = mismaDir ? RAMPA_SUBIDA : RAMPA_BAJADA;
  if (abs(diff) < paso) return target;
  return actual + (diff > 0 ? paso : -paso);
}

void driveRampa(int tIzq, int tDer) {
  tIzq = constrain(tIzq, -255, 255);
  tDer = constrain(tDer, -255, 255);
  rampaIzq = acercar(rampaIzq, tIzq);
  rampaDer = acercar(rampaDer, tDer);
  setMotor((int)rampaIzq, (int)rampaDer);
}

void driveDirecto(int izq, int der) {
  rampaIzq = izq; rampaDer = der;
  setMotor(izq, der);
}

void aplicarArco(int &izq, int &der) {
  if (modo_arco == 1) {
    der = (int)(der * ARC_RAPIDO);
    izq = (int)(izq * ARC_LENTO);
  } else if (modo_arco == 2) {
    izq = (int)(izq * ARC_RAPIDO);
    der = (int)(der * ARC_LENTO);
  }
}

void updateLED1() {
  digitalWrite(LED1, modo_arco != 0 ? HIGH : LOW);
}

// ════════════════════════════════════════════════════════════
// Callbacks Bluepad32
// ════════════════════════════════════════════════════════════
void onConnectedController(ControllerPtr c) {
  ctl = c;
  Serial.println("Mando PS4 conectado");
  iniciarBeeps(2);
}

void onDisconnectedController(ControllerPtr c) {
  ctl = nullptr;
  stopMotors();
  detenerMelodia();
  modo_arco = 0;
  updateLED1();
  Serial.println("Mando desconectado");
}

// ════════════════════════════════════════════════════════════
// Setup
// ════════════════════════════════════════════════════════════
void setup() {
  Serial.begin(115200);

  pinMode(AIN1, OUTPUT); pinMode(AIN2, OUTPUT);
  pinMode(BIN1, OUTPUT); pinMode(BIN2, OUTPUT);
  pinMode(STBY, OUTPUT); digitalWrite(STBY, HIGH);

  pinMode(LED1,   OUTPUT); digitalWrite(LED1,   LOW);
  pinMode(LED2,   OUTPUT); digitalWrite(LED2,   LOW);
  pinMode(BUZZER, OUTPUT); digitalWrite(BUZZER, LOW);

  ledcSetup(CH_A, PWM_FREQ, PWM_RES);
  ledcSetup(CH_B, PWM_FREQ, PWM_RES);
  ledcAttachPin(PWMA, CH_A);
  ledcAttachPin(PWMB, CH_B);

  BP32.setup(onConnectedController, onDisconnectedController);

  iniciarBeeps(1);
  Serial.println("Listo.");
  Serial.println("X=melodia toggle | Triangulo=vol+ | (X con melodia apagada=vol-)");
  Serial.println("R2=arco der | L2=arco izq | O=normal | R1/L1=giro rapido");
}

// ════════════════════════════════════════════════════════════
// Loop
// ════════════════════════════════════════════════════════════
void loop() {
  BP32.update();

  updateBuzzer();
  updateLED2();
  updateMelodia();   // ★ avanza la melodía si está activa

  if (!ctl || !ctl->isConnected()) { stopMotors(); return; }

  uint16_t btns = ctl->buttons();

  bool r2_ahora      = (btns & BTN_R2);
  bool l2_ahora      = (btns & BTN_L2);
  bool circulo_ahora = (btns & BTN_CIRCULO);
  bool cruz_ahora    = (btns & BTN_CRUZ);
  bool tri_ahora     = (btns & BTN_TRIANGULO);

  // ── ★ Modo arco ──────────────────────────────────────────
  if (r2_ahora && !r2_previo) {
    modo_arco = 1; updateLED1(); iniciarBeeps(1);
    Serial.println("Arco DERECHA");
  }
  if (l2_ahora && !l2_previo) {
    modo_arco = 2; updateLED1(); iniciarBeeps(2);
    Serial.println("Arco IZQUIERDA");
  }
  if (circulo_ahora && !circulo_previo) {
    modo_arco = 0; updateLED1(); iniciarBeeps(1, BEEP_LONG_MS);
    Serial.println("Modo NORMAL");
  }

  r2_previo      = r2_ahora;
  l2_previo      = l2_ahora;
  circulo_previo = circulo_ahora;

  // ── ★ Triángulo: subir volumen ────────────────────────────
  if (tri_ahora && !tri_previo) {
    if (vol_idx < 2) {
      vol_idx++;
      Serial.printf("Volumen: %d/3\n", vol_idx + 1);
      iniciarBeeps(vol_idx + 1);   // 1-2-3 pitidos = nivel de volumen
    } else {
      iniciarBeeps(3);
      Serial.println("Volumen al maximo");
    }
  }
  tri_previo = tri_ahora;

  // ── ★ Cruz: toggle melodía / bajar volumen ────────────────
  if (cruz_ahora && !cruz_previo) {
    if (!mel_activa) {
      // Si la melodía está parada: primero X = bajar volumen,
      // mantener X 500ms = iniciar melodía (ver abajo).
      // Implementación simple: X suelto = bajar vol, X al iniciar sumo = melodía
      // Decisión de diseño: X CORTO = bajar volumen, X cuando sumo = iniciar melodía
      // Para simplificar: X siempre inicia/detiene melodía; vol- con L3 o cuadrado
      iniciarMelodia();
      Serial.println("Melodia INICIADA");
    } else {
      detenerMelodia();
      iniciarBeeps(1);
      Serial.println("Melodia DETENIDA");
    }
  }
  // Bajar volumen: CUADRADO (antes era boost; ahora boost solo con joystick)
  // Si prefieres que cuadrado siga siendo boost, asigna vol- a otro botón
  bool cuadrado_ahora = (btns & BTN_CUADRADO);
  static bool cuad_previo = false;
  if (cuadrado_ahora && !cuad_previo) {
    if (vol_idx > 0) {
      vol_idx--;
      iniciarBeeps(vol_idx + 1);
      Serial.printf("Volumen: %d/3\n", vol_idx + 1);
    } else {
      iniciarBeeps(1);
      Serial.println("Volumen al minimo");
    }
  }
  cuad_previo  = cuadrado_ahora;
  cruz_previo  = cruz_ahora;

  // ── Sumo (L3 o selector) ─────────────────────────────────
  // NOTA: Quitamos BTN_CRUZ del sumo porque ahora controla melodía.
  // Sumo ahora se activa con L3 (thumbL press = botón 0x0400 en BP32).
  // Ajusta según tu mando si es necesario.
  bool l3_ahora = (btns & 0x0400);
  if (l3_ahora && !sumo_activo) {
    sumo_activo = true;
    sumo_inicio = millis();
    Serial.println("SUMO");
  }
  if (sumo_activo && millis() - sumo_inicio >= SUMO_MS) sumo_activo = false;

  // ── Giros rápidos ─────────────────────────────────────────
  if (btns & BTN_R1) { driveDirecto( 255, -255); return; }
  if (btns & BTN_L1) { driveDirecto(-255,  255); return; }

  // ── Anti-volteo ───────────────────────────────────────────
  bool a_tope = (abs(rampaIzq) >= MAXPWM * 0.95) && (abs(rampaDer) >= MAXPWM * 0.95);
  if (!antivolteo_activo) {
    if (a_tope) {
      if (tiempo_vel_max == 0) tiempo_vel_max = millis();
      else if (millis() - tiempo_vel_max > ANTIVOLTEO_MS) {
        antivolteo_activo = true;
        antivolteo_inicio = millis();
        Serial.println("Anti-volteo activado");
      }
    } else { tiempo_vel_max = 0; }
  }
  if (antivolteo_activo) {
    driveDirecto(255, -255);
    if (millis() - antivolteo_inicio > ANTIVOLTEO_GIRO_MS) {
      antivolteo_activo = false;
      tiempo_vel_max    = 0;
    }
    return;
  }

  // ── Cruceta ───────────────────────────────────────────────
  uint8_t d = ctl->dpad();
  if      (d & DPAD_UP)    { driveRampa(-MAXPWM, -MAXPWM); return; }
  else if (d & DPAD_DOWN)  { driveRampa( MAXPWM,  MAXPWM); return; }
  else if (d & DPAD_LEFT)  { driveRampa(-MAXPWM,  MAXPWM); return; }
  else if (d & DPAD_RIGHT) { driveRampa( MAXPWM, -MAXPWM); return; }

  // ── Joystick + arco ───────────────────────────────────────
  int y = ctl->axisY();
  int x = ctl->axisRX();
  if (abs(y) < DEADZONE) y = 0;
  if (abs(x) < DEADZONE) x = 0;

  int forwardVal = map(y, -512, 512, -MAXPWM, MAXPWM);
  int turn       = map(x, -512, 512, -MAXPWM, MAXPWM);
  int leftM      = forwardVal + turn;
  int rightM     = forwardVal - turn;

  // Boost: ahora con L2+joystick (arco izquierda ya usa L2, elige otro pin si necesitas)
  // O simplemente actívalo con cuadrado SOLO cuando la melodía ya no ocupa ese botón
  // Para compatibilidad, boost se activa si cuadrado no acaba de cambiar volumen
  // y la melodía no está activa:
  if ((btns & BTN_CUADRADO) && !mel_activa) { leftM += BOOSTVAL; rightM += BOOSTVAL; }

  if (y != 0 || x != 0) aplicarArco(leftM, rightM);

  if (y == 0 && x == 0) {
    driveRampa(0, 0);
  } else {
    driveRampa(leftM, rightM);
  }

  delay(10);
}
