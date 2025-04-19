// ---------- Receiver Channel Pins ----------
// Drive motors: ch1 (pin 3) & ch2 (pin 5)
// Blade motor: use channel 3 for toggle -> connected to pin 12
#define RC_RIGHT_PIN 3   // Channel 1 for drive (used as ch1)
#define RC_LEFT_PIN  5   // Channel 2 for drive (used as ch2)
#define RC_BLADE_PIN 12  // Channel 3 for blade toggle

// ---------- Right Drive Motor BTS7960 Pins ----------
#define R_PWM_right 6           // PWM pin (forward)
#define RIGHT_R_EN  2           // Enable pin (forward)
#define R_LEFT_PWM_right 9      // PWM pin (reverse)
#define RIGHT_L_EN  4           // Enable pin (reverse)

// ---------- Left Drive Motor BTS7960 Pins ----------
#define R_PWM_left 10           // PWM pin (forward)
#define LEFT_R_EN  7            // Enable pin (forward)
#define L_PWM_left 11           // PWM pin (reverse)
#define LEFT_L_EN  8            // Enable pin (reverse)

// ---------- Blade Motor BTS7960 Pins ----------
#define BLADE_RPWM 13
#define BLADE_LPWM A1          // Use analog pin A1 as digital output
#define BLADE_REN  A2
#define BLADE_LEN  A3

// --- Rx threshold values for drive motors ---
// Forward thresholds
int Ch1Ch2_start_Fwd = 1530;
int Ch1Ch2_End_Fwd   = 1980;
// Backward thresholds
int Ch1Ch2_start_Bac = 1460;
int Ch1Ch2_End_Bac   = 960;

// For blade motor: define neutral, full reverse, and full forward pulse widths
const int NEUTRAL   = 1500;  
const int MIN_PULSE = 1000;  // Full reverse
const int MAX_PULSE = 2000;  // Full forward

// --- Blade Toggle Globals ---
bool bladeEnabled = false;       // Current blade state (on/off)
bool bladeTogglePressed = false; // Used to debounce toggle action

// --- Motor Control Functions (for blade and drive, reused for consistency) ---
void driveForward(int rpwm, int lpwm, int ren, int len, int speed) {
  digitalWrite(ren, HIGH);
  digitalWrite(len, HIGH);
  analogWrite(rpwm, speed);
  analogWrite(lpwm, 0);
}

void driveReverse(int rpwm, int lpwm, int ren, int len, int speed) {
  digitalWrite(ren, HIGH);
  digitalWrite(len, HIGH);
  analogWrite(rpwm, 0);
  analogWrite(lpwm, speed);
}

void stopMotor(int rpwm, int lpwm, int ren, int len) {
  digitalWrite(ren, LOW);
  digitalWrite(len, LOW);
  analogWrite(rpwm, 0);
  analogWrite(lpwm, 0);
}

void setup() {
  Serial.begin(9600);
  
  // Set receiver pins as inputs
  pinMode(RC_RIGHT_PIN, INPUT);
  pinMode(RC_LEFT_PIN, INPUT);
  pinMode(RC_BLADE_PIN, INPUT);
  
  // Set drive motor control pins as outputs
  pinMode(RIGHT_R_EN, OUTPUT);   // Right enable (forward)
  pinMode(RIGHT_L_EN, OUTPUT);   // Right enable (reverse)
  pinMode(R_PWM_right, OUTPUT);
  pinMode(R_LEFT_PWM_right, OUTPUT);
  
  pinMode(LEFT_R_EN, OUTPUT);    // Left enable (forward)
  pinMode(LEFT_L_EN, OUTPUT);    // Left enable (reverse)
  pinMode(R_PWM_left, OUTPUT);
  pinMode(L_PWM_left, OUTPUT);
  
  // Set blade motor control pins as outputs
  pinMode(BLADE_REN, OUTPUT);
  pinMode(BLADE_LEN, OUTPUT);
  pinMode(BLADE_RPWM, OUTPUT);
  pinMode(BLADE_LPWM, OUTPUT);
  
  // Ensure all motor drivers start disabled
  stopMotor(R_PWM_right, R_LEFT_PWM_right, RIGHT_R_EN, RIGHT_L_EN);
  stopMotor(R_PWM_left, L_PWM_left, LEFT_R_EN, LEFT_L_EN);
  stopMotor(BLADE_RPWM, BLADE_LPWM, BLADE_REN, BLADE_LEN);
}

void loop() {
  // --- Drive Motor Control using drive channels ---
  double ch1 = pulseIn(RC_RIGHT_PIN, HIGH); // Channel 1 from transmitter
  double ch2 = pulseIn(RC_LEFT_PIN, HIGH);  // Channel 2 from transmitter
  
  Serial.print("ch1: ");
  Serial.print(ch1);
  Serial.print("\tch2: ");
  Serial.println(ch2);
  
  // Speed mapping for drive motors (using ch1 for speed calculation)
  int spdFwd_RightLeft = map(ch1, Ch1Ch2_start_Fwd, Ch1Ch2_End_Fwd, 0, 255);
  int spdBac_RightLeft = map(ch1, Ch1Ch2_start_Bac, Ch1Ch2_End_Bac, 0, 255);
  
  // Enable drive motor drivers
  digitalWrite(RIGHT_R_EN, HIGH);
  digitalWrite(RIGHT_L_EN, HIGH);
  digitalWrite(LEFT_R_EN, HIGH);
  digitalWrite(LEFT_L_EN, HIGH);
  
  // Drive motor conditions:
  if ((ch1 == 0) && (ch2 == 0)) {     
    analogWrite(R_PWM_right, 0);
    analogWrite(R_LEFT_PWM_right, 0);
    analogWrite(R_PWM_left, 0);
    analogWrite(L_PWM_left, 0);
  }
  // Forward
  else if ((ch1 > Ch1Ch2_start_Fwd) && (ch2 > Ch1Ch2_start_Fwd)) {     
    analogWrite(R_PWM_right, spdFwd_RightLeft);
    analogWrite(R_LEFT_PWM_right, 0);
    analogWrite(R_PWM_left, spdFwd_RightLeft);
    analogWrite(L_PWM_left, 0); 
  }
  // Right turn
  else if ((ch1 > Ch1Ch2_start_Fwd) && (ch2 < Ch1Ch2_start_Bac)) { 
    analogWrite(R_PWM_right, 0);
    analogWrite(R_LEFT_PWM_right, spdFwd_RightLeft);
    analogWrite(R_PWM_left, spdFwd_RightLeft);
    analogWrite(L_PWM_left, 0);
  }
  // Left turn
  else if ((ch1 < Ch1Ch2_start_Bac) && (ch2 > Ch1Ch2_start_Fwd)) {     
    analogWrite(R_PWM_right, spdBac_RightLeft);
    analogWrite(R_LEFT_PWM_right, 0);
    analogWrite(R_PWM_left, 0);
    analogWrite(L_PWM_left, spdBac_RightLeft);
  }
  // Backward
  else if ((ch1 < Ch1Ch2_start_Bac) && (ch2 < Ch1Ch2_start_Bac)) {  
    analogWrite(R_PWM_right, 0);
    analogWrite(R_LEFT_PWM_right, spdBac_RightLeft); 
    analogWrite(R_PWM_left, 0);
    analogWrite(L_PWM_left, spdBac_RightLeft); 
  }
  else {     
    analogWrite(R_PWM_right, 0);
    analogWrite(R_LEFT_PWM_right, 0);
    analogWrite(R_PWM_left, 0);
    analogWrite(L_PWM_left, 0);
  }
  
  // --- Blade Motor Control with Toggle Mechanism ---
  // Read the blade receiver channel pulse width (timeout = 25000 µs)
  unsigned long rcBlade = pulseIn(RC_BLADE_PIN, HIGH, 25000);
  Serial.print("Blade: ");
  Serial.println(rcBlade);
  
  // Toggle mechanism:
  // When the blade channel pulse goes above 1700 µs and it wasn't pressed before,
  // toggle the blade state. When the pulse returns near neutral (<1450 µs), reset.
  if (rcBlade > 1700 && !bladeTogglePressed) {
    bladeTogglePressed = true;
    bladeEnabled = !bladeEnabled; // Toggle blade state
    Serial.print("Blade toggled: ");
    Serial.println(bladeEnabled ? "ON" : "OFF");
  }
  if (rcBlade < 1450) {
    bladeTogglePressed = false;
  }
  
  // Now, if the blade is enabled, run it at full speed (or you can adjust as needed).
  if (bladeEnabled) {
    driveForward(BLADE_RPWM, BLADE_LPWM, BLADE_REN, BLADE_LEN, 255);
  } else {
    stopMotor(BLADE_RPWM, BLADE_LPWM, BLADE_REN, BLADE_LEN);
  }
  
  delay(20);  // Small delay for stability
}
