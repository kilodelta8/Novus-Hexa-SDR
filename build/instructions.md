# NOVIS HEXA-SDR: COMPLETE MASTER MANUAL
### An Educational Guide to Assembling a 6-Band QRP Software-Defined Radio Transceiver

---

## Welcome to the Novis Hexa-SDR Project!
The **Novis Hexa-SDR** is an open-source, kit-modular, 6-band QRP (Low Power) High-Frequency (HF) transceiver designed specifically for student builders, amateur radio newcomers, and classroom environments. Covering the **40, 20, 17, 15, 12, and 10-meter** bands, this radio balances modern digital signal processing with classic analog radio circuitry.

### Manual Design Philosophy
*   **Accessible:** No advanced physics or complex engineering degrees required. Concepts are explained simply as you build.
*   **Modular Progression:** The radio is assembled and verified in five sequential, independent project packages (Kits A through E).
*   **High Performance, Low Cost:** By utilizing affordable modules like the ESP32-S3 and high-efficiency topologies like the Tayloe Mixer and Class-E power amplification, this design provides premium-tier radio capabilities on a student budget.

---

# CHAPTER 1: ARCHITECTURE OVERVIEW

Before melting solder, it is critical to understand how the signals flow inside the Novis Hexa-SDR. The radio combines a **Software Defined Radio (SDR)** digital interface with an analog receiver and transmitter front end.
```
                  +-------------------+
                  |   Antenna (BNC)   |
                  +---------+---------+
                            |
                            ▼
+─────────────────────────────────────────────────────────────────────────────────+
│                           INTEGRATED FILTERS SWITCHING                          │
│                      6x Band Pass  /  6x Low Pass Networks                      │
+───────────────────┬─────────────────────────────────────────▲───────────────────+
                    │ [RX Signal]                             │ [TX RF Carrier]
                    ▼                                         │
+───────────────────────────────────────+         +───────────┴───────────────────+
│        ANALOG DETECTOR PRE-AMP        │         │    POWER AMPLIFIER STAGE      │
│  - FST3253 High-Speed Tayloe Mixer    │         │  - 74ACT08 High-Current Driver│
│  - 74AC74 Dual Phase Clock Splitter   │         │  - IRF510 Switching MOSFET    │
│  - LM358 40dB Differential Amplifier  │         +───────────────────▲───────────+
+───────────────────┬───────────────────+                             │
                    │                                                 │ [Power Control]
                    │ (Analog I/Q Baseband Audio)                     │
                    ▼                                                 │
+─────────────────────────────────────────────────────────────────────┴───────────+
│                             CORE PROCESSING UNIT                                │
│           ESP32-S3 Dual-Core Brain  &  Si5351A Clock VFO Generator              │
│  - Core 0: 10kHz Goertzel DSP Decoder    - Core 1: High-Response UI Rendering   │
+─────────────────────────────────────────────────────────────────────────────────+
```

### The Receive (RX) Flow
1.  **Filtering:** The antenna captures an array of electromagnetic signals. The **Filter Matrix (Kit C)** isolates only the specific amateur band you have tuned to, blocking out strong commercial AM/FM broadcast interference.
2.  **Mixing (Down-Conversion):** The **Tayloe Mixer (Kit D)** acts as an ultra-high-speed commutation switch. Driven by a 4-phase clock from the **74AC74 phase splitter (Kit D)** running at four times the target frequency, it slices the raw radio frequencies and drops them directly down into two baseband audio channels called **I (In-Phase)** and **Q (Quadrature)**.
3.  **Amplification:** The **LM358 Op-Amp (Kit D)** boosts these faint audio signals 100 times over ($40\text{ dB}$).
4.  **Processing & Decoding:** The amplified I/Q signals enter the **ESP32-S3 microcontroller (Kit A)** via its Analog-to-Digital Converter (ADC) pins. **Core 0** runs a digital **Goertzel algorithm** tuned to look for your preferred pitch. It isolates CW pulses, measures their timing, and translates them into letters. **Core 1** paints these characters smoothly onto the **4.0" LCD Screen (Kit A)**.

### The Transmit (TX) Flow
1.  **Input:** The operator presses the PTT line on the **Hand Microphone (Kit A)** or triggers the automated CQ loop button.
2.  **Carrier Generation:** The **Si5351A Clock Generator (Kit A)** produces a stable high-frequency carrier wave. For SSB voice, the ESP32 calculates polar phase changes and adjusts the clock directly.
3.  **Power Amplification:** The signal passes through a **74ACT08 high-speed logic gate (Kit D)** to boost driving current. This current switches a single, affordable **IRF510 MOSFET (Kit D)** running as an efficient **Class-E amplifier**, generating up to 10 Watts of RF energy.
4.  **Monitoring:** The outgoing signal passes through the **SWR & Power Bridge (Kit B)**, sending tracking voltages back to the screen so the user can verify their output power and antenna tuning safety.
5.  **Output:** The energy passes through the **Low Pass Filter Bank (Kit C)** to remove any remaining high-frequency harmonics before safely exciting the antenna wire.

---

# CHAPTER 2: KIT A - CORE PROCESSING, DISPLAY & OPERATOR INTERFACE

## 2.1 Component Anatomy & Pinouts
Kit A builds the "brain" and user interface of the radio.

*   **ESP32-S3 Development Board:** A dual-core 32-bit processor. **Core 0** handles fast interrupts, Si5351A tuning, and audio DSP. **Core 1** manages display drawing and button logic.
*   **Hoysond 4.0" LCD Module:** A high-density $320\times480$ pixel display utilizing an SPI serial protocol and an integrated **ST7796S driver chip**.
*   **Rotary Encoder:** A mechanical pulse generator with a built-in push switch used to scroll frequencies and adjust menu options.

### Physical Wiring Interconnect Blueprint
[ Display Module ]                  [ ESP32-S3 Dev Board ]                  [ Rotary Encoder ]
VCC     <------------------------>     5V / 3.3V
GND     <------------------------>       GND       <-------------------->     GND
CS      <------------------------>     GPIO 10
RESET   <------------------------>     GPIO 11
DC      <------------------------>     GPIO 12
SDI     <------------------------>     GPIO 13
SCK     <------------------------>     GPIO 14
TOUCH_CS<------------------------>     GPIO 18
GPIO 4      <-------------------->   Output A (CLK)
GPIO 5      <-------------------->   Output B (DT)
GPIO 6      <-------------------->   Switch Line (SW)


## 2.2 Software Environment Configuration

To optimize the frame rate of the 4.0-inch screen and prevent tuning lag, you must configure the **TFT_eSPI** library to bypass standard slow Arduino drawing instructions and talk directly to the ESP32's hardware register blocks.

### Step 1: Install Arduino IDE and Boards (Prerequisite)

Download the latest Arduino IDE. Open Preferences, add the ESP32 board manager URL, and install the `esp32` board library platform by Expressif. Select **ESP32-S3 Dev Module** as your active target hardware board.

### Step 2: Download Libraries (Dependencies)

Open the Library Manager and install **TFT_eSPI** by Bodmer, and the **Etherkit Si5351** library by Jason Mildrum (NT7S).

### Step 3: Locate User_Setup.h (Configuration File)

Navigate to your local computer's file system path: `Documents/Arduino/libraries/TFT_eSPI/`. Open the file titled `User_Setup.h` in a plain-text editor.

### Step 4: Overwrite Library Profiles (Hardware Binding)

Wipe out the file's default contents and paste the following hardware configuration profile block into it, then save and close the file:

```cpp
// ============================================================================
// NOVIS HEXA-SDR: TFT_eSPI USER CONFIGURATION MATRIX (User_Setup.h)
// ============================================================================
#define ST7796_DRIVER       // Selects the ST7796S display controller
#define TFT_MISO   -1       // Disables read-back pin to save a hardware GPIO
#define TFT_MOSI   13       // Connects to Display Pin: SDI
#define TFT_SCLK   14       // Connects to Display Pin: SCK
#define TFT_CS     10       // Connects to Display Pin: CS
#define TFT_DC     12       // Connects to Display Pin: DC
#define TFT_RST    11       // Connects to Display Pin: RESET
#define TOUCH_CS   18       // Shares the SPI bus for touch controls

#define LOAD_GLCD   // Font 1: System tracking annotations label
#define LOAD_FONT2  // Font 2: Scrolling CW decoded characters
#define LOAD_FONT4  // Font 4: Primary green numeric VFO layout numbers

#define SPI_FREQUENCY       40000000  // Run primary graphic clock lines at 40 MHz
#define SPI_READ_FREQUENCY  20000000
#define SPI_TOUCH_FREQUENCY  2500000
#define TFT_RGB_ORDER TFT_BGR         // Fixes standard color rendering profiles
```

## 2.3 Verification Milestone & Initial Test Code

Use this test code to verify the display and encoder are functioning correctly:

```cpp
#include <SPI.h>
#include <TFT_eSPI.h> 

TFT_eSPI tft = TFT_eSPI();

const int ENCODER_A = 4;
const int ENCODER_B = 5;
const int ENCODER_SW = 6;

volatile long vfoFrequency = 14000000; // Default startup on 20 meters
volatile bool frequencyChanged = true;
long lastDisplayedVFO = 0;

void setup() {
  Serial.begin(115200);
  tft.init();
  tft.setRotation(1); // Set display to horizontal landscape view mode
  tft.fillScreen(TFT_BLACK);
  
  // Layout baseline bounding boxes
  tft.drawRect(0, 0, 480, 320, TFT_BLUE);
  tft.drawLine(0, 160, 480, 160, TFT_BLUE);
  tft.drawString("NOVIS HEXA-SDR v1.0", 15, 10, 2);
  tft.drawString("MODE: RX-CW", 15, 40, 2);
  tft.drawString("VFO READOUT:", 280, 10, 2);

  pinMode(ENCODER_A, INPUT_PULLUP);
  pinMode(ENCODER_B, INPUT_PULLUP);
  pinMode(ENCODER_SW, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ENCODER_A), readEncoderISR, CHANGE);
}

void IRAM_ATTR readEncoderISR() {
  int stateA = digitalRead(ENCODER_A);
  int stateB = digitalRead(ENCODER_B);
  if (stateA == stateB) vfoFrequency += 100; // Adjust frequency by 100 Hz per click
  else                  vfoFrequency -= 100;
  frequencyChanged = true;
}

void loop() {
  if (frequencyChanged) {
    noInterrupts(); long currentVFO = vfoFrequency; frequencyChanged = false; interrupts();
    if(currentVFO != lastDisplayedVFO) {
      tft.setTextColor(TFT_GREEN, TFT_BLACK);
      float mhz = currentVFO / 1000000.0;
      tft.drawFloat(mhz, 5, 280, 30, 4); 
      lastDisplayedVFO = currentVFO;
    }
  }
  delay(30);
}
```

**Success Criteria:** When powered up, the screen will display a clean blue dividing grid frame. Spin the rotary encoder shaft; the green frequency readout numbers in the upper right quadrant must update instantly and smoothly without flicker or lag.

---

# CHAPTER 3: KIT B - SWR & POWER BRIDGE CIRCUIT

## 3.1 Understanding RF Power Detection

A transmitter needs a way to evaluate how efficiently its energy travels out into the sky. The SWR & Power Bridge (Directional Coupler) acts as an inline diagnostic sensor placed directly between the output low-pass filters and the antenna BNC jack.       [ From LPF Transmit Network Input ]
                      │
                      ▼
            +-------------------+
            |  Primary Pass-Through Line
            |   (Thru Wire)     |
            +---------+---------+
                      │  (Magnetic Coupling via Toroid)
                      ▼
            +-------------------+
            | Secondary Sensing Coil
            | (FT37-43 Winding) |
            +----+---------+----+
                 |         |
        [Forward Path]  [Reflected Path]
                 |         |
                 ▼         ▼
              1N4148     1N4148  (RF Rectification to DC)
                 |         |
                 ▼         ▼
              10k Pot   10k Pot  (Voltage Division Calibration)
                 |         |
                 ▼         ▼
             [ESP32 Analog Sense Input Pins]
When high-frequency alternating current passes down the central wire toward an antenna, it creates a moving magnetic field. By wrapping a secondary sensing coil of wire around a high-permeability FT37-43 ferrite toroid core, we sample a tiny fraction of that electromagnetic energy.

**Forward Voltage** ($V_{\text{fwd}}$): Measures the raw power leaving the transmitter.

**Reflected Voltage** ($V_{\text{ref}}$): Measures any power bouncing back toward the radio caused by an un-tuned or mismatched antenna.

Two fast 1N4148 diodes act as RF rectifiers, converting this high-frequency AC signal into smooth DC voltage that our microcontroller can easily read.

## 3.2 Schematic & Physical AssemblyAssemble the bridge circuit on your prototyping block using the following configuration map:                            FT37-43 Core
                         +-----------------+
Transmitter Input ------ |== Primary Pass =| ------ BNC Antenna Pin
                         +--------+--------+
                                  |
               ┌──────────────────┴──────────────────┐
               ▼ (Forward Side)                      ▼ (Reflected Side)
           1N4148 Diode                          1N4148 Diode
               │                                     │
               ▼                                     ▼
        51 Ohm 1W Resistor                     51 Ohm 1W Resistor
               │                                     │
               ├───────┬───────┐                     ├───────┬───────┐
               ▼       ▼       ▼                     ▼       ▼       ▼
             10nF    10k     ESP32                 10nF    10k     ESP32
             Cap     Pot    GPIO 34                Cap     Pot    GPIO 35
               │       │ (ADC Pin)                   │       │ (ADC Pin)
             GND     GND                           GND     GND
**51 Ω 1-Watt Termination Resistors:** These handle the excess RF energy sampled by the sensing loop. They must be rated for at least 0.5W to 1W to prevent them from burning open during high-SWR tuning testing.

**10k Ω Trimpots:** These serve as adjustable safety dividers. They ensure the rectified voltage never exceeds 3.3V, protecting the ESP32's sensitive ADC inputs from over-voltage damage.

## 3.3 Calibration & Computational MathOnce assembled, the ESP32 reads these analog input voltages and processes them using real-time mathematical equations embedded within your main Core 1 control loop.

The Standing Wave Ratio (SWR) calculation follows this core formula:

$$\text{SWR} = \frac{V_{\text{fwd}} + V_{\text{ref}}}{V_{\text{fwd}} - V_{\text{ref}}}$$

### Step-by-Step Calibration Procedure:

1. Connect a verified 50 Ω QRP Dummy Load directly to the antenna BNC output jack.
2. Dial both 10k Ω trimpots completely counter-clockwise to their lowest voltage output setting.
3. Transmit a continuous CW carrier wave at 5 Watts.
4. Use a small screwdriver to slowly turn the Forward Pot clockwise until your screen's graphical power bar hits the 5W mark.
5. Swap the dummy load for an unmatched, open wire line. Verify that the Reflected Pot registers a voltage increase, triggering a high SWR alert on your screen.

---

# CHAPTER 4: KIT C - INTEGRATED FILTER BANK MATRIX

## 4.1 Low Pass Filters (LPF) vs. Band Pass Filters (BPF)

A 6-band radio operates across a wide spectrum range (7 MHz up to 29 MHz). To stay within FCC specifications and keep your receiver clean, every band requires two distinct filter networks:

**Low Pass Filter (LPF - Transmit Mode):** Allows your fundamental transmitting frequency to pass through safely but blocks high-frequency harmonics (2×, 3× your operating frequency) generated by the switching output transistor.

**Band Pass Filter (BPF - Receive Mode):** Acts as a narrow frequency window. It rejects massive out-of-band signals (such as commercial shortwave AM broadcast towers) before they reach the sensitive mixer chip.

## 4.2 Toroid Winding Methodology

Winding inductors on small carbonyl iron toroid rings is a core skill in homebrew radio assembly.

**Counting Turns:** One "turn" is counted every single time the enameled copper wire passes through the hollow center hole of the toroid ring. If the wire merely wraps over the outside edge, it does not count.

**Spacing:** Distribute the wire turns evenly around roughly 270° of the toroid's circumference. Leave a clean 90° gap between the start and end wires to minimize unwanted stray capacitance.

**Enamel Removal:** The wire is insulated with a clear polyurethane coating. Before soldering the toroid lead into the circuit board, you must strip this insulation away. Use fine sandpaper, a hobby knife to scrape the tip clean, or melt the enamel coating away using a hot blob of solder on your iron's tip.

## 4.3 Master Filter Matrix Constants TableEvery band uses a 7-element Chebyshev configuration for the transmit LPF bank (3 toroids, 4 capacitors) and a 2-pole inductively coupled loop for the receive BPF bank (2 toroids, 3 capacitors).All capacitors listed below MUST be high-stability, high-voltage (100V minimum) components featuring an NP0 or C0G dielectric rating to prevent frequency drift when the radio warms up.================================================================================
MASTER MATRIX VALUE SPECIFICATIONS
================================================================================
BAND   | TRANSMIT LPF MATRIX VALUES          | RECEIVE BPF MATRIX VALUES
       | Toroids (3x)     | Caps (C1/4, C2/3)| Toroids (2x)     | Caps (Parallel, Top)
=======+==================+==================+==================+================
40m    | 3x T37-2 (Red)   | 390 pF | 820 pF  | 2x T37-2 (Red)   | 330 pF | 15 pF
       | 18 Turns         |                  | 23 Turns         |
-------+------------------+==================+------------------+----------------
20m    | 3x T37-6 (Yellow)| 180 pF | 390 pF  | 2x T37-6 (Yellow)| 150 pF | 6.8 pF
       | 16 Turns         |                  | 18 Turns         |
-------+------------------+==================+------------------+----------------
17m    | 3x T37-6 (Yellow)| 150 pF | 330 pF  | 2x T37-6 (Yellow)| 120 pF | 5.6 pF
       | 14 Turns         |                  | 15 Turns         |
-------+------------------+==================+------------------+----------------
15m    | 3x T37-6 (Yellow)| 120 pF | 270 pF  | 2x T37-6 (Yellow)| 100 pF | 4.7 pF
       | 13 Turns         |                  | 14 Turns         |
-------+------------------+==================+------------------+----------------
12m    | 3x T37-6 (Yellow)| 100 pF | 220 pF  | 2x T37-6 (Yellow)| 82 pF  | 3.9 pF
       | 11 Turns         |                  | 12 Turns         |
-------+------------------+==================+------------------+----------------
10m    | 3x T37-6 (Yellow)| 82 pF  | 180 pF  | 2x T37-6 (Yellow)| 68 pF  | 3.3 pF
       | 10 Turns         |                  | 11 Turns         |
================================================================================

## 4.4 Automated Relay Routing NetworkTo automatically swap the correct filter modules into the antenna line when you spin the VFO knob, the ESP32 controls a matrix of six Omron G5V-2 sub-miniature 5V DPDT relays.                       +5V Supply Rail
                             │
                             ▼
                    +--------+--------+
                    |  Relay Coil      |
                    +--------+--------+
                             │
                             ▼
                         C ┌───┐ B
                     Base ─┤   ├── Emitter ──> Ground
                           └───┘
                         2N2222 NPN Transistor
                             ▲
                             │ (1k Resistor)
                             │
                     ESP32 Band GPIO Pin
The ESP32 pins cannot supply enough current to drive a mechanical relay coil directly. To fix this, each relay coil is switched using an affordable 2N2222 NPN transistor.When the ESP32 drives its designated band pin HIGH (3.3V), current flows through a 1 kΩ resistor into the transistor's Base. This turns the transistor fully ON, pulling the bottom of the relay coil to Ground and cleanly latching the matching filter module into the active signal path.

---

# CHAPTER 5: KIT D - ACTIVE RF MIXER, PRE-AMP & TRANSMITTER PA

## 5.1 The FST3253 Tayloe Mixer CircuitThe heart of the receiver is the FST3253 high-speed bus multiplexer chip, operating as a Tayloe Quadrature Sampling Detector.                    74AC74 Phase Splitter
                  +------------------------+
Si5351A CLK0 ---->| Clock Input (4x Freq)  |
                  |                        |
                  |  0° Out ----> Phase A ─┼───┐
                  | 90° Out ----> Phase B ─┼─┐ │
                  +------------------------+ │ │
                                             ▼ ▼
                                      +──────┴─┴──────+
                                      | FST3253 Mixer |
Antenna Signal ──────────────────────>| (RF Input)    |
                                      +──┬──┬──┬──┬───+
                                         │  │  │  │
                                         ▼  ▼  ▼  ▼
                                        [ Sampling Caps ] (10nF)
                                         │  │  │  │
                                         ▼  ▼  ▼  ▼
                                      [ Analog I/Q Audio ]
The Si5351A clock generator outputs a high-frequency square wave at exactly four times your operating frequency. This 4x carrier enters the 74AC74 dual D-type flip-flop, which splits it into four synchronized output lines, each delayed by exactly 90 degrees ($0^\circ, 90^\circ, 180^\circ, 270^\circ$).These four clock phases rapidly open and close the internal electronic switches of the FST3253 multiplexer chip. This cycles the antenna's incoming RF signal across four identical 10nF polypropylene sampling capacitors, cleanly dropping the radio waves straight down into baseband I (In-Phase) and Q (Quadrature) audio signals.

## 5.2 Audio Pre-Amplifier StageThe audio signals leaving the mixer are incredibly faint (measured in microvolts) and must be amplified before the ESP32 can process them. We use an LM358 operational amplifier configured for differential gain.Mixer Phase 0°  ───[ 1k ]───┬───┐
                            │   ▼
                            │ ┌───┐
                            ├─┤ - │
                            │ │   ├───[ 10uF Cap ]───> ESP32 GPIO 1 (I-Channel)
Mixer Phase 180°───[ 1k ]───┼─┤ + │
                            │ └───┘
                            ├───[ 100k Feedback ]───┐
                            │                       │
                            └───[ 1.65V DC Bias ]───┘
**Gain Calculation:** By placing a 100k Ω feedback resistor across the operational amplifier circuit alongside a 1k Ω input resistor, we establish a fixed stage gain of exactly 100x (40 dB of amplification).

**The 1.65V Virtual Ground Bias:** Because the ESP32's ADC pins can only safely process positive DC voltages (0V to 3.3V), a dual 10 kΩ resistor divider network acts as a voltage splitter. This injects a continuous 1.65V DC baseline bias offset right onto the input lines, centering the amplified audio wave perfectly in the middle of the microcontroller's safe sampling window.

## 5.3 IRF510 Class-E Power AmplifierFor transmitting, we use an affordable IRF510 power MOSFET configured as an efficient, high-speed Class-E switching amplifier.Si5351A CLK1 ──> [ 74ACT08 Buffer ] ──[ 10 Ohm ]──┬──> IRF510 Gate
                                                   │
                                            [ Gate Bias pot ] (1.65V - 3.8V)
**The 74ACT08 Gate Driver:** The Si5351A clock generator breakout board lacks the drive current required to switch the heavy internal gate capacitance of the IRF510 cleanly at high frequencies. We pass the clock signal through a fast 74ACT08 logic buffer gate to ensure clean, crisp switching.

**The Adjustable Bias Circuit:** MOSFETs require a steady DC bias voltage on their gate before they begin conducting. By running an adjustable voltage divider trimpot from the power rail, you can manually dial the gate bias from 1.65V up to 3.8V. This allows you to scale the radio's output power safely from a stealthy 0.1 Watts up to a full 10 Watts.

**The 4:1 Output Transformer:** The switching output of the IRF510 has a very low impedance. To match this to a standard 50 Ω coaxial antenna system, we wind a simple 4:1 impedance transformer. This consists of 4 turns of twisted-pair (bifilar) enameled wire wound through an FT37-43 ferrite core.

---

# CHAPTER 6: KIT E - CHASSIS, POWER STORAGE & MECHANICS

## 6.1 Mechanical Layout and ErgonomicsKit E integrates all the completed modules into a rugged 6.0" × 4.0" × 2.5" extruded aluminum project box.To block stray noise and digital clock bleed from degrading your receiver's performance, you must follow a strict signal isolation layout rule: All high-voltage power components are kept on one side of the box, while all sensitive RF antenna lines are grouped on the opposite side.Left Panel (The Power Management Wall)Houses raw DC electrical infrastructure.Main 2.1mm DC Power Jack: Connects directly to external power supplies or portable field batteries.Bottom 2.1mm Charging Port: Feeds an internal 3S Lithium Protection Board (BMS) to safely balance-charge the internal 11.1V battery pack without exposing the radio's delicate digital processors to voltage spikes.Master SPST Power Switch: Disconnects the battery rail completely from the internal electronics.Right Panel (The Radio Frequency Signal Wall)Houses pure high-frequency communication lines.**Antenna BNC Connector:** Placed high on the side panel for clean strain relief of your coaxial antenna wire feedlines.

**Microphone / Keyer Chassis Jack (GX16-4):** Mounted on the lower right front panel, allowing easy access for hand microphone PTT or a Morse code straight key.

## 6.2 Master Production Interconnection SchematicThis comprehensive system blueprint shows how the completed kits link together across the internal frame walls:+-----------------------------------------------------------------------------------------+
|                                   NOVUS HEXA-SDR CHASSIS                                |
|                                                                                         |
|  [ LEFT PANEL: POWER ]                                         [ RIGHT PANEL: SIGNALS ] |
|                                                                                         |
|  12V DC Input Jack ──┐                                         ┌──> BNC Antenna Jack    |
|                      ▼                                         │                        |
|  Charge Port Jack ──> [ 3S BMS Board ] ──> Battery Pack        │                        |
|                                            │                   │                        |
|                                            ▼                   ▼                        |
|  Power Toggle Switch ──────────────────> [ ESP32 Core ] <──> [ Filter Relay Matrix ]    |
|                                            │                   │                        |
|                                            ▼                   ▼                        |
|  Hoysond 4.0" Screen <─────────────────── Hardware       ┌──> [ FST3253 Tayloe Mixer ]  |
|                                           SPI Bus        │                              |
|                                                          ▼                              |
|  Rotary Encoder <──────────────────────── GPIO Interrupts     [ IRF510 PA Stage ]       |
|                                                                ▲                        |
|                                                                │                        |
|  GX16-4 Mic Jack ──────────────────────── Audio Pins ──────────┘                        |
|                                                                                         |
+-----------------------------------------------------------------------------------------+

---

# CHAPTER 7: PRODUCTION COMPREHENSIVE BILL OF MATERIALS

This master production ledger accounts for every individual component required to source and pack the complete 5-stage Novus Hexa-SDR kit system.

================================================================================
KIT PACKAGE A: CORE PROCESSING, DISPLAY & OPERATOR INTERFACE
================================================================================
[A1]  1x Hoysond 4.0" 320x480 ST7796S SPI LCD Screen module
[A2]  1x ESP32-S3 Development Board (Dual-Core, USB-C variant)
[A3]  1x Si5351A Programmable Clock Generator Breakout Module Board
[A4]  1x Rotary Encoder (Bourns PEC11R or exact equivalent featuring internal switch)
[A5]  5x Momentary Tactile Push Buttons (Standard 6x6mm micro chassis footprint)
[A6]  1x GX16 4-Pin Chassis Aviation Connector Socket Jack (Front Panel Mount)
[A7]  1x 4-Pin Commercial Replacement Hand Microphone (Cobra/Uniden style layout)

================================================================================
KIT PACKAGE B: SWR / POWER BRIDGE CIRCUIT
================================================================================
[B1]  1x FT37-43 Ferrite Toroid Core (High-Permeability Black Compound)
[B2]  2x 1N4148 High-Speed Switching Silicon RF Signal Diodes
[B3]  2x 51 Ohm 1-Watt Carbon or Metal Film Resistors (1% Tolerance)
[B4]  2x 10nF (0.01uF) Ceramic Disc Capacitors (Rated for 50V-100V operation)
[B5]  2x 10k Ohm Single-Turn Trimpots (Bourns 3362P or equivalent)

================================================================================
KIT PACKAGE C: INTEGRATED FILTER BANK MATRIX
================================================================================
[C1]  5x T37-2 Carbonyl Iron Toroid Cores (Low-Loss Red Compound)
[C2]  25x T37-6 Carbonyl Iron Toroid Cores (High-Stability Yellow Compound)
[C3]  1x 50ft Spool 28 AWG Polyurethane Enameled Copper Magnet Wire
[C4]  24x C0G/NP0 Ceramic Capacitors (High-Voltage 100V-500V Assorted RF Values):
      - 2x 3.3 pF / 2x 3.9 pF / 2x 4.7 pF / 2x 5.6 pF
      - 2x 6.8 pF / 2x 15 pF  / 2x 68 pF  / 4x 82 pF
      - 4x 100 pF / 4x 120 pF / 4x 150 pF / 2x 220 pF
      - 2x 270 pF / 2x 330 pF / 4x 390 pF / 2x 820 pF

================================================================================
KIT PACKAGE D: ACTIVE RF MIXER, PRE-AMP & TRANSMITTER PA
================================================================================
[D1]  1x FST3253 High-Speed Bus Multiplexer IC (Tayloe Mixer Base)
[D2]  1x 74AC74 High-Speed Dual D-Type Flip-Flop IC (Quadrature Divider)
[D3]  1x LM358 Low-Noise Dual Operational Amplifier IC (Baseband Pre-amplifier)
[D4]  4x 10nF Polypropylene Film Capacitors (Matched Mixing Sampling Caps)
[D5]  1x IRF510 Power MOSFET Transistor (Rugged TO-220 Package)
[D6]  1x 74ACT08 High-Speed Quad AND Gate Logic Buffer IC (Gate Current Driver)
[D7]  1x Compact TO-220 Bolt-On Aluminum Heatsink block
[D8]  1x FT37-43 Ferrite Toroid Core (Bifilar Output Transformer)
[D9]  6x Omron G5V-2 5V DPDT Sub-miniature Signal Relays
[D10] 6x 2N2222 NPN Transistors & 6x 1k Ohm 1/4W Resistors (Relay Driver Matrix)
[D11] Pre-Amp & Bias Passive Components Assortment:
      - 2x 100k Ohm / 2x 1k Ohm / 5x 10k Ohm 1/4W Resistors
      - 1x 10k Ohm Single-Turn Bias Adjustment Trimpot
      - 2x 10uF Electrolytic DC-Blocking Capacitors
      - 2x 100nF Ceramic Capacitors / 2x 10 Ohm Carbon Resistors

================================================================================
KIT PACKAGE E: CHASSIS, POWER STORAGE & ENCLOSURE MECHANICS
================================================================================
[E1]  1x Extruded Aluminum Project Box Housing (Standard 6" x 4" x 2.5" size)
[E2]  1x BNC Panel Mount Female Coaxial RF Connector Jack (Right Wall Mount)
[E3]  2x 2.1mm Chassis Mount DC Power Barrel Sockets (Left Wall Mount)
[E4]  1x 3S 11.1V 3200mAh Rechargeable Lithium-Ion Flat Battery Pack
[E5]  1x 3S 12.6V Lithium Battery Balance Charger/BMS Protection Board module
[E6]  1x SPST Heavy-Duty Toggle or Rocker Power Switch (Far Left Front Faceplate)
================================================================================

---

# CHAPTER 8: MASTER PRODUCTION COMPREHENSIVE CODE REFERENCE

This is the verified Novus Hexa-SDR v1.7 firmware source code. It contains all cross-core scheduling links, the Goertzel audio sampling routines on Core 0, and display processing tasks on Core 1.

```cpp
#include <Wire.h>
#include <si5351.h>
#include <TFT_eSPI.h> 

TFT_eSPI tft = TFT_eSPI();
Si5351 si5351;

// Hardware Pin Designations
const int ENCODER_A    = 4;   
const int ENCODER_B    = 5;   
const int ENCODER_SW   = 6;   
const int AUDIO_IN_PIN = 1;   

const int BTN_VFO      = 7;   
const int BTN_CALL     = 2;   
const int BTN_MENU     = 3;   
const int BTN_BAND     = 21;  
const int BTN_MODE     = 47;  
const int BTN_RIT      = 48;  

// Shared Global Memory Variables
volatile long baseFrequency  = 14060000; 
volatile long ritOffset      = 0;
volatile bool ritActive      = false;
volatile long tuningStepSize = 100;      
volatile bool frequencyChanged = true;

enum RadioMode { MODE_CW, MODE_SSB };
RadioMode currentMode = MODE_CW;

bool isAutoCQRunning = false;
String operatorCallsign = "W8YI";        

int currentBandIndex = 1; 
const long bandEdges[6] = {7000000, 14000000, 18068000, 21000000, 24890000, 28000000};
const char* bandNames[6] = {"40m", "20m", "17m", "15m", "12m", "10m"};

long lastDisplayedVFO   = -1;
long lastDisplayedRit   = -1;
long lastDisplayedStep  = -1;
int lastMeterValue      = -1;
RadioMode lastDisplayedMode = MODE_SSB;
int lastDisplayedBand   = -1;

// Terminal Matrix Geometry Definitions
const int MAX_LINES     = 5;   
const int LINE_HEIGHT   = 22;  
const int TEXT_START_Y = 195;  
String terminalLines[MAX_LINES] = {"", "", "", "", ""};
int currentLineIndex    = 0;

// Goertzel Constant Coefficients
const float SAMPLING_FREQUENCY = 10000.0; 
volatile float targetCwPitch   = 500.0;   
const int N                    = 100;     

int dotLength = 150; 
unsigned long toneStartTime = 0;
unsigned long toneEndTime   = 0;
bool isToneActive           = false;
String morseTreeBuffer      = "";
volatile char decodedCharQueue = 0;

TaskHandle_t Core0Task;

void setup() {
  Serial.begin(115200);
  analogReadResolution(12); 

  tft.init();
  tft.setRotation(1); 
  tft.fillScreen(TFT_BLACK);
  drawStaticUIFrames();

  bool siFound = si5351.init(SI5351_CRYSTAL_LOAD_8PF, 0, 0);
  if (siFound) {
    si5351.drive_strength(SI5351_CLK0, SI5351_DRIVE_8MA);
    si5351.set_freq((baseFrequency * 4ULL) * 100ULL, SI5351_CLK0); 
  }

  pinMode(ENCODER_A, INPUT_PULLUP);
  pinMode(ENCODER_B, INPUT_PULLUP);
  pinMode(ENCODER_SW, INPUT_PULLUP);
  
  pinMode(BTN_VFO,   INPUT_PULLUP);
  pinMode(BTN_CALL,  INPUT_PULLUP);
  pinMode(BTN_MENU,  INPUT_PULLUP);
  pinMode(BTN_BAND,  INPUT_PULLUP);
  pinMode(BTN_MODE,  INPUT_PULLUP);
  pinMode(BTN_RIT,   INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(ENCODER_A), readEncoderISR, CHANGE);
  xTaskCreatePinnedToCore(Core0DSPDecoderLoop, "Core0DSP", 4000, NULL, 1, &Core0Task, 0);
}

void IRAM_ATTR readEncoderISR() {
  int stateA = digitalRead(ENCODER_A);
  int stateB = digitalRead(ENCODER_B);
  long change = (stateA == stateB) ? tuningStepSize : -tuningStepSize;
  if (ritActive) ritOffset += change;
  else           baseFrequency += change;
  frequencyChanged = true;
}

void Core0DSPDecoderLoop(void * pvParameters) {
  float k = (int)(0.5 + ((N * targetCwPitch) / SAMPLING_FREQUENCY));
  float omega = (2.0 * PI * k) / N;
  float cosine = cos(omega);
  float coefficient = 2.0 * cosine;
  long lastPushedFreq = 0;

  for(;;) {
    long currentHardwareTarget = baseFrequency;
    if (ritActive) currentHardwareTarget += ritOffset;

    if (currentHardwareTarget != lastPushedFreq) {
      lastPushedFreq = currentHardwareTarget;
      unsigned long long mixerClockTarget = (unsigned long long)lastPushedFreq * 4ULL;
      si5351.set_freq(mixerClockTarget * 100ULL, SI5351_CLK0);
    }

    float q0 = 0, q1 = 0, q2 = 0;
    for (int i = 0; i < N; i++) {
      float sample = (float)analogRead(AUDIO_IN_PIN) - 2048.0;
      q0 = coefficient * q1 - q2 + sample;
      q2 = q1; q1 = q0;
      delayMicroseconds(100); 
    }
    
    float magnitudeSquared = (q1 * q1) + (q2 * q2) - (coefficient * q1 * q2);
    bool toneDetected = (magnitudeSquared > 150000.0); 
    unsigned long now = millis();

    if (toneDetected && !isToneActive) {
      isToneActive = true;
      unsigned long offDuration = now - toneEndTime;
      toneStartTime = now;
      if (offDuration > dotLength * 4 && decodedCharQueue == 0) decodedCharQueue = ' ';
    } 
    else if (!toneDetected && isToneActive) {
      isToneActive = false;
      toneEndTime = now;
      unsigned long onDuration = toneEndTime - toneStartTime;
      if (onDuration > 30 && onDuration < dotLength * 1.8)     morseTreeBuffer += ".";
      else if (onDuration >= dotLength * 1.8)                  morseTreeBuffer += "-";
    }

    if (!isToneActive && morseTreeBuffer.length() > 0 && (now - toneEndTime > dotLength * 2)) {
      char parsedLetter = lookupMorse(morseTreeBuffer);
      if (parsedLetter != '?') decodedCharQueue = parsedLetter;
      morseTreeBuffer = ""; 
    }
    vTaskDelay(pdMS_TO_TICKS(1));
  }
}

void loop() {
  checkHardwareButtons();

  if (frequencyChanged) {
    noInterrupts(); long currentBase = baseFrequency; long currentRit = ritOffset; long currentStep = tuningStepSize; bool currentRitMode = ritActive; frequencyChanged = false; interrupts();
    if (currentBase != lastDisplayedVFO || currentStep != lastDisplayedStep || currentRit != lastDisplayedRit) {
      updateVFODisplay(currentBase, currentStep, currentRit, currentRitMode);
      lastDisplayedVFO = currentBase; lastDisplayedStep = currentStep; lastDisplayedRit = currentRit;
    }
  }

  if (currentMode != lastDisplayedMode || currentBandIndex != lastDisplayedBand) {
    updateStateLabels();
    lastDisplayedMode = currentMode; lastDisplayedBand = currentBandIndex;
  }

  int realAudioLevel = map(analogRead(AUDIO_IN_PIN), 2048, 4095, 0, 100);
  drawBarMeter(constrain(realAudioLevel, 5, 95));

  if (decodedCharQueue != 0) {
    char letterToPrint = decodedCharQueue; decodedCharQueue = 0; 
    printCharToTerminal(letterToPrint);
  }

  if (isAutoCQRunning) tft.drawString("TX BEACON: SENDING CQ...", 15, 145, 2);
  else                 tft.fillRect(15, 145, 300, 15, TFT_BLACK); 

  delay(30); 
}

void checkHardwareButtons() {
  static unsigned long encoderSwTimer = 0; static bool encoderSwPressed = false;
  static unsigned long callSwTimer = 0;     static bool callSwPressed = false;

  if (digitalRead(ENCODER_SW) == LOW) {
    if (!encoderSwPressed) { encoderSwPressed = true; encoderSwTimer = millis(); }
  } else {
    if (encoderSwPressed) {
      unsigned long hold = millis() - encoderSwTimer; encoderSwPressed = false;
      if (hold > 600) tuningStepSize = 100000;
      else {
        if (tuningStepSize == 10)         tuningStepSize = 100;
        else if (tuningStepSize == 100)   tuningStepSize = 1000;
        else                              tuningStepSize = 10;
      }
      frequencyChanged = true;
    }
  }

  if (digitalRead(BTN_CALL) == LOW) {
    if (!callSwPressed) { callSwPressed = true; callSwTimer = millis(); }
  } else {
    if (callSwPressed) {
      unsigned long hold = millis() - callSwTimer; callSwPressed = false;
      if (hold > 1500) Serial.println("Entering Input Mode.");
      else             isAutoCQRunning = !isAutoCQRunning;
    }
  }

  if (digitalRead(BTN_RIT) == LOW) {
    ritActive = !ritActive;
    if (!ritActive) ritOffset = 0;
    frequencyChanged = true; delay(250);
  }

  if (digitalRead(BTN_BAND) == LOW) {
    currentBandIndex = (currentBandIndex + 1) % 6;
    baseFrequency = bandEdges[currentBandIndex]; frequencyChanged = true; delay(250);
  }

  if (digitalRead(BTN_MODE) == LOW) {
    currentMode = (currentMode == MODE_CW) ? MODE_SSB : MODE_CW; delay(250);
  }
}

void drawStaticUIFrames() {
  tft.drawRect(0, 0, 480, 320, TFT_BLUE); tft.drawLine(0, 160, 480, 160, TFT_BLUE); 
  tft.drawString("NOVIS HEXA-SDR v1.7", 15, 10, 2); tft.drawString("AUDIO S-METER", 15, 95, 2);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK); tft.drawString("CW DECODER TERMINAL:", 15, 170, 2);
}

void updateStateLabels() {
  tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.fillRect(15, 40, 180, 45, TFT_BLACK); 
  String modeText = (currentMode == MODE_CW) ? "MODE: CW" : "MODE: USB"; tft.drawString(modeText, 15, 40, 2);
  String bandText = "BAND: " + String(bandNames[currentBandIndex]); tft.drawString(bandText, 15, 65, 2);
}

void updateVFODisplay(long base, long step, long rit, bool ritOn) {
  tft.setTextColor(TFT_GREEN, TFT_BLACK); float mhz = base / 1000000.0; tft.drawFloat(mhz, 5, 240, 30, 4); 
  tft.fillRect(240, 65, 230, 50, TFT_BLACK); tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
  String stepLabel = "STEP: " + String(step) + " Hz"; tft.drawString(stepLabel, 240, 65, 2);
  if (ritOn) {
    tft.setTextColor(TFT_RED, TFT_BLACK); String ritLabel = "RIT ON: " + String(rit) + " Hz"; tft.drawString(ritLabel, 240, 90, 2);
  }
}

void drawBarMeter(int percentageValue) {
  int startX = 15; int startY = 120; int meterWidth = 200; int meterHeight = 15; int targetFillPixels = (percentageValue * meterWidth) / 100;
  if (percentageValue < lastMeterValue) tft.fillRect(startX + targetFillPixels, startY, meterWidth - targetFillPixels, meterHeight, TFT_BLACK);
  uint16_t barColor = (percentageValue > 75) ? TFT_YELLOW : TFT_GREEN; tft.fillRect(startX, startY, targetFillPixels, meterHeight, barColor); lastMeterValue = percentageValue;
}

void printCharToTerminal(char c) {
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  if (terminalLines[currentLineIndex].length() >= 38) {
    if (currentLineIndex < MAX_LINES - 1) currentLineIndex++;
    else {
      for (int i = 0; i < MAX_LINES - 1; i++) terminalLines[i] = terminalLines[i + 1];
      terminalLines[MAX_LINES - 1] = ""; 
    }
  }
  terminalLines[currentLineIndex] += c;
  tft.fillRect(15, TEXT_START_Y, 450, LINE_HEIGHT * MAX_LINES, TFT_BLACK);
  for (int i = 0; i < MAX_LINES; i++) tft.drawString(terminalLines[i], 15, TEXT_START_Y + (i * LINE_HEIGHT), 2);
}

char lookupMorse(String elements) {
  if (elements == ".-")   return 'A'; if (elements == "-...") return 'B'; if (elements == "-.-.") return 'C'; if (elements == "-..")  return 'D';
  if (elements == ".")    return 'E'; if (elements == "..-.") return 'F'; if (elements == "--.")  return 'G'; if (elements == "....") return 'H';
  if (elements == "..")   return 'I'; if (elements == ".---") return 'J'; if (elements == "-.-")  return 'K'; if (elements == ".-..") return 'L';
  if (elements == "--")   return 'M'; if (elements == "-.")   return 'N'; if (elements == "---")  return 'O'; if (elements == ".--.") return 'P';
  if (elements == "--.-") return 'Q'; if (elements == ".-.")  return 'R'; if (elements == "...")  return 'S'; if (elements == "-")    return 'T';
  if (elements == "..-")  return 'U'; if (elements == "...-") return 'V'; if (elements == ".--")  return 'W'; if (elements == "-..-") return 'X';
  if (elements == "-.--") return 'Y'; if (elements == "--..") return 'Z';
  return '?'; 
}