
// SMART GREENHOUSE - INTERRUPT-DRIVEN AUTOMATIC CONTROL

// ANALOG SENSORS

const int SOIL_PIN  = A0;
const int LIGHT_PIN = A1;
const int TEMP_PIN  = A2;

// OUTPUTS

const int PUMP_LED  = 4;
const int LIGHT_LED = 5;
const int FAN_LED   = 6;
const int WARN_LED  = 7;
const int BUZZER    = 11;

// DIGITAL EVENT INPUTS


const int SOIL_EVENT_PIN = 8;
const int LIGHT_EVENT_PIN = 9;
const int TEMP_EVENT_PIN = 10;

// THRESHOLDS

const int SOIL_DRY_THRESHOLD = 400;
const int LIGHT_LOW_THRESHOLD = 400;

const float FAN_TEMPERATURE = 28.0;
const float WARNING_TEMPERATURE = 30.0;


// INTERRUPT FLAGS


volatile bool soilEventFlag = false;
volatile bool lightEventFlag = false;
volatile bool tempEventFlag = false;

volatile bool timerFlag = false;

volatile byte previousPortBState;
void setup() {

  Serial.begin(9600);

  pinMode(PUMP_LED, OUTPUT);
  pinMode(LIGHT_LED, OUTPUT);
  pinMode(FAN_LED, OUTPUT);
  pinMode(WARN_LED, OUTPUT);
  pinMode(BUZZER, OUTPUT);

  pinMode(SOIL_EVENT_PIN, INPUT_PULLUP);
  pinMode(LIGHT_EVENT_PIN, INPUT_PULLUP);
  pinMode(TEMP_EVENT_PIN, INPUT_PULLUP);

  // PIN CHANGE INTERRUPT CONFIGURATION
  
  previousPortBState = PINB;

  
  PCICR |= (1 << PCIE0);

  PCMSK0 |= (1 << PCINT0);
  PCMSK0 |= (1 << PCINT1);
  PCMSK0 |= (1 << PCINT2);


  // TIMER1 CONFIGURATION
  
  noInterrupts();

  TCCR1A = 0;
  TCCR1B = 0;

  // CTC mode
  TCCR1B |= (1 << WGM12);

  // Prescaler = 64
  TCCR1B |= (1 << CS11);
  TCCR1B |= (1 << CS10);

  OCR1A = 24999;


  TIMSK1 |= (1 << OCIE1A);

  interrupts();

  Serial.println("================================");
  Serial.println("   SMART GREENHOUSE SYSTEM");
  Serial.println("   INTERRUPT-DRIVEN MODE");
  Serial.println("================================");

  Serial.println();
  Serial.println("PCINT enabled: D8, D9, D10");
  Serial.println("Timer1 enabled");
  Serial.println("System starting...");
  Serial.println();
}

// MAIN LOOP

void loop() {

 

  if (soilEventFlag) {

    noInterrupts();
    soilEventFlag = false;
    interrupts();

    Serial.println("[PCINT] Soil event detected");

    processGreenhouse();
  }


  if (lightEventFlag) {

    noInterrupts();
    lightEventFlag = false;
    interrupts();

    Serial.println("[PCINT] Light event detected");

    processGreenhouse();
  }


  if (tempEventFlag) {

    noInterrupts();
    tempEventFlag = false;
    interrupts();

    Serial.println("[PCINT] Temperature event detected");

    processGreenhouse();
  }


  // ----------------------------------------------------------
  // Periodic Timer task
  // ----------------------------------------------------------

  if (timerFlag) {

    noInterrupts();
    timerFlag = false;
    interrupts();

    Serial.println();
    Serial.println("[TIMER] Periodic greenhouse monitoring");

    processGreenhouse();
  }
}


// ============================================================
// GREENHOUSE PROCESSING FUNCTION
// Sense → Think → Act
// ============================================================

void processGreenhouse() {

  // ==========================================================
  // SENSE
  // ==========================================================

  int soilValue = analogRead(SOIL_PIN);
  int lightValue = analogRead(LIGHT_PIN);
  int tempRaw = analogRead(TEMP_PIN);

  // TMP36 calculation
  float voltage = tempRaw * (5.0 / 1023.0);
  float temperatureC = (voltage - 0.5) * 100.0;


  // ==========================================================
  // DISPLAY SENSOR VALUES
  // ==========================================================

  Serial.println("--------------------------------");

  Serial.print("Soil Moisture: ");
  Serial.println(soilValue);

  Serial.print("Light Level: ");
  Serial.println(lightValue);

  Serial.print("Temperature: ");
  Serial.print(temperatureC);
  Serial.println(" C");

  // THINK + ACT
  // SOIL MOISTURE

  if (soilValue < SOIL_DRY_THRESHOLD) {

    digitalWrite(PUMP_LED, HIGH);

    Serial.println("STATUS: Soil is DRY");
    Serial.println("ACTION: Water Pump ON");

  } else {

    digitalWrite(PUMP_LED, LOW);

    Serial.println("STATUS: Soil moisture OK");
    Serial.println("ACTION: Water Pump OFF");
  }


  // THINK + ACT
  // LIGHT


  if (lightValue < LIGHT_LOW_THRESHOLD) {

    digitalWrite(LIGHT_LED, HIGH);

    Serial.println("STATUS: Low light detected");
    Serial.println("ACTION: Grow Light ON");

  } else {

    digitalWrite(LIGHT_LED, LOW);

    Serial.println("STATUS: Light level OK");
    Serial.println("ACTION: Grow Light OFF");
  }



  // THINK + ACT
  // TEMPERATURE


  if (temperatureC >= FAN_TEMPERATURE) {

    digitalWrite(FAN_LED, HIGH);

    Serial.println("STATUS: Temperature HIGH");
    Serial.println("ACTION: Fan ON");

  } else {

    digitalWrite(FAN_LED, LOW);

    Serial.println("STATUS: Temperature NORMAL");
    Serial.println("ACTION: Fan OFF");
  }


  // HIGH TEMPERATURE WARNING


  if (temperatureC >= WARNING_TEMPERATURE) {

    digitalWrite(WARN_LED, HIGH);

    tone(BUZZER, 1000);

    Serial.println("!!! WARNING !!!");
    Serial.println("Temperature is VERY HIGH!");
    Serial.println("Warning alarm ON");

  } else {

    digitalWrite(WARN_LED, LOW);

    noTone(BUZZER);
  }

  Serial.println("--------------------------------");
}


// PIN CHANGE INTERRUPT SERVICE ROUTINE
// PCINT0 VECTOR
//
// D8  = PB0
// D9  = PB1
// D10 = PB2



ISR(PCINT0_vect) {

  byte currentPortBState = PINB;

  // Detect changed pins
  byte changedPins = currentPortBState ^ previousPortBState;

  // D8 changed
  if (changedPins & (1 << PB0)) {

    // Trigger only when button is pressed
    if (!(currentPortBState & (1 << PB0))) {
      soilEventFlag = true;
    }
  }

  // D9 changed
  if (changedPins & (1 << PB1)) {

    if (!(currentPortBState & (1 << PB1))) {
      lightEventFlag = true;
    }
  }

  // D10 changed
  if (changedPins & (1 << PB2)) {

    if (!(currentPortBState & (1 << PB2))) {
      tempEventFlag = true;
    }
  }

  // Store current state for next interrupt
  previousPortBState = currentPortBState;
}



// TIMER1 COMPARE MATCH INTERRUPT

volatile byte timerCounter = 0;

ISR(TIMER1_COMPA_vect) {

  timerCounter++;

  if (timerCounter >= 10) {

    timerCounter = 0;

    timerFlag = true;
  }
}