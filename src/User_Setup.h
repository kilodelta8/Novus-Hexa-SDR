// ============================================================================
// NOVIS HEXA-SDR: TFT_eSPI USER CONFIGURATION MATRIX (User_Setup.h)
// ============================================================================

// --- STEP 1: DEFINE DISPLAY DRIVER ARCHITECTURE ---
#define ST7796_DRIVER       // Tells library to compile for the ST7796S silicon controller

// --- STEP 2: DEFINE PHYSICAL PIN INTERFACE FOR ESP32-S3 ---
// We map these to dedicated high-speed native hardware SPI lines
#define TFT_MISO   -1       // Pin unallocated (Saves a GPIO since we only write to glass)
#define TFT_MOSI   13       // Connects to Display Pin: SDI / MOSI
#define TFT_SCLK   14       // Connects to Display Pin: SCK / CLK
#define TFT_CS     10       // Connects to Display Pin: CS (Chip Select)
#define TFT_DC     12       // Connects to Display Pin: DC / RS (Data/Command Selection)
#define TFT_RST    11       // Connects to Display Pin: RESET

// --- STEP 3: INTEGRATE CAPACITIVE TOUCH SPI LINES ---
// This pins the touch controller chip to the master graphics engine bus loop
#define TOUCH_CS   18       // Dedicated Touch Screen Controller Chip Select pin

// --- STEP 4: SELECTION OF COMPREHENSIVE FONT MATRIX ---
// Loading only the specific geometric fonts we need saves precious program space
#define LOAD_GLCD   // Font 1: Standard clean 8-pixel font for titles/labels
#define LOAD_FONT2  // Font 2: Clean 16-pixel high alphanumeric font for terminal lines
#define LOAD_FONT4  // Font 4: Medium bold font for step readouts and parameters

// --- STEP 5: BUS SPEED OPTIMIZATION CONSTANTS ---
// Pushing the clocks to maximum keeps the S-meters snappy and frequency dialing lag-free
#define SPI_FREQUENCY       40000000  // Run primary graphic clock lines at full 40 MHz 
#define SPI_READ_FREQUENCY  20000000  // Set read clock limits securely at 20 MHz
#define SPI_TOUCH_FREQUENCY  2500000  // Set touch clock speed safely at 2.5 MHz

// --- STEP 6: BACKEND COLOR PATTERN ALIGNMENT ---
#define TFT_RGB_ORDER TFT_BGR         // Adjusts proper color registration for this display module