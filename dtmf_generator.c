#include <furi.h>
#include <furi_hal.h>
#include <gui/gui.h>
#include <gui/elements.h>
#include <input/input.h>
#include <furi_hal_speaker.h>
#include <float_tools.h>

#include "dtmf_generator_icons.h"
#define TAG "DTMF Generator"
#define ARRAYSIZE(a) (sizeof(a) / sizeof(*(a)))

/*
DTMF Tones are defined as the combination of two pure tone sine waves, represented by the following table:
                    |        | 1209 Hz | 1336 Hz | 1477 Hz | 1633 Hz |
                    |--------|---------|---------|---------|---------|
                    | 697 Hz | 1       | 2       | 3       | A       |
                    | 770 Hz | 4       | 5       | 6       | B       |
                    | 852 Hz | 7       | 8       | 9       | C       |
                    | 941 Hz | *       | 0       | #       | D       |
*/

/** @struct DTMFTone
 *  @brief This structure represents an individual DTMF Tone
 *  @var DTMFTone::key
 *  Represents the key being pressed
 *  @var DTMFTone::low
 *  Contains the low frequency of the tone in Hz
 *  @var DTMFTone::low_str
 *  Contains the low frequency of the tone as a string
 *  @var DTMFTone::high
 *  Contains the high frequency of the tone in Hz
 *  @var DTMFTone::high_str
 *  Contains the high frequency of the tone as a string
 */
typedef struct {
    char key;
    int32_t low;
    char low_str[8];
    int32_t high;
    char high_str[8];
} DTMFTone;

/**
 * @brief Holds the index of the DTMF Tone
 * @var DTMF_SELECTED::DTMFTone1
 * Index of the DTMF Tone 1
 * @var DTMF_SELECTED::DTMFTone2
 * Index of the DTMF Tone 2
 * @var DTMF_SELECTED::DTMFTone3
 * Index of the DTMF Tone 3
 * @var DTMF_SELECTED::DTMFToneA
 * Index of the DTMF Tone A
 * @var DTMF_SELECTED::DTMFTone4
 * Index of the DTMF Tone 4
 * @var DTMF_SELECTED::DTMFTone5
 * Index of the DTMF Tone 5
 * @var DTMF_SELECTED::DTMFTone6
 * Index of the DTMF Tone 6
 * @var DTMF_SELECTED::DTMFToneB
 * Index of the DTMF Tone B
 * @var DTMF_SELECTED::DTMFTone7
 * Index of the DTMF Tone 7
 * @var DTMF_SELECTED::DTMFTone8
 * Index of the DTMF Tone 8
 * @var DTMF_SELECTED::DTMFTone9
 * Index of the DTMF Tone 9
 * @var DTMF_SELECTED::DTMFToneC
 * Index of the DTMF Tone C
 * @var DTMF_SELECTED::DTMFToneStar
 * Index of the DTMF Tone *
 * @var DTMF_SELECTED::DTMFTone0
 * Index of the DTMF Tone 0
 * @var DTMF_SELECTED::DTMFToneHash
 * Index of the DTMF Tone #
 * @var DTMF_SELECTED::DTMFToneD
 * Index of the DTMF Tone D
 * @var DTMF_SELECTED::DTMFPlayStop
 * Represents the play/stop button
 * @var DTMF_SELECTED::DTMFVolumeUp
 * Represents the Volume Up button
 * @var DTMF_SELECTED::DTMFVolumeDown
 * Represents the Volume Down button
 * @var DTMF_SELECTED::DTMFToneInvalid
 * Used for counting and to represent an invalid tone
 */
typedef enum DTMF_SELECTED {
    DTMFTone1,
    DTMFTone2,
    DTMFTone3,
    DTMFToneA,
    DTMFTone4,
    DTMFTone5,
    DTMFTone6,
    DTMFToneB,
    DTMFTone7,
    DTMFTone8,
    DTMFTone9,
    DTMFToneC,
    DTMFToneStar,
    DTMFTone0,
    DTMFToneHash,
    DTMFToneD,
    DTMFPlayStop,
    DTMFVolumeUp,
    DTMFVolumeDown,
    DTMFToneInvalid,
} DTMF_SELECTED;

/** @struct DTMFState
 *  @brief This structure represents the current state of the program
 *  @var DTMFState::playing
 *  True if a tone is currently playing, false otherwise
 *  @var DTMFState::volume
 *  Holds the current volume
 *  @var DTMFState::tone
 *  Holds the currrent tone
 *  @var DTMFState::selected
 *  Holds the currently selected "button"
 */
typedef struct {
    bool playing;
    int volume;
    DTMFTone* tone;
    DTMF_SELECTED selected;
} DTMFState;

/** @brief Represents the keyboard layout */
static DTMF_SELECTED keyboard[4][5] = {
    {DTMFTone1, DTMFTone2, DTMFTone3, DTMFToneA, DTMFToneInvalid},
    {DTMFTone4, DTMFTone5, DTMFTone6, DTMFToneB, DTMFPlayStop},
    {DTMFTone7, DTMFTone8, DTMFTone9, DTMFToneC, DTMFVolumeUp},
    {DTMFToneStar, DTMFTone0, DTMFToneHash, DTMFToneD, DTMFVolumeDown},
};

/** @brief Holds an array of valid DTMF Tones
 *  @note You can use the DTMF_SELECTED enum to access the tones in this array
 */
static DTMFTone tones[] = {
    {'1', 697, "697 Hz", 1209, "1209 Hz"},
    {'2', 697, "697 Hz", 1336, "1336 Hz"},
    {'3', 697, "697 Hz", 1477, "1477 Hz"},
    {'A', 697, "697 Hz", 1633, "1633 Hz"},
    {'4', 770, "770 Hz", 1209, "1209 Hz"},
    {'5', 770, "770 Hz", 1336, "1336 Hz"},
    {'6', 770, "770 Hz", 1477, "1477 Hz"},
    {'B', 770, "770 Hz", 1633, "1633 Hz"},
    {'7', 852, "852 Hz", 1209, "1209 Hz"},
    {'8', 852, "852 Hz", 1336, "1336 Hz"},
    {'9', 852, "852 Hz", 1477, "1477 Hz"},
    {'C', 852, "852 Hz", 1633, "1633 Hz"},
    {'*', 941, "941 Hz", 1209, "1209 Hz"},
    {'0', 941, "941 Hz", 1336, "1336 Hz"},
    {'#', 941, "941 Hz", 1477, "1477 Hz"},
    {'D', 941, "941 Hz", 1633, "1633 Hz"},
};

/**
 * @brief Callback for when a key is pressed
 *
 * @param input_event Input Event that was generated
 * @param ctx State that's passed as a context
 */
static void dtmf_generator_input_callback(InputEvent* input_event, void* ctx) {
    furi_assert(ctx);

    FuriMessageQueue* event_queue = ctx;
    furi_message_queue_put(event_queue, input_event, FuriWaitForever);
}

/**
 * @brief Callback for when the application needs to draw the screen
 *
 * @param canvas Canvs to draw on
 * @param context State that's passed as a context
 */
static void dtmf_generator_draw_callback(Canvas* canvas, void* context) {
    DTMFState* state = context;
    furi_assert(state, "Invalid context in dtmf_generator_draw_callback");
    if(!state) {
        return;
    }

    /**  Header **/
    canvas_set_font(canvas, FontPrimary);
    elements_multiline_text_aligned(canvas, 127, 3, AlignRight, AlignTop, "DTMF Gen");
    /** !Header **/

    /**  Keypad */
    canvas_set_font(canvas, FontSecondary);
    for(size_t i = 0; i < DTMFPlayStop; i++) {
        DTMFTone* tone = &tones[i];
        uint8_t x = 5 + (i % 4) * 15; // 50
        uint8_t y = 12 + (i / 4) * 15; // 68

        // The '1' is too skinny and looks weird by itself - still does, but less so now :)
        if(tone->key == '1') {
            elements_multiline_text_framed(canvas, x, y, "1 ");
        } else {
            elements_multiline_text_framed(canvas, x, y, &tone->key);
        }

        // Now we have to draw the underline to show the currently selected tone
        if(state->selected == i) {
            canvas_draw_box(canvas, x, y, 12, 3);
        }
    }
    /** !Keypad */

    /**  Play */
    elements_multiline_text_framed(canvas, 65, 27, " >");
    if(state->selected == DTMFPlayStop) {
        canvas_draw_box(canvas, 65, 27, 12, 3);
    }
    /** !Play */

    /**  Volume */
    elements_multiline_text_framed(canvas, 65, 42, "+");
    if(state->selected == DTMFVolumeUp) {
        canvas_draw_box(canvas, 65, 42, 12, 3);
    }
    elements_multiline_text_framed(canvas, 65, 57, "-");
    if(state->selected == DTMFVolumeDown) {
        canvas_draw_box(canvas, 65, 57, 12, 3);
    }
    /** !Volume */

    /**  Status */
    elements_multiline_text(canvas, 85, 27, "Vol:");
    if(state->volume == 0) {
        elements_multiline_text(canvas, 105, 27, "0%");
    } else if(state->volume == 1) {
        elements_multiline_text(canvas, 105, 27, "10%");
    } else if(state->volume == 2) {
        elements_multiline_text(canvas, 105, 27, "20%");
    } else if(state->volume == 3) {
        elements_multiline_text(canvas, 105, 27, "30%");
    } else if(state->volume == 4) {
        elements_multiline_text(canvas, 105, 27, "40%");
    } else if(state->volume == 5) {
        elements_multiline_text(canvas, 105, 27, "50%");
    } else if(state->volume == 6) {
        elements_multiline_text(canvas, 105, 27, "60%");
    } else if(state->volume == 7) {
        elements_multiline_text(canvas, 105, 27, "70%");
    } else if(state->volume == 8) {
        elements_multiline_text(canvas, 105, 27, "80%");
    } else if(state->volume == 9) {
        elements_multiline_text(canvas, 105, 27, "90%");
    } else if(state->volume == 10) {
        elements_multiline_text(canvas, 105, 27, "100%");
    } else {
        elements_multiline_text(canvas, 105, 27, "");
    }
    elements_multiline_text(canvas, 85, 42, "Freq: ");
    if(state->tone) {
        char* str = state->tone->high_str;
        elements_multiline_text(canvas, 85, 57, str);
    }
    /** !Status */
}

/**
 * @brief Moves the selected key
 * 
 * @param selected Currently selected "button"
 * @param key Key Press
 */
static void move_selected(DTMFState* state, InputKey key) {
    if(!state) {
        return;
    }

    for(size_t i = 0; i < ARRAYSIZE(keyboard); i++) {
        for(size_t j = 0; j < ARRAYSIZE(keyboard[0]); j++) {
            if(keyboard[i][j] == state->selected) {
                switch(key) {
                case InputKeyUp:
                    if(i > 0) {
                        state->selected = keyboard[i - 1][j];
                    } else {
                        state->selected = keyboard[ARRAYSIZE(keyboard) - 1][j];
                    }
                    goto check_invalid_key;
                case InputKeyDown:
                    if(i < ARRAYSIZE(keyboard) - 1) {
                        state->selected = keyboard[i + 1][j];
                    } else {
                        state->selected = keyboard[0][j];
                    }
                    goto check_invalid_key;
                case InputKeyLeft:
                    if(j > 0) {
                        state->selected = keyboard[i][j - 1];
                    } else {
                        state->selected = keyboard[i][ARRAYSIZE(keyboard[0]) - 1];
                    }
                    goto check_invalid_key;
                case InputKeyRight:
                    if(j < ARRAYSIZE(keyboard[0]) - 1) {
                        state->selected = keyboard[i][j + 1];
                    } else {
                        state->selected = keyboard[i][0];
                    }
                    goto check_invalid_key;
                default:
                    break;
                }
            }
        }
    }

check_invalid_key:
    // Handle a special case - where a user gets to the Invalid Button
    //  in which case we just skip over it in the same direction, by
    //  calling this function again
    if(state->selected == DTMFToneInvalid) {
        move_selected(state, key);
    }
}

/**
 * @brief Turns off the sound if it belongs to us
 * 
 */
static void sound_off() {
    if(furi_hal_speaker_is_mine()) {
        furi_hal_speaker_stop();
        furi_hal_speaker_release();
    }
}

/**
 * @brief Turns on the sound
 * 
 * @param state Current DTMFState
 */
static void sound_on(DTMFState* state) {
    if(!state || !state->tone) {
        return;
    }

    if(furi_hal_speaker_is_mine() || furi_hal_speaker_acquire(30)) {
        furi_hal_speaker_start(state->tone->high, (float)state->volume / (float)100);
    }
}

/**
 * @brief Main entry point to the DTMF Generator application
 * 
 * @param p UNUSED
 * @return int32_t Status code 
 */
int32_t dtmf_generator_main(void* p) {
    UNUSED(p);

    InputEvent event;
    DTMFState* state = NULL;
    ViewPort* view_port = NULL;
    FuriMessageQueue* event_queue = NULL;
    Gui* gui = NULL;

    state = malloc(sizeof(DTMFState));
    if(!state) {
        return -1;
    }
    state->playing = false;
    state->volume = 5;
    state->tone = NULL;
    state->selected = DTMFTone1;

    event_queue = furi_message_queue_alloc(8, sizeof(InputEvent));
    furi_assert(event_queue, "Failed to allocate event_queue");
    view_port = view_port_alloc();
    furi_assert(view_port, "Failed to allocate view_port");

    view_port_draw_callback_set(view_port, dtmf_generator_draw_callback, state);
    view_port_input_callback_set(view_port, dtmf_generator_input_callback, event_queue);

    gui = furi_record_open(RECORD_GUI);
    furi_assert(gui, "Failed to open gui record");
    gui_add_view_port(gui, view_port, GuiLayerFullscreen);

    bool running = true;
    while(running) {
        if(furi_message_queue_get(event_queue, &event, 100) == FuriStatusOk) {
            if(event.type == InputTypePress) {
                switch(event.key) {
                case InputKeyOk:
                    if(state->selected == DTMFPlayStop) {
                        break;
                    } else if(state->selected == DTMFVolumeUp) {
                        if(state->volume < 10) {
                            state->volume++;
                        }
                    } else if(state->selected == DTMFVolumeDown) {
                        if(state->volume > 0) {
                            state->volume--;
                        }
                    } else {
                        state->tone = &tones[state->selected];
                    }
                    break;
                case InputKeyLeft:
                case InputKeyRight:
                case InputKeyUp:
                case InputKeyDown:
                    move_selected(state, event.key);
                    break;
                default:
                    running = false;
                    break;
                }
            }
            if(event.type == InputTypeRelease) {
                switch(event.key) {
                case InputKeyOk:
                    if(state->selected == DTMFPlayStop) {
                        sound_off();
                        state->playing = false;
                    }
                    break;
                default:
                    break;
                }
            }
            if(event.type == InputTypeLong) {
                switch(event.key) {
                case InputKeyOk:
                    if(state->selected == DTMFPlayStop) {
                        state->playing = true;
                        sound_on(state);
                    }
                    break;
                default:
                    break;
                }
            }
        }
        view_port_update(view_port);
    }

    sound_off();

    view_port_enabled_set(view_port, false);
    gui_remove_view_port(gui, view_port);
    view_port_free(view_port);
    furi_message_queue_free(event_queue);

    furi_record_close(RECORD_GUI);

    free(state);

    return 0;
}
