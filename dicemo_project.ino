#include <Arduino.h>
#include <lvgl.h>
#include "Arduino_GFX_Library.h"
#include "pin_config.h"
#include "esp_sleep.h"
#include <Wire.h>
#include "SensorQMI8658.hpp"
#include "HWCDC.h"
#include <math.h>
#include "images.h"
#include "dynapull.c"
#include "robot.c"
#include <algorithm>
#include <Preferences.h>
#include "dicemo_quotes.h"


HWCDC USBSerial;
#define EXAMPLE_LVGL_TICK_PERIOD_MS 2
#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 280

// Pin Configuration
const int inputPin = 40;   // Button input
const int outputPin = 41;  // Power control output
const int beePin = 42;     // Buzzer

// Sensor Data
SensorQMI8658 qmi;
IMUdata acc, gyr;

// Pending DiceMo quote type
DicemoQuoteType pending_quote_type = QUOTE_NONE;
unsigned long quote_timer_start = 0;
unsigned long next_quote_interval = 0;
bool quote_timer_active = false;

// Calibration variables
static float baseRoll = 0, basePitch = 0;
static bool isCalibrated = false;
static unsigned long arrowLastShownTime = 0;
static const unsigned long arrowVisibilityDuration = 500; // Keep arrows visible for 500ms

// Sleep vars
bool isSleeping = false;
bool isDeepSleeping = false; // Track deep sleep explicitly
unsigned long lastActivityTime = 0;
const unsigned long backlightTimeout = 45000; // 45 seconds
const unsigned long deepSleepTimeout = 600000; // 10 minutes
bool justWokeUp = false;
unsigned long wakeCooldownStart = 0;
const unsigned long wakeCooldownDuration = 1500;  // 1.5 seconds to ignore false motion

// Debounce timing
static unsigned long lastMoveTime = 0;
static const unsigned long moveDebounceTime = 500; // 500ms debounce to prevent excessive menu repeats
static unsigned long lastButtonPressTime = 0;
static const unsigned long buttonDebounceTime = 300; // Prevent multiple dice being added at once

// Button Variables
bool buttonState = false;
bool lastButtonState = false;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;
unsigned long lastClickTime = 0;
unsigned long clickInterval = 500;       // Double-click interval
unsigned long longPressDuration = 2000;  // 1 sec long press
bool longPressDetected = false;
bool doubleClickDetected = false;
bool clickDetected = false;
bool pendingSingleClick = false;

// Battery indictator 
static unsigned long lastBatteryUpdate = 0;
const int voltageDividerPin = 1;  // GPIO1 pin
float vRef = 3.3;                 // Power supply voltage of ESP32-S3 (unit: volts)
float R1 = 200000.0;              // Resistance value of the first resistor (unit: ohms)
float R2 = 100000.0;              // Resistance value of the second resistor (unit: ohms)
bool is_battery_critical = false;
static lv_timer_t *battery_flash_timer = NULL;
static bool battery_visible = true;  // Used for toggling visibility

// UI Objects
static lv_obj_t *modal_bg;
static lv_obj_t *selection_box;
static lv_obj_t *dice_img;
static lv_obj_t *dice_label;
static lv_obj_t *selected_dice_label;
static lv_obj_t *dice_count_label;
static lv_obj_t *dice_list;
static lv_obj_t *modal = NULL;
static lv_obj_t *bg_img_obj;
static lv_obj_t *dice_result_label;
static int bg_frame = 0;
static lv_obj_t *modifier_label;
static lv_obj_t *prev_roll_label;
static lv_obj_t *roll_breakdown_screen = NULL;
static lv_obj_t *breakdown_container = NULL;
static lv_obj_t *main_screen = NULL; // Define main screen globally
static lv_obj_t* dice_grid = NULL;
static lv_obj_t* dicemo_img_obj = NULL;
static lv_obj_t *fav_indicator_label = NULL;
static lv_obj_t *battery_indicator_label = NULL;
static lv_obj_t *volume_indicator_label = NULL;
static lv_obj_t *dicemo_widget = NULL;
static lv_obj_t *dicemo_bubble = NULL;
static lv_obj_t *dicemo_text_label = NULL;
static lv_coord_t single_col[] = {LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
static lv_coord_t single_row[] = {LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};

// Global variable to track  timer
static lv_timer_t *transition_timer = NULL;
static lv_timer_t *breakdown_delay_timer = NULL;

// Dice  Variables
static const lv_img_dsc_t *dice_images[] = {
    &nodie, &d4, &d6, &d8, &d10, &d12, &d20, &dpercent,
    &clearfav, &clearmemory, &volume  // <-- Add mute toggle image
};

static const char *dice_names[] = {
    "Reset Dice",    // index 0 (was "nodie")
    "1d4", "1d6", "1d8", "1d10", "1d12", "1d20", "1d%",
    "Clear Favorites", "Clear NVM", "Mute Toggle"
};

static int selected_dice_index = 0;
static int dice_counts[8] = {0};
static int modifier = 0;
#define MAX_ROLLED_DICE 40  // Prevent memory overflow
int rolled_values[MAX_ROLLED_DICE] = {0};  // Store individual dice results
int num_rolled_dice = 0;  // Track how many dice were rolled
int rolled_types[MAX_ROLLED_DICE] = {0};  // Stores die size (4, 6, 8, etc.) for each rolled value
static lv_obj_t* dice_container = NULL;
static int current_page = 0;
static int total_pages = 0;
static lv_timer_t* carousel_timer = NULL;
bool isRolling = false;
#define NUM_DICE_OPTIONS 11

// Variable to store the last roll result
static char last_roll_result[16] = "--"; // Default to "--" until we have a roll
static char last_five_rolls[5][16] = {"--", "--", "--", "--", "--"};
static int last_roll_index = 0;

static int roll_step = 0;
static lv_timer_t *roll_timer = NULL;

// Debounce tilt
static unsigned long lastTiltTime = 0;

// Buzzer State Machine
const char *boot_sound = "Boot:d=8,o=5,b=160:g,c,e,g,2p,f,a,2p";
const char *wake_sound = "Wake:d=16,o=6,b=140:c,e,g,c7,g6,e,g,a,c7,p,g6,a#,c7,d7,e7";
const char *roll_sound = "Roll:d=16,o=5,b=200:c,p,e,p,g";
const char* neutral_sound = "Neutral:d=8,o=5,b=100:g5,f#,f,d#,d,c#";
const char *crit_sound = "Crit:d=8,o=6,b=160:g,c6,e6,g6,2p,c7,g6,c7,p,e7";
const char *fail_sound = "Fail:d=4,o=5,b=80:g,b4,g,d#,p,f,g#,c5,b4,p,d4,d4,c4";
const char *sleep_sound = "Sleepy:d=8,o=4,b=90:g,f#,g,4p,c#,c,2b3";
const char *misc_sound = "DicemoMisc:d=16,o=5,b=140:e6,g6,b,4p,e6,g6,b,4p,e6,g6,b,e6,g6,b,p,d6,e6,g6,b,a6,g6,e6,d6,e6,g6,b,a6,g6,e6";

static bool isPlaying = false;
static bool isMuted = false;

struct Note {
    const char *name;
    int frequency;
};

// --- RTTTL Full Parser and Playback ---
#define MAX_NOTES 128

struct ParsedNote {
    int frequency;   // Frequency in Hz (0 for rest)
    int duration;    // Duration in milliseconds
};

ParsedNote parsed_melody[MAX_NOTES];
int parsed_note_count = 0;
int parsed_note_index = 0;

int default_duration = 4;
int default_octave = 6;
int bpm = 63;
unsigned long note_start_time = 0;
// Note frequency table (C, C#, D, D#, ..., B) for 4th octave
const int rtttl_freq_table[] = {
    262, 277, 294, 311, 330, 349, 370, 392, 415, 440, 466, 494
};
const char* rtttl_note_names[] = {
    "c", "c#", "d", "d#", "e", "f", "f#", "g", "g#", "a", "a#", "b"
};
#define NOTE_TABLE_SIZE 12

Preferences prefs; // for non-volatile storage

// Struct to hold favorite dice sets
struct DiceFavorite {
    int dice_counts[8]; // stores dice counts (indices match dice_types)
    int modifier;       // modifier for each set
};

struct DiceHistory {
    char last_roll[16];
    char last_five[5][16];
};

DiceHistory history;
DiceFavorite favorites[5]; // Array of 5 favorites
int current_fav_index = 0; // Tracks currently active favorite

const lv_img_dsc_t* get_image_for_die_size(int sides) {
    switch (sides) {
        case 4: return &d4;
        case 6: return &d6;
        case 8: return &d8;
        case 10: return &d10;
        case 12: return &d12;
        case 20: return &d20;
        case 100: return &dpercent;
        default: return &nodie;
    }
}

// Default roll placeholders
const char *default_roll_texts[] = {
    "Ready to Roll", 
    "DiceMo Awaits", 
    "Roll for Glory!", 
    "Choose Your Fate!",
    "Adventure awaits...huzzah!",
    "Ready to Roll", 
    "Scott's mom!", 
    "Roll for Glory!", 
    "Choose Your Fate!",
    "The Fates are Listening...said call back",
    "Destiny Calls....caw",
    "Your Roll, Your Legend",
    "May the Odds Be Ever Nat 20",
    "Time to Tempt Fate",
    "Summon the Chaos and Scott's mom",
    "Brace for Impact, bitch",
    "The Dice Are Restless",
    "Heroes Never Hesitate",
    "What Could Possibly Go Wrong?",
    "Shake the Realms",
    "Begin the Mayhem",
    "This is How Legends Begin",
    "Only Luck (and Rage) Remain"
};


void animate_to_next_page(lv_timer_t* timer);  // forward declaration
using DisplayPageFunc = std::function<void(int)>; // ensure this stays globally defined at top
static DisplayPageFunc g_display_page_func = nullptr;


// RTTTL Note Frequency Table (for 5th octave, adjust for others)
Note note_table[] = {
    {"c", 523},  {"c#", 554}, {"d", 587},  {"d#", 622},
    {"e", 659},  {"f", 698},  {"f#", 740}, {"g", 784},
    {"g#", 831}, {"a", 880},  {"a#", 932}, {"b", 988},
    {"4p", 0},   // Rest Note
};
#define NOTE_COUNT (sizeof(note_table) / sizeof(Note))

// Display Buffer
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[SCREEN_WIDTH * SCREEN_HEIGHT / 10];

// Display Setup
Arduino_DataBus *bus = new Arduino_ESP32SPI(LCD_DC, LCD_CS, LCD_SCK, LCD_MOSI);
Arduino_GFX *gfx = new Arduino_ST7789(bus, LCD_RST, 0, true, LCD_WIDTH, LCD_HEIGHT, 0, 20, 0, 0);

/**
 * @brief LVGL display flush function with horizontal mirroring
 */
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);
    uint16_t temp_buf[w];

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            temp_buf[w - 1 - x] = color_p[y * w + x].full; // Reverse row for mirroring
        }
        gfx->draw16bitRGBBitmap(area->x1, area->y1 + y, temp_buf, w, 1);
    }

    lv_disp_flush_ready(disp);
}

/**
 * @brief LVGL tick handler
 */
void lv_tick_handler(void *arg) {
    lv_tick_inc(EXAMPLE_LVGL_TICK_PERIOD_MS);
}

/**
 * @brief Initializes the display and LVGL
 */
void init_display() {
    gfx->begin();
    
    pinMode(LCD_BL, OUTPUT);
    digitalWrite(LCD_BL, HIGH);

    lv_init();
    lv_disp_draw_buf_init(&draw_buf, buf, NULL, SCREEN_WIDTH * SCREEN_HEIGHT / 10);

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = SCREEN_WIDTH;
    disp_drv.ver_res = SCREEN_HEIGHT;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);
}

/**
 * @brief Initializes the LVGL periodic timer
 */
void init_lvgl_timer() {
    const esp_timer_create_args_t lvgl_tick_timer_args = {
        .callback = &lv_tick_handler,
        .name = "lvgl_tick"
    };

    esp_timer_handle_t lvgl_tick_timer = NULL;
    esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer);
    esp_timer_start_periodic(lvgl_tick_timer, EXAMPLE_LVGL_TICK_PERIOD_MS * 1000);
}

/**
 * @brief Initializes the accelerometer
 */
void init_sensors() {
    if (!qmi.begin(Wire, QMI8658_L_SLAVE_ADDRESS, IIC_SDA, IIC_SCL)) {
        while (1) delay(1000);  // Halt if sensor init fails
    }

    qmi.configAccelerometer(SensorQMI8658::ACC_RANGE_4G, SensorQMI8658::ACC_ODR_1000Hz, SensorQMI8658::LPF_MODE_0, true);
    qmi.enableAccelerometer();
}

// Function to set a random default roll message
void set_random_default_text() {
    int index = esp_random() % 22; // Pick a random message
    lv_label_set_text(prev_roll_label, default_roll_texts[index]);
}

// Sleeo logic
void enter_sleep_mode() {
    if (isSleeping) return;

    USBSerial.println("Entering idle mode (backlight off)...");
    digitalWrite(LCD_BL, LOW);
    isSleeping = true;

    quote_timer_active = true;
    quote_timer_start = millis();
    next_quote_interval = random(backlightTimeout, deepSleepTimeout - 30000); // 45–90 sec
    USBSerial.printf("Quote timer set to %lu seconds\n", next_quote_interval / 1000);
}

void enter_deep_sleep() {
    if (isDeepSleeping) return;

    USBSerial.println("Entering deep sleep (power off)...");
    isDeepSleeping = true;

    play_named_sound(sleep_sound);
    delay(1000);  // Let sound finish

    digitalWrite(outputPin, LOW);
    noTone(beePin);
}

void wake_from_sleep() {
    if (!isSleeping) return;

    USBSerial.println("Waking from sleep...");
    digitalWrite(LCD_BL, HIGH);
    isSleeping = false;
    play_named_sound(wake_sound);
    lv_scr_load(main_screen);
    set_random_default_text();

}

// Buzzer logic
void update_buzzer() {
    if (!isPlaying || parsed_note_index >= parsed_note_count) {
        noTone(beePin);
        isPlaying = false;
        return;
    }

    unsigned long now = millis();
    ParsedNote& note = parsed_melody[parsed_note_index];

    if (now - note_start_time >= note.duration) {
        parsed_note_index++;
        note_start_time = now;

        if (parsed_note_index >= parsed_note_count) {
            noTone(beePin);
            isPlaying = false;
            return;
        }

        note = parsed_melody[parsed_note_index];
        if (note.frequency > 0) {
            tone(beePin, note.frequency, note.duration);
        } else {
            noTone(beePin); // rest
        }
    }
}

void play_named_sound(const char* sound_name) {
    if (sound_name) {
        play_rtttl(sound_name);
    }
}

void play_rtttl(const char* rtttl_string) {
    if (isMuted || !rtttl_string || strlen(rtttl_string) < 5) return;
    parse_rtttl(rtttl_string);
}

int get_note_freq(const char* note, int octave) {
    for (int i = 0; i < NOTE_TABLE_SIZE; ++i) {
        if (strcmp(note, rtttl_note_names[i]) == 0) {
            int base_freq = rtttl_freq_table[i];
            int shift = octave - 4;
            return shift >= 0 ? (base_freq << shift) : (base_freq >> -shift);
        }
    }
    return 0; // Not found or rest
}

void parse_rtttl(const char* rtttl) {
    parsed_note_count = 0;
    parsed_note_index = 0;

    if (!rtttl) return;

    // Skip song name
    const char* p = strchr(rtttl, ':');
    if (!p) return;
    ++p;

    // Parse defaults
    while (*p && *p != ':') {
        if (*p == 'd') {
            p += 2; // skip "d="
            default_duration = atoi(p);
        } else if (*p == 'o') {
            p += 2;
            default_octave = atoi(p);
        } else if (*p == 'b') {
            p += 2;
            bpm = atoi(p);
        }
        while (*p && *p != ',' && *p != ':') ++p;
        if (*p == ',') ++p;
    }

    if (*p == ':') ++p;

    // Timing calculations
    int whole_note_duration = (60000 * 4) / bpm;

    // Parse note sequence
    while (*p && parsed_note_count < MAX_NOTES) {
        int duration = default_duration;
        int octave = default_octave;
        bool dotted = false;
        char note_buf[4] = {0};

        // Duration
        if (isdigit(*p)) {
            duration = atoi(p);
            while (isdigit(*p)) ++p;
        }

        // Note (may include '#')
        if (*p == 'p') {
            note_buf[0] = 'p';
            ++p;
        } else if (isalpha(*p)) {
            note_buf[0] = tolower(*p);
            ++p;
            if (*p == '#') {
                note_buf[1] = '#';
                ++p;
            }
        }

        // Dot
        if (*p == '.') {
            dotted = true;
            ++p;
        }

        // Octave override
        if (isdigit(*p)) {
            octave = *p - '0';
            ++p;
        }

        // Comma
        while (*p == ',') ++p;

        // Calculate duration
        int note_duration = whole_note_duration / duration;
        if (dotted) note_duration += note_duration / 2;

        int freq = (note_buf[0] == 'p') ? 0 : get_note_freq(note_buf, octave);
        parsed_melody[parsed_note_count++] = { freq, note_duration };
    }

    USBSerial.printf("Parsed %d RTTTL notes.\n", parsed_note_count);
    parsed_note_index = 0;
    note_start_time = millis();
    isPlaying = true;
}

void save_mute_state() {
    prefs.begin("settings", false);
    prefs.putBool("isMuted", isMuted);
    prefs.end();
}

void load_mute_state() {
    prefs.begin("settings", true);
    isMuted = prefs.getBool("isMuted", false); // Default to false
    prefs.end();
}

void save_history_to_nvs() {
    prefs.begin("diceHist", false);
    prefs.putBytes("history", &history, sizeof(history));
    prefs.end();
    USBSerial.println("History saved to NVS.");
}

// Favorites logic
void save_favorites_to_nvs() {
    prefs.begin("diceFav", false);
    prefs.putBytes("favorites", favorites, sizeof(favorites));
    prefs.end();
    USBSerial.println("Favorites saved to NVS.");
}

void load_history_from_nvs() {
    prefs.begin("diceHist", true);
    size_t len = prefs.getBytesLength("history");
    if (len == sizeof(history)) {
        prefs.getBytes("history", &history, len);
        USBSerial.println("History loaded from NVS.");
    } else {
        USBSerial.println("No dice history found.");
        memset(&history, 0, sizeof(history));
        strcpy(history.last_roll, "--");
        for (int i = 0; i < 5; i++) strcpy(history.last_five[i], "--");
    }
    prefs.end();
}

void load_favorites_from_nvs() {
    prefs.begin("diceFav", true);
    size_t len = prefs.getBytesLength("favorites");
    if (len == sizeof(favorites)) {
        prefs.getBytes("favorites", favorites, len);
        USBSerial.println("Favorites loaded from NVS.");
    } else {
        USBSerial.println("No favorites found in NVS. Setting defaults.");
        // Default: 1d20
        for (int i = 0; i < 5; i++) {
            memset(favorites[i].dice_counts, 0, sizeof(favorites[i].dice_counts));
            favorites[i].dice_counts[6] = 1; // default d20
            favorites[i].modifier = 0;
        }
        save_favorites_to_nvs();
    }
    prefs.end();
}

void apply_saved_roll_history() {
    strncpy(last_roll_result, history.last_roll, sizeof(last_roll_result));
    lv_label_set_text(dice_result_label, last_roll_result);

    for (int i = 0; i < 5; i++) {
        strncpy(last_five_rolls[i], history.last_five[i], sizeof(last_five_rolls[i]));
    }

    // Display newest roll first (left) to oldest (right)
    char roll_history[80];
    snprintf(roll_history, sizeof(roll_history), "%s | %s | %s | %s | %s",
        last_five_rolls[(last_roll_index + 4) % 5],
        last_five_rolls[(last_roll_index + 3) % 5],
        last_five_rolls[(last_roll_index + 2) % 5],
        last_five_rolls[(last_roll_index + 1) % 5],
        last_five_rolls[last_roll_index]);
    
    lv_label_set_text(prev_roll_label, roll_history);
}


void apply_favorite(int index) {
    memcpy(dice_counts, favorites[index].dice_counts, sizeof(dice_counts));
    modifier = favorites[index].modifier;
    update_selected_label();
    update_fav_indicator_label();  // 👈 add this line
    USBSerial.printf("Favorite %d applied.\n", index + 1);
}

void cycle_favorites() {
    current_fav_index = (current_fav_index + 1) % 5;
    apply_favorite(current_fav_index);
}

void toggle_battery_visibility(lv_timer_t * timer) {
    battery_visible = !battery_visible;
    lv_obj_set_style_opa(battery_indicator_label, battery_visible ? LV_OPA_COVER : LV_OPA_TRANSP, 0);
}

void update_battery_status() {
    int adcValue = analogRead(voltageDividerPin);
    float voltage = (float)adcValue * (vRef / 4095.0);
    float actualVoltage = voltage * ((R1 + R2) / R2);

    // Calibrated range for LiPo (tweak if needed)
    const float MIN_VOLTAGE = 3.5;
    const float MAX_VOLTAGE = 4.05;

    // Calculate percentage
    float percentage = ((actualVoltage - MIN_VOLTAGE) / (MAX_VOLTAGE - MIN_VOLTAGE)) * 100.0;
    percentage = constrain(percentage, 0, 100);
    int percent_int = round(percentage);

    // Determine symbol and color
    const char* battery_symbol = LV_SYMBOL_BATTERY_EMPTY;
    lv_color_t color = lv_color_white();
    is_battery_critical = false;

    if (actualVoltage > 4.08) {
        battery_symbol = LV_SYMBOL_CHARGE;
        color = color = lv_color_white();
    } else if (percentage > 90) {
        battery_symbol = LV_SYMBOL_BATTERY_FULL;
        color = lv_palette_main(LV_PALETTE_GREEN);
    } else if (percentage > 70) {
        battery_symbol = LV_SYMBOL_BATTERY_3;
        color = lv_palette_main(LV_PALETTE_GREEN);
    } else if (percentage > 50) {
        battery_symbol = LV_SYMBOL_BATTERY_2;
        color = lv_palette_main(LV_PALETTE_GREEN);
    } else if (percentage > 25) {
        battery_symbol = LV_SYMBOL_BATTERY_1;
        color = lv_palette_main(LV_PALETTE_ORANGE);
    } else {
        battery_symbol = LV_SYMBOL_BATTERY_EMPTY;
        color = lv_palette_main(LV_PALETTE_RED);
        is_battery_critical = true;
    }

    static char last_battery_text[32] = "";
    char buffer[32] = "";

    // Charging — no % shown
    if (battery_symbol == LV_SYMBOL_CHARGE) {
        snprintf(buffer, sizeof(buffer), "%s", battery_symbol);
    } else {
        snprintf(buffer, sizeof(buffer), "%s %d%%", battery_symbol, percent_int);
    }

    // Only update if different
    if (strcmp(last_battery_text, buffer) != 0) {
        strcpy(last_battery_text, buffer);
        lv_label_set_text(battery_indicator_label, buffer);
        lv_obj_set_style_text_color(battery_indicator_label, color, 0);
    }

    // Flash if critical
    if (is_battery_critical) {
        if (!battery_flash_timer) {
            battery_flash_timer = lv_timer_create(toggle_battery_visibility, 500, NULL);
        }
    } else {
        if (battery_flash_timer) {
            lv_timer_del(battery_flash_timer);
            battery_flash_timer = NULL;
            lv_obj_set_style_opa(battery_indicator_label, LV_OPA_COVER, 0); // Ensure visible
        }
    }
}


/**
 * @brief Creates the main UI screen
 */
void update_background(lv_timer_t *timer) {
    if (!bg_img_obj) {
        USBSerial.println("Error: bg_img_obj is NULL. Skipping update.");
        return;
    }
    
    if (bg_frame < 0 || bg_frame >= 16) {
        USBSerial.printf("Error: bg_frame out of bounds (%d). Resetting to 0.\n", bg_frame);
        bg_frame = 0;
    }

    if (bg_frames[bg_frame] == NULL) {
        USBSerial.printf("Error: bg_frames[%d] is NULL. Skipping update.\n", bg_frame);
        return;
    }

    lv_img_set_src(bg_img_obj, bg_frames[bg_frame]);
    bg_frame = (bg_frame + 1) % 16; // Loop through frames safely

    // USBSerial.printf("Background updated to frame %d\n", bg_frame);
}

void update_fav_indicator_label() {
    // Check if all favorites are identical
    bool identical = true;
    for (int i = 1; i < 5; i++) {
        if (memcmp(&favorites[0], &favorites[i], sizeof(DiceFavorite)) != 0) {
            identical = false;
            break;
        }
    }

    // Only show the indicator if favorites are identical
    lv_label_set_text_fmt(fav_indicator_label, "%s %d %s", LV_SYMBOL_LEFT, current_fav_index + 1, LV_SYMBOL_RIGHT);
    lv_obj_set_style_text_color(fav_indicator_label, lv_palette_main(LV_PALETTE_YELLOW), 0);

}

void update_volume_indicator_label() {
    if (!volume_indicator_label) return;  // ✅ early exit if not created yet
    lv_label_set_text(volume_indicator_label, isMuted ? LV_SYMBOL_MUTE : LV_SYMBOL_VOLUME_MAX);
    lv_obj_set_style_text_color(volume_indicator_label, lv_color_white(), 0);
}


void create_main_ui() {
    if (main_screen) {
        USBSerial.println("Main screen already exists. Skipping recreation.");
        return;
    }

    USBSerial.printf("Sanity check before dice breakdown: breakdown_screen=%p, dice_container=%p, carousel_timer=%p\n",
                 roll_breakdown_screen, dice_container, carousel_timer);

    USBSerial.println("Creating new main screen...");

    main_screen = lv_obj_create(NULL);

    // Create the animated background
    bg_img_obj = lv_img_create(main_screen);
    lv_obj_set_size(bg_img_obj, SCREEN_WIDTH, SCREEN_HEIGHT);
    lv_obj_align(bg_img_obj, LV_ALIGN_CENTER, 0, 0);
    lv_img_set_src(bg_img_obj, &bg_frames[0]); // Set initial frame

    // Timer to cycle background images every 200ms
    lv_timer_create(update_background, 200, NULL);

    // Dice result label (centered, will show roll results later)
    dice_result_label = lv_label_create(main_screen);
    lv_label_set_text(dice_result_label, "69");
    lv_obj_align(dice_result_label, LV_ALIGN_CENTER, 0, 10);
    lv_obj_set_style_text_color(dice_result_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(dice_result_label, &dynapull, 0);

    // Horizontal line under the result label
    lv_obj_t *separator1 = lv_line_create(main_screen);
    static lv_point_t line_points[] = {{0, 0}, {100, 0}};
    lv_line_set_points(separator1, line_points, 2);
    lv_obj_align(separator1, LV_ALIGN_CENTER, 0, 45);
    lv_obj_set_style_line_color(separator1, lv_color_white(), 0);
    lv_obj_set_style_line_width(separator1, 2, 0);

    // Selected dice label (below separator)
    selected_dice_label = lv_label_create(main_screen);
    lv_label_set_text(selected_dice_label, "1d20 selected");
    lv_obj_align(selected_dice_label, LV_ALIGN_CENTER, 0, 70);
    lv_obj_set_style_text_color(selected_dice_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(selected_dice_label, &robot, 0);

    // Another horizontal line below the selected dice label
    lv_obj_t *separator2 = lv_line_create(main_screen);
    lv_line_set_points(separator2, line_points, 2);
    lv_obj_align(separator2, LV_ALIGN_CENTER, 0, 95);
    lv_obj_set_style_line_color(separator2, lv_color_white(), 0);
    lv_obj_set_style_line_width(separator2, 2, 0);

    // Previous roll result label (below second separator)
    prev_roll_label = lv_label_create(main_screen);
    lv_label_set_text(prev_roll_label, "--");
    lv_obj_align(prev_roll_label, LV_ALIGN_CENTER, 0, 115);
    lv_obj_set_style_text_color(prev_roll_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(prev_roll_label, &robot, 0);

    // Favorite Indicator Label (top-left)
    fav_indicator_label = lv_label_create(main_screen);
    lv_label_set_text(fav_indicator_label, "");
    lv_obj_align(fav_indicator_label, LV_ALIGN_TOP_LEFT, 10, 30);
    lv_obj_set_style_text_color(fav_indicator_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(fav_indicator_label, &robot, 0);

    // Battery Indicator Label (top-right)
    battery_indicator_label = lv_label_create(main_screen);
    lv_label_set_text(battery_indicator_label, "");
    lv_obj_align(battery_indicator_label, LV_ALIGN_TOP_RIGHT, -10, 30);
    lv_obj_set_style_text_color(battery_indicator_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(battery_indicator_label, &robot, 0);

    // Volume Indicator Label (top-right)
    volume_indicator_label = lv_label_create(main_screen);
    lv_label_set_text(volume_indicator_label, "");
    lv_obj_align(volume_indicator_label, LV_ALIGN_TOP_MID, -30, 30);
    lv_obj_set_style_text_color(volume_indicator_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(volume_indicator_label, &robot, 0);

    // Dicemo mascot widget
    dicemo_widget = lv_obj_create(main_screen);
    lv_obj_set_size(dicemo_widget, SCREEN_WIDTH, SCREEN_HEIGHT); // Adjust as needed
    lv_obj_align(dicemo_widget, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(dicemo_widget, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(dicemo_widget, LV_OPA_COVER, 0);
    lv_obj_clear_flag(dicemo_widget, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(dicemo_widget, LV_OBJ_FLAG_HIDDEN);  // start hidden

    dicemo_bubble = lv_obj_create(dicemo_widget);
    lv_obj_set_size(dicemo_bubble, 120, 180);  // slightly wider
    lv_obj_align(dicemo_bubble, LV_ALIGN_TOP_MID, -50, 20); // adjusted position
    lv_obj_set_style_bg_color(dicemo_bubble, lv_color_white(), 0);
    lv_obj_set_style_radius(dicemo_bubble, 10, 0);
    lv_obj_set_style_bg_opa(dicemo_bubble, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(dicemo_bubble, 2, 0);
    lv_obj_set_style_border_color(dicemo_bubble, lv_palette_main(LV_PALETTE_BLUE), 0);

    dicemo_text_label = lv_label_create(dicemo_bubble);
    lv_label_set_long_mode(dicemo_text_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(dicemo_text_label, 100);  // Make sure it wraps nicely
    lv_obj_align(dicemo_text_label, LV_ALIGN_CENTER, 0, 0);
    lv_label_set_text(dicemo_text_label, "");
    lv_obj_set_style_text_font(dicemo_text_label, &robot, 0);
    lv_obj_set_style_text_color(dicemo_text_label, lv_color_black(), 0);

    lv_obj_t *dicemo_img = lv_img_create(dicemo_widget);
    lv_img_set_src(dicemo_img, &dicemo); // This is 120x180
    lv_obj_align(dicemo_img, LV_ALIGN_BOTTOM_MID, 60, -20);  // Slight offset to the left

    USBSerial.println("UI Updated: Main screen assigned, labels repositioned, separators added.");
}

void show_dicemo_saying(DicemoQuoteType type, const char* optional_sound = NULL, bool reset_timer = true) {
    if (!dicemo_widget || !dicemo_text_label || !dicemo_bubble) {
        USBSerial.println("❌ Dicemo widget or label not initialized!");
        return;
    }

    // Ensure backlight is ON
    digitalWrite(LCD_BL, HIGH);

    // Only reset activity timer if explicitly allowed
    if (reset_timer) {
        lastActivityTime = millis();
    } else {
        // If we’re already in the 5–10 min countdown, turn BL off after 10 sec.
        unsigned long idleTime = millis() - lastActivityTime;
        if (idleTime > backlightTimeout && idleTime < deepSleepTimeout) {
            lv_timer_create([](lv_timer_t *t) {
                digitalWrite(LCD_BL, LOW);
                USBSerial.println("🔅 Turning BL off after periodic Dicemo message.");
                lv_timer_del(t);
            }, 10000, NULL); // 10 seconds
        }
    }

    lv_label_set_text(dicemo_text_label, get_random_quote(type));

    // Set starting Y below the screen
    lv_obj_set_y(dicemo_widget, SCREEN_HEIGHT);
    lv_obj_clear_flag(dicemo_widget, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(dicemo_widget);

    // Bounce-in animation
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, dicemo_widget);
    lv_anim_set_values(&a, SCREEN_HEIGHT, 0); // From off-screen to 0
    lv_anim_set_time(&a, 400);
    lv_anim_set_path_cb(&a, lv_anim_path_overshoot); // Bouncy effect
    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_y);
    lv_anim_start(&a);

    if (optional_sound && !isMuted) {
        play_named_sound(optional_sound);
    }

    lv_timer_create([](lv_timer_t *t) {
        lv_anim_t hide;
        lv_anim_init(&hide);
        lv_anim_set_var(&hide, dicemo_widget);
        lv_anim_set_values(&hide, 0, SCREEN_HEIGHT);
        lv_anim_set_time(&hide, 300);
        lv_anim_set_exec_cb(&hide, (lv_anim_exec_xcb_t)lv_obj_set_y);
        lv_anim_set_path_cb(&hide, lv_anim_path_ease_in);
        lv_anim_set_ready_cb(&hide, [](lv_anim_t *a) {
            lv_obj_add_flag(dicemo_widget, LV_OBJ_FLAG_HIDDEN);
        });
        lv_anim_start(&hide);

        lv_timer_del(t);
    }, 6000, NULL);
}

/**
 * @brief Calculates average roll efficiency based on last 5 rolls.
 * Each die is evaluated individually: (roll - 1) / (sides - 1)
 * Returns a value from 0.0 (worst) to 1.0 (best)
 */
float calculate_roll_efficiency() {
    int total_rolls = 0;
    float efficiency_sum = 0.0f;

    for (int i = 0; i < num_rolled_dice; i++) {
        int roll = rolled_values[i];
        int sides = rolled_types[i];

        if (sides <= 1) continue; // Skip invalid dice

        float eff = (float)(roll - 1) / (float)(sides - 1);
        efficiency_sum += eff;
        total_rolls++;
    }

    if (total_rolls == 0) return 0.5f; // neutral default
    return efficiency_sum / total_rolls;
}



/**
 * @brief Updates the selected dice label dynamically.
 */
void update_selected_label() {
    char buffer[64] = "";
    bool hasDice = false;

    for (int i = 1; i <= 7; i++) {  // ✅ Include d% (index 7)
        if (dice_counts[i] > 0) {
            hasDice = true;
            char temp[16];

            if (i == 7) { // d% special case
                snprintf(temp, sizeof(temp), "%dd%% ", dice_counts[i]);
            } else {
                snprintf(temp, sizeof(temp), "%d%s ", dice_counts[i], dice_names[i] + 1);  // Remove extra 'd'
            }

            strcat(buffer, temp);
        }
    }

    // Append modifier if any
    if (modifier > 0) {
        char mod_text[8];
        snprintf(mod_text, sizeof(mod_text), "+%d", modifier);
        strcat(buffer, mod_text);
    }

    // Default to 1d20 if nothing is selected
    if (!hasDice) {
        strcpy(buffer, "1d20");
    }

    lv_label_set_text(selected_dice_label, buffer);
}



void toggle_modal() {
    if (modal_bg) {
        lv_obj_del(modal_bg);
        modal_bg = NULL;
        USBSerial.println("Modal closed.");

        // Save current dice counts into current favorite when modal closes
        memcpy(favorites[current_fav_index].dice_counts, dice_counts, sizeof(dice_counts));
        favorites[current_fav_index].modifier = modifier;
        save_favorites_to_nvs();

        update_selected_label();
        return;
    }

    modal_bg = lv_obj_create(lv_scr_act());
    lv_obj_set_size(modal_bg, SCREEN_WIDTH, SCREEN_HEIGHT);
    lv_obj_set_style_bg_color(modal_bg, lv_color_black(), 0);

    selection_box = lv_obj_create(modal_bg);
    lv_obj_set_size(selection_box, 200, 200);
    lv_obj_set_style_border_color(selection_box, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_obj_set_style_border_width(selection_box, 3, 0);
    lv_obj_set_style_bg_opa(selection_box, LV_OPA_TRANSP, 0);
    lv_obj_align(selection_box, LV_ALIGN_CENTER, 0, 0);

    // Restore dice_counts and modifier from favorite WITHOUT resetting selected_dice_index
    memcpy(dice_counts, favorites[current_fav_index].dice_counts, sizeof(dice_counts));
    modifier = favorites[current_fav_index].modifier;

    // Set selected_dice_index to first selected dice or default to nodie if none
    selected_dice_index = 0;
    for (int i = 1; i < 8; i++) {
        if (dice_counts[i] > 0) {
            selected_dice_index = i;
            break;
        }
    }

    update_selected_label();

    dice_img = lv_img_create(selection_box);
    lv_img_set_src(dice_img, dice_images[selected_dice_index]);
    lv_obj_align(dice_img, LV_ALIGN_CENTER, 0, 0);

    dice_label = lv_label_create(selection_box);
    lv_label_set_text(dice_label, dice_names[selected_dice_index]);
    lv_obj_set_style_text_font(dice_label, &robot, 0);
    lv_obj_set_style_text_color(dice_label, lv_color_white(), 0);
    lv_obj_align(dice_label, LV_ALIGN_BOTTOM_RIGHT, -10, -10);

    dice_count_label = lv_label_create(selection_box);
    lv_label_set_text_fmt(dice_count_label, "x%d", dice_counts[selected_dice_index]);
    lv_obj_align(dice_count_label, LV_ALIGN_BOTTOM_RIGHT, -10, 5);
    lv_obj_set_style_text_color(dice_count_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(dice_count_label, &robot, 0);
}


void show_main_screen(lv_timer_t *timer) {
    USBSerial.println("Switching back to main screen...");
    USBSerial.printf("Sanity check before dice breakdown: breakdown_screen=%p, dice_container=%p, carousel_timer=%p\n",
                 roll_breakdown_screen, dice_container, carousel_timer);

    if (transition_timer) {
        lv_timer_del(transition_timer);
        transition_timer = NULL;
    }

    if (roll_breakdown_screen) {
        lv_scr_load(main_screen);
        lv_obj_del(roll_breakdown_screen);
        roll_breakdown_screen = NULL;
        dice_container = NULL; // Explicitly NULL these out
        breakdown_container = NULL;
        dice_grid = NULL;
    }

    if (!main_screen) {
        USBSerial.println("Main screen is NULL. Recreating...");
        create_main_ui();
    }

    isRolling = false;

    lastButtonPressTime = millis();

    USBSerial.printf("Free Heap: %d bytes\n", ESP.getFreeHeap());
    USBSerial.println("Main screen loaded successfully.");
}


// Forward declaration
static void on_page_flip_ready(lv_anim_t* a);

static void show_main_screen_cb(lv_timer_t *t) {
    show_main_screen(NULL);
    lv_timer_del(t);
}


void cleanup_after_breakdown() {
    USBSerial.println("🏁 Breakdown complete. Handling post-carousel logic...");

    if (pending_quote_type != QUOTE_NONE) {
        const char* sound = nullptr;

        switch (pending_quote_type) {
            case QUOTE_HAPPY:  sound = crit_sound; break;
            case QUOTE_MAD:    sound = fail_sound; break;
            case QUOTE_NEUTRAL: sound = nullptr; break;  // Optional neutral behavior
            default: break;
        }

        USBSerial.printf("🎭 Showing Dicemo: %d\n", pending_quote_type);
        show_dicemo_saying(pending_quote_type, sound);
        pending_quote_type = QUOTE_NONE;

        lv_timer_create(show_main_screen_cb, 3000, NULL);
    } else {
        show_main_screen(NULL);
    }

    // Ensure all dice UI-related state is reset explicitly
    dice_container = NULL;
    carousel_timer = NULL;
    breakdown_delay_timer = NULL;
}

void show_page_indicator(int current, int total) {
    if (!dice_container) return;

    lv_obj_t* label = lv_label_create(dice_container);
    lv_label_set_text_fmt(label, "Page %d of %d", current + 1, total);
    lv_obj_align_to(label, dice_container, LV_ALIGN_OUT_BOTTOM_MID, 0, -2); 
    lv_obj_set_style_text_color(label, lv_color_white(), 0);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_bg_opa(label, LV_OPA_60, 0);
    lv_obj_set_style_bg_color(label, lv_color_black(), 0);
    lv_obj_set_style_pad_all(label, 6, 0);
    lv_obj_set_style_radius(label, 6, 0);
    lv_obj_set_style_border_width(label, 0, 0);

    // Auto-delete after 1.5 seconds
    lv_timer_create([](lv_timer_t* t) {
        lv_obj_t* label = (lv_obj_t*)t->user_data;
        if (label) lv_obj_del(label);
        lv_timer_del(t);
    }, 1500, label);
}


void show_roll_breakdown_screen() {
    USBSerial.printf("Showing roll breakdown carousel... Free Heap: %d bytes\n", ESP.getFreeHeap());

    if (roll_breakdown_screen) {
        lv_obj_del(roll_breakdown_screen);
        roll_breakdown_screen = NULL;
    }

    if (carousel_timer) {
        lv_timer_del(carousel_timer);
        carousel_timer = NULL;
    }

    g_display_page_func = nullptr;

    roll_breakdown_screen = lv_obj_create(NULL);
    lv_scr_load(roll_breakdown_screen);
    lv_obj_set_style_bg_color(roll_breakdown_screen, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(roll_breakdown_screen, LV_OPA_COVER, 0);

    dice_container = lv_obj_create(roll_breakdown_screen);
    lv_obj_set_size(dice_container, SCREEN_WIDTH, SCREEN_HEIGHT);
    lv_obj_align(dice_container, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_opa(dice_container, LV_OPA_TRANSP, 0);
    lv_obj_clear_flag(dice_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_border_width(dice_container, 0, 0);
    lv_obj_set_style_pad_all(dice_container, 0, 0);
    lv_obj_set_style_radius(dice_container, 0, 0);
    lv_obj_set_layout(dice_container, LV_LAYOUT_GRID);

    const int dice_per_page = 4;
    total_pages = (num_rolled_dice + dice_per_page - 1) / dice_per_page;
    current_page = 0;

    g_display_page_func = [=](int page_number) {
        lv_obj_clean(dice_container);

        // Count dice on this page
        int remaining = num_rolled_dice - page_number * dice_per_page;
        int dice_this_page = std::min(remaining, dice_per_page);

        // Setup a 2-column grid with flexible rows
        static lv_coord_t cols[] = {120, 120, LV_GRID_TEMPLATE_LAST};  // 2 columns
        static lv_coord_t rows[] = {125, 125, LV_GRID_TEMPLATE_LAST}; // adds 10px vertical margin
        lv_obj_set_grid_dsc_array(dice_container, cols, rows);

        // Add top padding only for multi-dice pages
        if (dice_this_page > 1) {
            lv_obj_set_style_pad_top(dice_container, 20, 0);
        } else {
            lv_obj_set_style_pad_top(dice_container, 0, 0);
        }

        // Track dice types separately
        int dice_counts_copy[8];
        memcpy(dice_counts_copy, dice_counts, sizeof(dice_counts_copy));

        int dice_skipped = page_number * dice_per_page;
        int dice_idx = 0;

        for (int i = 0; i < dice_skipped; i++) {
            for (int j = 1; j < 8; j++) {
                if (dice_counts_copy[j] > 0) {
                    dice_counts_copy[j]--;
                    break;
                }
            }
        }

        for (int i = dice_skipped; i < num_rolled_dice && dice_idx < dice_per_page; i++) {
            int die_index = -1;
            for (int j = 1; j < 8; j++) {
                if (dice_counts_copy[j] > 0) {
                    die_index = j;
                    dice_counts_copy[j]--;
                    break;
                }
            }

            if (die_index == -1) continue;

            const lv_img_dsc_t* die_image = get_image_for_die_size(rolled_types[i]);
            if (!die_image) continue;

            lv_obj_t* die_holder = lv_obj_create(dice_container);
            lv_obj_set_size(die_holder, 100, 100);
            lv_obj_set_style_pad_left(die_holder, 4, 0);
            lv_obj_set_style_pad_right(die_holder, 4, 0);
            lv_obj_set_style_pad_top(die_holder, 4, 0);
            lv_obj_set_style_pad_bottom(die_holder, 4, 0);
            lv_obj_set_style_bg_opa(die_holder, LV_OPA_TRANSP, 0);
            lv_obj_clear_flag(die_holder, LV_OBJ_FLAG_SCROLLABLE);

            int col = dice_this_page == 1 ? 0 : dice_idx % 2;
            int row = dice_this_page == 1 ? 0 : dice_idx / 2;

            // For 1 die: use 1x1 grid and center manually
            if (dice_this_page == 1) {
                lv_obj_set_grid_dsc_array(dice_container, single_col, single_row);
                lv_obj_set_grid_cell(die_holder, LV_GRID_ALIGN_CENTER, 0, 1, LV_GRID_ALIGN_CENTER, 0, 1);
            } else {
                lv_obj_set_grid_cell(die_holder, LV_GRID_ALIGN_CENTER, col, 1, LV_GRID_ALIGN_CENTER, row, 1);
                lv_obj_set_style_pad_left(die_holder, 2, 0);
                lv_obj_set_style_pad_right(die_holder, 2, 0);
            }

            lv_obj_t* dice_img = lv_img_create(die_holder);
            lv_img_set_src(dice_img, die_image);
            lv_obj_align(dice_img, LV_ALIGN_CENTER, 0, 0);

            // Recolor for max/min rolls
            if (rolled_values[i] == rolled_types[i]) {
                lv_obj_set_style_img_recolor(dice_img, lv_palette_main(LV_PALETTE_GREEN), LV_PART_MAIN);
                lv_obj_set_style_img_recolor_opa(dice_img, LV_OPA_70, LV_PART_MAIN);
            } else if (rolled_values[i] == 1) {
                lv_obj_set_style_img_recolor(dice_img, lv_palette_main(LV_PALETTE_RED), LV_PART_MAIN);
                lv_obj_set_style_img_recolor_opa(dice_img, LV_OPA_70, LV_PART_MAIN);
            } else {
                lv_obj_set_style_img_recolor_opa(dice_img, LV_OPA_TRANSP, LV_PART_MAIN);
            }

            // Result label container
            lv_obj_t* result_bg = lv_obj_create(die_holder);
            lv_obj_set_size(result_bg, 40, 30);
            lv_obj_align(result_bg, LV_ALIGN_BOTTOM_RIGHT, -4, -4);
            lv_obj_set_style_bg_color(result_bg, lv_color_black(), 0);
            lv_obj_set_style_bg_opa(result_bg, LV_OPA_70, 0);
            lv_obj_set_style_radius(result_bg, 5, 0);
            lv_obj_set_style_border_width(result_bg, 0, 0);

            lv_obj_t* dice_result_label = lv_label_create(result_bg);
            lv_label_set_text_fmt(dice_result_label, "%d", rolled_values[i]);
            lv_obj_set_style_text_color(dice_result_label, lv_color_white(), 0);
            lv_obj_set_style_text_font(dice_result_label, &lv_font_montserrat_20, 0);
            lv_obj_center(dice_result_label);

            dice_idx++;
        }
    };


    g_display_page_func(current_page);
    lv_obj_set_y(dice_container, SCREEN_HEIGHT);
 
    // NEW: show page indicator
    show_page_indicator(current_page, total_pages);

    lv_anim_t bounce;
    lv_anim_init(&bounce);
    lv_anim_set_var(&bounce, dice_container);
    lv_anim_set_values(&bounce, SCREEN_HEIGHT, 0);
    lv_anim_set_time(&bounce, 350);
    lv_anim_set_exec_cb(&bounce, (lv_anim_exec_xcb_t)lv_obj_set_y);
    lv_anim_set_path_cb(&bounce, lv_anim_path_overshoot);
    lv_anim_start(&bounce);

    if (total_pages > 1) {
        carousel_timer = lv_timer_create([](lv_timer_t* t) {
            if (!g_display_page_func || !dice_container) return;

            lv_obj_add_flag(dice_container, LV_OBJ_FLAG_HIDDEN);  // Hide before update
            g_display_page_func(++current_page);
            lv_obj_clear_flag(dice_container, LV_OBJ_FLAG_HIDDEN);  // Show after update
            lv_obj_set_y(dice_container, SCREEN_HEIGHT);  // Then animate in
            lv_refr_now(NULL); // force full render before animating

            // NEW: show page indicator
            show_page_indicator(current_page, total_pages);

            lv_anim_t bounce;
            lv_anim_init(&bounce);
            lv_anim_set_var(&bounce, dice_container);
            lv_anim_set_values(&bounce, SCREEN_HEIGHT, 0);
            lv_anim_set_time(&bounce, 350);
            lv_anim_set_exec_cb(&bounce, (lv_anim_exec_xcb_t)lv_obj_set_y);
            lv_anim_set_path_cb(&bounce, lv_anim_path_overshoot);
            lv_anim_start(&bounce);

            if (current_page == total_pages - 1) {
                lv_timer_del(carousel_timer);
                carousel_timer = NULL;

                // Delay before final flash + fade
                lv_timer_create([](lv_timer_t* t2) {
                    lv_anim_t flash;
                    lv_anim_init(&flash);
                    lv_anim_set_var(&flash, dice_container);
                    lv_anim_set_values(&flash, LV_OPA_TRANSP, LV_OPA_COVER);
                    lv_anim_set_time(&flash, 100);
                    lv_anim_set_exec_cb(&flash, [](void* obj, int32_t v) {
                        lv_obj_set_style_opa((lv_obj_t*)obj, v, 0);
                    });
                    lv_anim_set_ready_cb(&flash, [](lv_anim_t* a) {
                        lv_anim_t fade;
                        lv_anim_init(&fade);
                        lv_anim_set_var(&fade, dice_container);
                        lv_anim_set_values(&fade, LV_OPA_COVER, LV_OPA_TRANSP);
                        lv_anim_set_time(&fade, 200);
                        lv_anim_set_exec_cb(&fade, [](void* obj, int32_t v) {
                            lv_obj_set_style_opa((lv_obj_t*)obj, v, 0);
                        });
                        lv_anim_set_ready_cb(&fade, [](lv_anim_t* a2) {
                            cleanup_after_breakdown();
                        });
                        lv_anim_start(&fade);
                    });
                    lv_anim_start(&flash);
                    lv_timer_del(t2);
                }, 2500, NULL);
            }
        }, 2500, NULL);
    } else {
        // Single page, same as before
        carousel_timer = lv_timer_create([](lv_timer_t *t) {
            lv_anim_t fade_out;
            lv_anim_init(&fade_out);
            lv_anim_set_var(&fade_out, dice_container);
            lv_anim_set_values(&fade_out, LV_OPA_COVER, LV_OPA_TRANSP);
            lv_anim_set_time(&fade_out, 300);
            lv_anim_set_exec_cb(&fade_out, [](void* obj, int32_t v) {
                lv_obj_set_style_opa((lv_obj_t*)obj, v, 0);
            });
            lv_anim_set_ready_cb(&fade_out, [](lv_anim_t* a) {
                cleanup_after_breakdown();
            });
            lv_anim_start(&fade_out);
            lv_timer_del(t);
        }, 2500, NULL);
    }
}


/**
 * @brief Handles button input (single-click = power on, long-press = power off, double-click = modal)
 */
void handle_button_input() {
    int reading = digitalRead(inputPin);
    unsigned long now = millis();

    if (reading != lastButtonState) {
        lastDebounceTime = now;
    }

    if ((now - lastDebounceTime) > debounceDelay) {
        if (reading != buttonState) {
            buttonState = reading;

            if (buttonState == LOW) {  // Button Pressed
                if (now - lastClickTime < clickInterval) {
                    // Double-click detected
                    doubleClickDetected = true;
                    pendingSingleClick = false;
                    lastClickTime = 0;

                    if (!modal_bg) {
                        toggle_modal();
                        lastActivityTime = millis();
                    }
                } else {
                    // First click — might be a single
                    lastClickTime = now;

                    // Special case: if modal is open, treat this as instant single click
                    if (modal_bg != NULL) {
                        select_dice();  // ⚠️ instant modal tap
                        lastActivityTime = millis();
                        pendingSingleClick = false;
                        return;
                    }

                    // Otherwise defer the single click to see if it's a double
                    pendingSingleClick = true;
                }
            } else {
                if (longPressDetected) {
                    if (digitalRead(outputPin) == HIGH) {
                        USBSerial.println("Long press detected. Powering off...");
                        digitalWrite(outputPin, LOW);
                        noTone(beePin);
                    }
                    longPressDetected = false;
                }
            }
        }
    }

    // Single click logic
    if (pendingSingleClick && (now - lastClickTime > clickInterval)) {
        pendingSingleClick = false;
        clickDetected = true;

        if (isDeepSleeping || isSleeping) {
            // Handle waking explicitly if sleeping
            digitalWrite(LCD_BL, HIGH); 
            isSleeping = false;
            isDeepSleeping = false;
            lastActivityTime = millis();
            play_named_sound(wake_sound);
            USBSerial.println("Waking from sleep (single click detected)");
        } else if (modal_bg != NULL) {
            select_dice();
        } else {
            cycle_favorites();
        }
    }


    // Long press
    if (buttonState == LOW && (now - lastClickTime >= longPressDuration)) {
        if (!longPressDetected) {
            delay(50);
            if (digitalRead(inputPin) == LOW) {
                USBSerial.println("Long press detected!");
                tone(beePin, 2000);
                longPressDetected = true;
            }
        }
    }

    lastButtonState = reading;
}

void handle_modal_motion() {
    float pitch = atan2(-acc.x, sqrt(acc.y * acc.y + acc.z * acc.z)) * 180.0 / M_PI - basePitch;
    unsigned long now = millis();

    if (roll_breakdown_screen != NULL){
      return;
    }

    // Tilt navigation
    if (now - lastMoveTime > moveDebounceTime) {
        float threshold = 20.0;
        if (pitch < -threshold) {
            update_dice_selection(1); // UP
            lastMoveTime = now;
            lastActivityTime = now;
        } else if (pitch > threshold) {
            update_dice_selection(-1); // DOWN
            lastMoveTime = now;
            lastActivityTime = now;
        }
    }

    // Tap to close modal
    float tapThreshold = 0.4;
    float accel_magnitude = sqrt(acc.x * acc.x + acc.y * acc.y + acc.z * acc.z);
    if (accel_magnitude > tapThreshold && now - lastButtonPressTime > 500) {
        toggle_modal();  // Close modal on tap
        lastButtonPressTime = now;
        lastActivityTime = now;
    }
}


void handle_sensor_data() {
    if (qmi.getDataReady()) {
        qmi.getAccelerometer(acc.x, acc.y, acc.z);
        qmi.getGyroscope(gyr.x, gyr.y, gyr.z);

        // Wake on movement
        float tapThreshold = 0.4;
        float wakeOffset = 0.1; // Make it slightly easier to wake than to roll
        float accel_magnitude = sqrt(acc.x * acc.x + acc.y * acc.y + acc.z * acc.z);
        if (isSleeping && accel_magnitude > (tapThreshold - wakeOffset)) {
            wake_from_sleep();
            lastActivityTime = millis();
            return;
        }

        if (!isCalibrated) {
            baseRoll = atan2(acc.y, acc.z) * 180.0 / M_PI;
            basePitch = atan2(-acc.x, sqrt(acc.y * acc.y + acc.z * acc.z)) * 180.0 / M_PI;
            isCalibrated = true;
        }

        if (justWokeUp && millis() - wakeCooldownStart < wakeCooldownDuration) {
            return;
        } else {
            justWokeUp = false;
        }

        if (roll_breakdown_screen != NULL){
          return; // exit if in roll animation
        }

        // Always allow motion handling if modal is open
        if (modal_bg != NULL) {
            handle_modal_motion();
            return; // Don't roll while modal is up
        }

        if (!isRolling) {
            float total_accel = fabs(acc.x) + fabs(acc.y) + fabs(acc.z);
            if (total_accel > tapThreshold && millis() - lastButtonPressTime > 500) {
                roll_selected_dice();
                lastActivityTime = millis();
                lastButtonPressTime = millis();
            }
        }
    }
}


/**
 * @brief Updates dice selection in the modal.
 */
void update_dice_selection(int direction) {
    if (millis() - lastTiltTime < moveDebounceTime) return;

    selected_dice_index += direction;

    if (selected_dice_index >= NUM_DICE_OPTIONS) selected_dice_index = 0;
    if (selected_dice_index < 0) selected_dice_index = NUM_DICE_OPTIONS - 1;

    if (selected_dice_index < 8) {
        lv_label_set_text_fmt(dice_count_label, "x%d", dice_counts[selected_dice_index]);
    } else {
        lv_label_set_text(dice_count_label, ""); // hide count for special options
    }

    // Ensure correct image for mute option
    if (selected_dice_index == 10) {
        dice_images[10] = isMuted ? &mute : &volume;
        lv_label_set_text(dice_label, isMuted ? "Sound Off" : "Sound On");
    } else if (selected_dice_index == 0){
        lv_label_set_text(dice_label, "Reset Dice");
    } else {
        lv_label_set_text(dice_label, dice_names[selected_dice_index]);
    }
 
    lv_img_set_src(dice_img, dice_images[selected_dice_index]);  // ✅ Call this once, after updating
    lastTiltTime = millis();
}


/**
 * @brief Handles dice selection.
 */
void select_dice() {
    if (millis() - lastButtonPressTime < buttonDebounceTime) return;

    if (selected_dice_index == 7) {  // Percentile (only 1 allowed)
        memset(dice_counts, 0, sizeof(dice_counts));
        dice_counts[selected_dice_index] = 1;
    } else if (selected_dice_index == 0) {  // No dice (reset selection)
        memset(dice_counts, 0, sizeof(dice_counts));
        modifier = 0;
    } else if (selected_dice_index == 8) {  // CLEAR FAVORITES
        for (int i = 0; i < 5; i++) {
            memset(favorites[i].dice_counts, 0, sizeof(favorites[i].dice_counts));
            favorites[i].dice_counts[6] = 1; // reset to default 1d20
            favorites[i].modifier = 0;
        }
        save_favorites_to_nvs();
        USBSerial.println("Favorites reset to default (1d20).");
    } else if (selected_dice_index == 9) {  // CLEAR NVM (History + Favorites)
        prefs.begin("diceHist", false);
        prefs.clear();
        prefs.end();

        prefs.begin("diceFav", false);
        prefs.clear();
        prefs.end();

        USBSerial.println("NVM Cleared: Favorites and History reset.");
        ESP.restart();  // clean restart to fully apply
    } 
    else if (selected_dice_index == 10) {  // Mute toggle
        isMuted = !isMuted;
        dice_images[10] = isMuted ? &mute : &volume;  // Update image array
        lv_img_set_src(dice_img, dice_images[10]);   // 🔁 Force refresh displayed image
        save_mute_state();
        update_volume_indicator_label();
        USBSerial.printf("Mute toggled via modal: %s\n", isMuted ? "ON" : "OFF");
        tone(beePin, 800, 80); // Optional feedback
    }
    else {  // normal dice increment
        if (dice_counts[selected_dice_index] < 40) {
            dice_counts[selected_dice_index]++;
        }
    }

    tone(beePin, 1200, 80); // 🎵 Play Selection Sound
    lv_label_set_text_fmt(dice_count_label, "x%d", dice_counts[selected_dice_index]);
    update_selected_label(); // Update main screen label too
    lastButtonPressTime = millis();
}


// ESP32 RNG for rolling
int roll_dice(int sides) {
    return (esp_random() % sides) + 1;
}


// Updates the dice result label dynamically
void update_dice_result_label(int total) {
    char roll_result[16];
    snprintf(roll_result, sizeof(roll_result), "%d", total);
    lv_label_set_text(dice_result_label, roll_result);
    lv_label_set_text(prev_roll_label, roll_result);
}

// Handles dice rolling
// ** Animation function for rolling effect**

void roll_selected_dice() {
    if (isRolling) {
        USBSerial.println("Roll blocked: already rolling.");
        return;
    }
    
    if (roll_timer != NULL) {
        USBSerial.println("⚠️ roll_timer already exists, not starting another.");
        return;
    }

    play_named_sound(roll_sound);
    isRolling = true;  // 🔒 Set lock

    memset(rolled_values, 0, sizeof(rolled_values));
    memset(rolled_types, 0, sizeof(rolled_types));
    num_rolled_dice = 0;
    roll_step = 0;

    // Roll each selected dice type
    for (int i = 1; i < 8; i++) {
        if (dice_counts[i] > 0) {
            int die_size = 0;
            switch (i) {
                case 1: die_size = 4; break;
                case 2: die_size = 6; break;
                case 3: die_size = 8; break;
                case 4: die_size = 10; break;
                case 5: die_size = 12; break;
                case 6: die_size = 20; break;
                case 7: die_size = 100; break;
                default: die_size = 0; break;
            }

            if (die_size > 0) {
                for (int j = 0; j < dice_counts[i] && num_rolled_dice < MAX_ROLLED_DICE; j++) {
                    int roll = roll_dice(die_size);
                    rolled_values[num_rolled_dice] = roll;
                    rolled_types[num_rolled_dice] = die_size;
                    num_rolled_dice++;
                    USBSerial.printf("Rolled d%d: %d\n", die_size, roll);
                }
            }
        }
    }

    // Default to 1d20
    if (num_rolled_dice == 0) {
        rolled_values[0] = roll_dice(20);
        rolled_types[0] = 20;
        num_rolled_dice = 1;
    }

    roll_timer = lv_timer_create(roll_animation, 80, NULL);
}

void trigger_breakdown_after_delay() {
    USBSerial.println("🔥 trigger_breakdown_after_delay() called");

    if (breakdown_delay_timer) {
        lv_timer_del(breakdown_delay_timer); 
        breakdown_delay_timer = NULL;
    }

    breakdown_delay_timer = lv_timer_create(breakdown_timer_cb, 1000, NULL);
}

void breakdown_timer_cb(lv_timer_t *t) {
    USBSerial.println("📉 breakdown_timer_cb() firing");
    breakdown_delay_timer = NULL; // critical to NULL this immediately!
    show_roll_breakdown_screen();
    lv_timer_del(t);  // clean up this one-shot
}


void roll_animation(lv_timer_t *timer) {
    if (num_rolled_dice <= 0 || rolled_values == NULL || rolled_types == NULL) {
        USBSerial.println("Nothing to show in breakdown. Aborting.");
        return;
    }

    static int max_possible_roll = 0;
    if (roll_step == 0) {
        max_possible_roll = 0;
        for (int i = 0; i < num_rolled_dice; i++) {
            max_possible_roll += rolled_types[i];
        }
        max_possible_roll += modifier;
    }

    if (roll_step < 10) {
        int fake_roll = (esp_random() % max_possible_roll) + 1;
        char roll_text[8];
        snprintf(roll_text, sizeof(roll_text), "%d", fake_roll);
        lv_label_set_text(dice_result_label, roll_text);
        roll_step++;
        return;
    }

    // Final roll reveal
    int total = 0;
    for (int i = 0; i < num_rolled_dice; i++) {
        total += rolled_values[i];
    }
    total += modifier;

    snprintf(last_roll_result, sizeof(last_roll_result), "%d", total);
    lv_label_set_text(dice_result_label, last_roll_result);

    // --- QUOTE SELECTION ---
    if (num_rolled_dice == 1 && rolled_types[0] == 20) {
        // 1d20 Crit / Fail behavior (hard-coded)
        if (rolled_values[0] == 20) {
            USBSerial.println("🎯 CRITICAL HIT!");
            pending_quote_type = QUOTE_HAPPY;
        } else if (rolled_values[0] == 1) {
            USBSerial.println("💩 CRITICAL FAIL!");
            pending_quote_type = QUOTE_MAD;
        } else if ((esp_random() % 4) == 0) { // 25% chance to trigger
            pending_quote_type = QUOTE_NEUTRAL;
        } else {
            pending_quote_type = QUOTE_NONE;
        }
    } else {
        // Multi-dice evaluation — random chance
        if ((esp_random() % 4) == 0) { // 25% chance to trigger
            float efficiency_sum = 0.0f;
            int total_evaluated = 0;

            for (int i = 0; i < num_rolled_dice; i++) {
                int roll = rolled_values[i];
                int sides = rolled_types[i];

                if (sides <= 1) continue;

                float eff = (float)(roll - 1) / (float)(sides - 1);
                efficiency_sum += eff;
                total_evaluated++;
            }

            float avg_efficiency = total_evaluated > 0 ? (efficiency_sum / total_evaluated) : 0.5f;
            USBSerial.printf("🎯 Roll Efficiency: %.2f\n", avg_efficiency);

            if (avg_efficiency >= 0.75f) {
                pending_quote_type = QUOTE_HAPPY;
            } else if (avg_efficiency >= 0.45f) {
                pending_quote_type = QUOTE_NEUTRAL;
            } else {
                pending_quote_type = QUOTE_MAD;
            }
        } else {
            pending_quote_type = QUOTE_NONE;
        }
    }

    // --- Save roll history ---
    strncpy(last_five_rolls[last_roll_index], last_roll_result, sizeof(last_five_rolls[last_roll_index]));
    last_roll_index = (last_roll_index + 1) % 5;
    strncpy(history.last_roll, last_roll_result, sizeof(history.last_roll));
    for (int i = 0; i < 5; i++) {
        strncpy(history.last_five[i], last_five_rolls[i], sizeof(history.last_five[i]));
    }
    save_history_to_nvs();

    char roll_history[80];
    snprintf(roll_history, sizeof(roll_history), "%s | %s | %s | %s | %s",
        last_five_rolls[(last_roll_index + 4) % 5],
        last_five_rolls[(last_roll_index + 3) % 5],
        last_five_rolls[(last_roll_index + 2) % 5],
        last_five_rolls[(last_roll_index + 1) % 5],
        last_five_rolls[last_roll_index]);
    lv_label_set_text(prev_roll_label, roll_history);

    // Cleanup + Delay before breakdown
    max_possible_roll = 0;
    roll_step = 0;
    lv_timer_del(timer);
    roll_timer = NULL;

    trigger_breakdown_after_delay();
}


// Handles modifier selection
void adjust_modifier() {
    if (modifier < 10) {
        modifier++;
    } else {
        modifier = 0;
    }
    lv_label_set_text_fmt(modifier_label, "Modifier: +%d", modifier);
    update_selected_label();
}

/**
 * @brief Arduino Setup function
 */
void setup() {
    esp_log_level_set("*", ESP_LOG_VERBOSE);

    USBSerial.begin(115200);
    
    pinMode(inputPin, INPUT_PULLUP);
    pinMode(outputPin, OUTPUT);
    pinMode(beePin, OUTPUT);
    
    digitalWrite(outputPin, HIGH);  // **Ensure device stays powered on**
    delay(100);                      // **Small delay to allow power to stabilize**

    init_display();
    init_lvgl_timer();
    
    USBSerial.println("I am DiceMo bitch");
    init_sensors();
    
    create_main_ui();
    lv_scr_load(main_screen); 

    delay(500);
    update_battery_status();
    load_mute_state(); // Make sure we aren't playing a sound we shouldn't lol
    update_volume_indicator_label();
    show_dicemo_saying(QUOTE_WELCOME, boot_sound);
 
    // Set default dice selection at startup
    load_favorites_from_nvs();          // load from NVS
    load_history_from_nvs();
    apply_saved_roll_history();
    apply_favorite(current_fav_index);  // apply first favorite (default 1d20)
    USBSerial.printf("Free Heap: %d bytes\n", ESP.getFreeHeap());
}


/**
 * @brief Arduino Loop function
 */
void loop() {
    lv_timer_handler();
    delay(5);
    handle_button_input();
    handle_sensor_data();
    update_buzzer();

    unsigned long now = millis();

    if (now - lastBatteryUpdate > 10000) {
        update_battery_status();
        lastBatteryUpdate = now;
    }

    unsigned long idleTime = now - lastActivityTime;

    if (!isSleeping && idleTime > backlightTimeout) {
        enter_sleep_mode();  // backlight off at 45 seconds
    }

    if (idleTime > deepSleepTimeout) {
        enter_deep_sleep();  // full power-off at 2 minutes
        return;
    }

    if (isSleeping && quote_timer_active && (now - quote_timer_start >= next_quote_interval)) {
        show_dicemo_saying(QUOTE_MISC, misc_sound, false);
        quote_timer_start = now;
        next_quote_interval = random(backlightTimeout, deepSleepTimeout - 30000);
        USBSerial.println("Random Dicemo quote triggered!");
    }

    if (!isSleeping && quote_timer_active && (idleTime < backlightTimeout)) {
        quote_timer_active = false;
        USBSerial.println("Activity detected, quote timer reset.");
    }
}
