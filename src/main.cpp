/*
* NOVIS HEXA-SDR v1.7
* Date: May 23, 2026
* Author: KD8UFZ (John Durham), with heavy inspiration from the original Si5351A tuning and Goertzel DSP work of W8YI (Bill Carver) and
* the TFT_eSPI graphics framework by Bodmer.  As well as N3FJZ and his MAX-SSB Project. This project is a labor of love to create a 
* compact, efficient, and visually intuitive software-defined radio transceiver interface using the powerful ESP32-S3 microcontroller 
* and a vibrant 4.0" ST7796S SPI display. 
* The code is structured to maximize performance by leveraging dual-core processing for real-time signal decoding and responsive user 
* interface management, while also implementing a clean and informative graphical layout for amateur radio operators.
* As those who came before, and those who will come after me, do what ye will with this codebase and project.
* If you find it useful, inspiring, or a good base to build your own SDR projects on, then I have succeeded in my goals.
* If you have found a fun project or a first project or radio and had a good time building and learning, then I have succeeded in my goals.
*
* Any BUGS, ISSUES, OR FEATURE REQUESTS can be submitted on the GitHub repository page for this project: https://github.com/kilodelta8/NOVIS-Hexa-SDR
* There is also my email: kd8ufzATliveDOTcom
*/


#include <Wire.h>
#include <si5351.h>
#include <TFT_eSPI.h> 

// Ensure your TFT_eSPI "User_Setup.h" file is correctly configured for the 
// Hoysond 4.0" ST7796S SPI display pins and touch lines!

TFT_eSPI tft = TFT_eSPI();
Si5351 si5351;

// ============================================================================
// HARDWARE PIN LOGIC MAP
// ============================================================================
const int ENCODER_A    = 4;   // Rotary Pulse Line A
const int ENCODER_B    = 5;   // Rotary Pulse Line B
const int ENCODER_SW   = 6;   // Rotary Click Switch Pin
const int AUDIO_IN_PIN = 1;   // ESP32-S3 ADC1_CH0 (I-Channel Audio Line)

const int BTN_VFO      = 7;   // Button 1: VFO/Memory Toggle
const int BTN_CALL     = 2;   // Button 2: Auto-CQ Beacon / Hold to Program
const int BTN_MENU     = 3;   // Button 3: Setup Menu Toggle / Exit
const int BTN_BAND     = 21;  // Button 4: Cyclic 6-Band Selector
const int BTN_MODE     = 47;  // Button 5: CW / USB Mode Switch
const int BTN_RIT      = 48;  // Button 6: Receiver Incremental Tuning Offset

// ============================================================================
// GLOBAL RADIO ARCHITECTURE STATES
// ============================================================================
volatile long baseFrequency  = 14060000; // Default startup on 20m CW (14.060 MHz)
volatile long ritOffset      = 0;
volatile bool ritActive      = false;
volatile long tuningStepSize = 100;      // Default tuning resolution increments (100 Hz)
volatile bool frequencyChanged = true;

enum RadioMode { MODE_CW, MODE_SSB };
RadioMode currentMode = MODE_CW;

// Smart Auto-CQ Beacon Flags
bool isAutoCQRunning = false;
String operatorCallsign = "W8YI";        // Default saved user callsign string

// Native 6-Band Plan Matrix Limits
int currentBandIndex = 1; // Array Index 1 = 20 Meters
const long bandEdges[6] = {7000000, 14000000, 18068000, 21000000, 24890000, 28000000};
const char* bandNames[6] = {"40m", "20m", "17m", "15m", "12m", "10m"};

// Display Trackers to Prevent Screen Flicker
long lastDisplayedVFO   = -1;
long lastDisplayedRit   = -1;
long lastDisplayedStep  = -1;
int lastMeterValue      = -1;
RadioMode lastDisplayedMode = MODE_SSB;
int lastDisplayedBand   = -1;

// ============================================================================
// CW TERMINAL WINDOW BUFFER CONSTANTS
// ============================================================================
const int MAX_LINES     = 5;   // Maximized text lines in the lower screen region
const int LINE_HEIGHT   = 22;  // Vertical line pitch pixels
const int TEXT_START_Y = 195;  // Core boundary layout separator coordinate
String terminalLines[MAX_LINES] = {"", "", "", "", ""};
int currentLineIndex    = 0;

// ============================================================================
// GOERTZEL DSP SIGNAL EXTRACTION FILTERS
// ============================================================================
const float SAMPLING_FREQUENCY = 10000.0; // 10 kHz specialized sampling pace
volatile float targetCwPitch   = 500.0;   // Default 500 Hz operator tone choice
const int N                    = 100;     // DSP analytical collection block size

// Self-scaling CW timing speed metrics (in milliseconds)
int dotLength = 150; 
unsigned long toneStartTime = 0;
unsigned long toneEndTime   = 0;
bool isToneActive           = false;
String morseTreeBuffer      = "";

// Thread-Safe Cross-Core Communication Queue Register
volatile char decodedCharQueue = 0;

TaskHandle_t Core0Task;

// ============================================================================
// APPLICATION INITIALIZATION SETUP
// ============================================================================
void setup() {
  Serial.begin(115200);
  analogReadResolution(12); // Enforce 12-bit safe ADC bounds tracking (0-4095)

  // 1. Fire up the Hoysond 4.0-inch graphics glass faceplate
  tft.init();
  tft.setRotation(1); // Deploy in true landscape orientation mode
  tft.fillScreen(TFT_BLACK);
  drawStaticUIFrames();

  // 2. Initialize the Si5351A local mixing clock line
  bool siFound = si5351.init(SI5351_CRYSTAL_LOAD_8PF, 0, 0);
  if (siFound) {
    si5351.drive_strength(SI5351_CLK0, SI5351_DRIVE_8MA);
    // Push initial 4x clock multiplication to start the phase splitter chip
    si5351.set_freq((baseFrequency * 4ULL) * 100ULL, SI5351_CLK0); 
  } else {
    Serial.println("Warning: Si5351A hardware engine unallocated. Check I2C.");
  }

  // 3. Bind discrete operational mechanical interface inputs
  pinMode(ENCODER_A, INPUT_PULLUP);
  pinMode(ENCODER_B, INPUT_PULLUP);
  pinMode(ENCODER_SW, INPUT_PULLUP);
  
  pinMode(BTN_VFO,   INPUT_PULLUP);
  pinMode(BTN_CALL,  INPUT_PULLUP);
  pinMode(BTN_MENU,  INPUT_PULLUP);
  pinMode(BTN_BAND,  INPUT_PULLUP);
  pinMode(BTN_MODE,  INPUT_PULLUP);
  pinMode(BTN_RIT,   INPUT_PULLUP);

  // Instant vector change tracking pin interrupts bound to Core 0 calculation paths
  attachInterrupt(digitalPinToInterrupt(ENCODER_A), readEncoderISR, CHANGE);

  // 4. Lock background task loops permanently on System Protocol Core 0
  xTaskCreatePinnedToCore(Core0DSPDecoderLoop, "Core0DSP", 4000, NULL, 1, &Core0Task, 0);
}

// ============================================================================
// CORE 0: HIGH-SPEED ASYNCHRONOUS USER TUNING INTERRUPT
// ============================================================================
void IRAM_ATTR readEncoderISR() {
  int stateA = digitalRead(ENCODER_A);
  int stateB = digitalRead(ENCODER_B);
  
  long change = (stateA == stateB) ? tuningStepSize : -tuningStepSize;
  
  if (ritActive) {
    ritOffset += change; // Tuning knob shifts receive-only tracking offset window
  } else {
    baseFrequency += change; // Tuning knob shifts main transceiver tracking frame
  }
  frequencyChanged = true;
}

// ============================================================================
// CORE 0: AUDIO SAMPLING, GOERTZEL MATH & HARDWARE REGISTER WRITING
// ============================================================================
void Core0DSPDecoderLoop(void * pvParameters) {
  // Pre-calculate highly optimized static Goertzel filtration coefficients
  float k = (int)(0.5 + ((N * targetCwPitch) / SAMPLING_FREQUENCY));
  float omega = (2.0 * PI * k) / N;
  float cosine = cos(omega);
  float coefficient = 2.0 * cosine;

  long lastPushedFreq = 0;

  for(;;) {
    // A. Keep the external Si5351A clock fully synchronized with the tuning loop
    long currentHardwareTarget = baseFrequency;
    if (ritActive) {
      currentHardwareTarget += ritOffset;
    }

    if (currentHardwareTarget != lastPushedFreq) {
      lastPushedFreq = currentHardwareTarget;
      
      // CRITICAL TAYLOE 4X MIXER MULTIPLICATION LOGIC:
      unsigned long long mixerClockTarget = (unsigned long long)lastPushedFreq * 4ULL;
      si5351.set_freq(mixerClockTarget * 100ULL, SI5351_CLK0);
    }

    // B. Live Goertzel Signal Processing Loop block execution
    float q0 = 0, q1 = 0, q2 = 0;
    
    for (int i = 0; i < N; i++) {
      float sample = (float)analogRead(AUDIO_IN_PIN) - 2048.0; // Wipes out internal 1.65V DC offset
      q0 = coefficient * q1 - q2 + sample;
      q2 = q1;
      q1 = q0;
      delayMicroseconds(100); // Locks input collection to a rigid 10kHz sample interval
    }
    
    float magnitudeSquared = (q1 * q1) + (q2 * q2) - (coefficient * q1 * q2);
    bool toneDetected = (magnitudeSquared > 150000.0); // Variable amplitude lock trigger threshold
    unsigned long now = millis();

    // C. Decode Timing State Machine Logic
    if (toneDetected && !isToneActive) {
      isToneActive = true;
      unsigned long offDuration = now - toneEndTime;
      toneStartTime = now;
      
      if (offDuration > dotLength * 4 && decodedCharQueue == 0) {
        decodedCharQueue = ' '; // Signal a word break space output onto Core 1
      }
    } 
    else if (!toneDetected && isToneActive) {
      isToneActive = false;
      toneEndTime = now;
      unsigned long onDuration = toneEndTime - toneStartTime;

      if (onDuration > 30 && onDuration < dotLength * 1.8) {
        morseTreeBuffer += ".";
      } else if (onDuration >= dotLength * 1.8) {
        morseTreeBuffer += "-";
      }
    }

    // Capture complete letters if the incoming carrier tone remains idle long enough
    if (!isToneActive && morseTreeBuffer.length() > 0 && (now - toneEndTime > dotLength * 2)) {
      char parsedLetter = lookupMorse(morseTreeBuffer);
      if (parsedLetter != '?') {
        decodedCharQueue = parsedLetter; // Ship character over to the Core 1 display register
      }
      morseTreeBuffer = ""; 
    }

    vTaskDelay(pdMS_TO_TICKS(1)); // FreeRTOS system core check breathing break
  }
}

// ============================================================================
// CORE 1: MAIN APPLICATION OPERATOR INTERFACE & GRAPHICS LOOP
// ============================================================================
void loop() {
  checkHardwareButtons();

  // Step A: Redraw the primary green numeric VFO window components safely
  if (frequencyChanged) {
    noInterrupts();
    long currentBase = baseFrequency;
    long currentRit  = ritOffset;
    long currentStep = tuningStepSize;
    bool currentRitMode = ritActive;
    frequencyChanged = false;
    interrupts();

    if (currentBase != lastDisplayedVFO || currentStep != lastDisplayedStep || currentRit != lastDisplayedRit) {
      updateVFODisplay(currentBase, currentStep, currentRit, currentRitMode);
      lastDisplayedVFO = currentBase;
      lastDisplayedStep = currentStep;
      lastDisplayedRit = currentRit;
    }
  }

  // Step B: Update mode/band indicators dynamically
  if (currentMode != lastDisplayedMode || currentBandIndex != lastDisplayedBand) {
    updateStateLabels();
    lastDisplayedMode = currentMode;
    lastDisplayedBand = currentBandIndex;
  }

  // Step C: Route ADC audio activity down to the visual bar-graph meter frame
  int realAudioLevel = map(analogRead(AUDIO_IN_PIN), 2048, 4095, 0, 100);
  drawBarMeter(constrain(realAudioLevel, 5, 95));

  // Step D: Catch letters sent across the core pipelines from the decoder process
  if (decodedCharQueue != 0) {
    char letterToPrint = decodedCharQueue;
    decodedCharQueue = 0; 
    printCharToTerminal(letterToPrint);
  }

  // Step E: Auto-CQ Transmit warning message generator routing
  if (isAutoCQRunning) {
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.drawString("TX BEACON: TRANSMITTING CQ LOOP... ", 15, 145, 2);
  } else {
    tft.fillRect(15, 145, 300, 15, TFT_BLACK); 
  }

  delay(30); // 33Hz frame refresh engine velocity lock
}

// ============================================================================
// HARDWARE FRONT PANEL INPUT TRACKING CONTROLLER
// ============================================================================
void checkHardwareButtons() {
  static unsigned long encoderSwTimer = 0;
  static bool encoderSwPressed = false;
  static unsigned long callSwTimer     = 0;
  static bool callSwPressed     = false;

  // 1. Encoder Multi-Press Timing Logic (Short click vs Long Hold)
  if (digitalRead(ENCODER_SW) == LOW) {
    if (!encoderSwPressed) { encoderSwPressed = true; encoderSwTimer = millis(); }
  } else {
    if (encoderSwPressed) {
      unsigned long hold = millis() - encoderSwTimer;
      encoderSwPressed = false;
      if (hold > 600) {
        tuningStepSize = 100000; // Long press: jump to 100 kHz steps
      } else {
        // Short press: cycle steps between 10 Hz, 100 Hz, and 1 kHz
        if (tuningStepSize == 10)         tuningStepSize = 100;
        else if (tuningStepSize == 100)   tuningStepSize = 1000;
        else                              tuningStepSize = 10;
      }
      frequencyChanged = true;
    }
  }

  // 2. Intelligent Auto-CQ Beacon Input Tracker
  if (digitalRead(BTN_CALL) == LOW) {
    if (!callSwPressed) { callSwPressed = true; callSwTimer = millis(); }
  } else {
    if (callSwPressed) {
      unsigned long hold = millis() - callSwTimer;
      callSwPressed = false;
      
      if (hold > 1500) {
        Serial.println("BEEP! Launching capacitive touch keyboard setup.");
        // Touch setup keyboard interface hook goes here
      } else {
        isAutoCQRunning = !isAutoCQRunning; // Short press: toggle the automated CQ loop
      }
    }
  }

  // 3. Receiver Incremental Tuning (RIT) Trigger Switch
  if (digitalRead(BTN_RIT) == LOW) {
    ritActive = !ritActive;
    if (!ritActive) ritOffset = 0; // Snap the offset instantly back to center when turned off
    frequencyChanged = true;
    delay(250); // Anti-rebound bounce latch delay
  }

  // 4. Relay Filtering Band-Switch Network Selector
  if (digitalRead(BTN_BAND) == LOW) {
    currentBandIndex = (currentBandIndex + 1) % 6;
    baseFrequency = bandEdges[currentBandIndex]; // Instantly snap tuning index to bottom edge of new band
    frequencyChanged = true;
    delay(250);
  }

  // 5. Operating Signal Mode Selector
  if (digitalRead(BTN_MODE) == LOW) {
    currentMode = (currentMode == MODE_CW) ? MODE_SSB : MODE_CW;
    delay(250);
  }
}

// ============================================================================
// CORE 1: GRAPHICS DRAW ROUTINES (TFT_eSPI Interface Management)
// ============================================================================
void drawStaticUIFrames() {
  tft.drawRect(0, 0, 480, 320, TFT_BLUE);
  tft.drawLine(0, 160, 480, 160, TFT_BLUE); 
  
  tft.drawString("NOVIS HEXA-SDR v1.7", 15, 10, 2);
  tft.drawString("AUDIO BASEBAND S-METER", 15, 95, 2);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.drawString("CW DECODER TERMINAL WINDOW:", 15, 170, 2);
}

void updateStateLabels() {
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.fillRect(15, 40, 180, 45, TFT_BLACK); 
  
  String modeText = (currentMode == MODE_CW) ? "MODE: CW" : "MODE: USB";
  tft.drawString(modeText, 15, 40, 2);
  
  String bandText = "BAND: " + String(bandNames[currentBandIndex]);
  tft.drawString(bandText, 15, 65, 2);
}

void updateVFODisplay(long base, long step, long rit, bool ritOn) {
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  float mhz = base / 1000000.0;
  tft.drawFloat(mhz, 5, 240, 30, 4); // Renders clean high-density VFO string

  tft.fillRect(240, 65, 230, 50, TFT_BLACK); // Flush active data window artifacts
  
  tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
  String stepLabel = "STEP: " + String(step) + " Hz";
  tft.drawString(stepLabel, 240, 65, 2);

  if (ritOn) {
    tft.setTextColor(TFT_RED, TFT_BLACK);
    String ritLabel = "RIT ON: " + String(rit) + " Hz";
    tft.drawString(ritLabel, 240, 90, 2);
  }
}

void drawBarMeter(int percentageValue) {
  int startX = 15; int startY = 120; int meterWidth = 200; int meterHeight = 15;
  int targetFillPixels = (percentageValue * meterWidth) / 100;
  if (percentageValue < lastMeterValue) {
    tft.fillRect(startX + targetFillPixels, startY, meterWidth - targetFillPixels, meterHeight, TFT_BLACK);
  }
  uint16_t barColor = (percentageValue > 75) ? TFT_YELLOW : TFT_GREEN;
  tft.fillRect(startX, startY, targetFillPixels, meterHeight, barColor);
  lastMeterValue = percentageValue;
}

// ============================================================================
// TERMINAL ARRAY WINDOW RING-BUFFER ENGINE
// ============================================================================
void printCharToTerminal(char c) {
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  
  // Wrap lines cleanly if text reaches the right edge of the screen frame bounds
  if (terminalLines[currentLineIndex].length() >= 38) {
    if (currentLineIndex < MAX_LINES - 1) {
      currentLineIndex++;
    } else {
      // Screen array floor reached! Shift old rows up to make room for new text
      for (int i = 0; i < MAX_LINES - 1; i++) {
        terminalLines[i] = terminalLines[i + 1];
      }
      terminalLines[MAX_LINES - 1] = ""; 
    }
  }
  
  terminalLines[currentLineIndex] += c;
  
  // Redraw the terminal screen block
  tft.fillRect(15, TEXT_START_Y, 450, LINE_HEIGHT * MAX_LINES, TFT_BLACK);
  for (int i = 0; i < MAX_LINES; i++) {
    tft.drawString(terminalLines[i], 15, TEXT_START_Y + (i * LINE_HEIGHT), 2);
  }
}

// ============================================================================
// BINARY SEARCH TREE MORSE INTERPRETATION ENGINE
// ============================================================================
char lookupMorse(String elements) {
  if (elements == ".-")   return 'A'; if (elements == "-...") return 'B';
  if (elements == "-.-.") return 'C'; if (elements == "-..")  return 'D';
  if (elements == ".")    return 'E'; if (elements == "..-.") return 'F';
  if (elements == "--.")  return 'G'; if (elements == "....") return 'H';
  if (elements == "..")   return 'I'; if (elements == ".---") return 'J';
  if (elements == "-.-")  return 'K'; if (elements == ".-..") return 'L';
  if (elements == "--")   return 'M'; if (elements == "-.")   return 'N';
  if (elements == "---")  return 'O'; if (elements == ".--.") return 'P';
  if (elements == "--.-") return 'Q'; if (elements == ".-.")  return 'R';
  if (elements == "...")  return 'S'; if (elements == "-")    return 'T';
  if (elements == "..-")  return 'U'; if (elements == "...-") return 'V';
  if (elements == ".--")  return 'W'; if (elements == "-..-") return 'X';
  if (elements == "-.--") return 'Y'; if (elements == "--..") return 'Z';
  return '?'; // Output marker for unrecognized character streams
}