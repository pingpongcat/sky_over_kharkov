#ifndef LOCALIZATION_H
#define LOCALIZATION_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Language codes
typedef enum {
    LANG_ENGLISH = 0,
    LANG_POLISH = 1,
    LANG_UKRAINIAN = 2,
    LANG_COUNT = 3
} Language;

// Translation keys
typedef enum {
    // Main Menu
    STR_GAME_TITLE,
    STR_GAME_SUBTITLE,
    STR_GAME_INSTRUCTIONS,
    STR_SELECT_LEVEL,
    STR_LEVEL_1_DESC,
    STR_LEVEL_2_DESC,
    STR_LEVEL_3_DESC,
    STR_PRESS_OPTIONS,

    // Options Menu
    STR_OPTIONS,
    STR_SHOW_BREAKDOWN,
    STR_ALLOW_NEGATIVE,
    STR_MUSIC_VOLUME,
    STR_LANGUAGE,
    STR_CLOSE_OPTIONS,

    // In-Game
    STR_SCORE,
    STR_LEVEL,

    // Pause Menu
    STR_PAUSED,
    STR_PRESS_RESUME,

    // Game Over
    STR_OUT_OF_AMMO,

    // Language names
    STR_LANG_ENGLISH,
    STR_LANG_POLISH,
    STR_LANG_UKRAINIAN,

    STR_COUNT
} StringKey;

// Localization system structure
typedef struct {
    char* translations[LANG_COUNT][STR_COUNT];
    Language currentLanguage;
} LocalizationSystem;

// Global localization instance
static LocalizationSystem loc = {0};

// Get translated string
static inline const char* GetText(StringKey key) {
    if (key >= 0 && key < STR_COUNT && loc.currentLanguage >= 0 && loc.currentLanguage < LANG_COUNT) {
        return loc.translations[loc.currentLanguage][key];
    }
    return "";
}

// Get language name
static inline const char* GetLanguageName(Language lang) {
    switch(lang) {
        case LANG_ENGLISH: return GetText(STR_LANG_ENGLISH);
        case LANG_POLISH: return GetText(STR_LANG_POLISH);
        case LANG_UKRAINIAN: return GetText(STR_LANG_UKRAINIAN);
        default: return "Unknown";
    }
}

// Parse a line from INI file
static void ParseTranslationLine(const char* line, Language lang) {
    char key[256];
    char value[512];

    // Skip empty lines and comments
    if (line[0] == '\0' || line[0] == '#' || line[0] == ';') return;

    // Parse key=value format
    const char* equals = strchr(line, '=');
    if (!equals) return;

    // Extract key
    size_t keyLen = equals - line;
    if (keyLen >= sizeof(key)) return;
    strncpy(key, line, keyLen);
    key[keyLen] = '\0';

    // Trim key
    while (keyLen > 0 && (key[keyLen-1] == ' ' || key[keyLen-1] == '\t')) {
        key[--keyLen] = '\0';
    }

    // Extract value
    const char* valueStart = equals + 1;
    while (*valueStart == ' ' || *valueStart == '\t') valueStart++;

    strncpy(value, valueStart, sizeof(value) - 1);
    value[sizeof(value) - 1] = '\0';

    // Trim trailing whitespace and newlines
    size_t valueLen = strlen(value);
    while (valueLen > 0 && (value[valueLen-1] == ' ' || value[valueLen-1] == '\t' ||
                            value[valueLen-1] == '\n' || value[valueLen-1] == '\r')) {
        value[--valueLen] = '\0';
    }

    // Map key to StringKey enum
    StringKey strKey = STR_COUNT;

    if (strcmp(key, "GAME_TITLE") == 0) strKey = STR_GAME_TITLE;
    else if (strcmp(key, "GAME_SUBTITLE") == 0) strKey = STR_GAME_SUBTITLE;
    else if (strcmp(key, "GAME_INSTRUCTIONS") == 0) strKey = STR_GAME_INSTRUCTIONS;
    else if (strcmp(key, "SELECT_LEVEL") == 0) strKey = STR_SELECT_LEVEL;
    else if (strcmp(key, "LEVEL_1_DESC") == 0) strKey = STR_LEVEL_1_DESC;
    else if (strcmp(key, "LEVEL_2_DESC") == 0) strKey = STR_LEVEL_2_DESC;
    else if (strcmp(key, "LEVEL_3_DESC") == 0) strKey = STR_LEVEL_3_DESC;
    else if (strcmp(key, "PRESS_OPTIONS") == 0) strKey = STR_PRESS_OPTIONS;
    else if (strcmp(key, "OPTIONS") == 0) strKey = STR_OPTIONS;
    else if (strcmp(key, "SHOW_BREAKDOWN") == 0) strKey = STR_SHOW_BREAKDOWN;
    else if (strcmp(key, "ALLOW_NEGATIVE") == 0) strKey = STR_ALLOW_NEGATIVE;
    else if (strcmp(key, "MUSIC_VOLUME") == 0) strKey = STR_MUSIC_VOLUME;
    else if (strcmp(key, "LANGUAGE") == 0) strKey = STR_LANGUAGE;
    else if (strcmp(key, "CLOSE_OPTIONS") == 0) strKey = STR_CLOSE_OPTIONS;
    else if (strcmp(key, "SCORE") == 0) strKey = STR_SCORE;
    else if (strcmp(key, "LEVEL") == 0) strKey = STR_LEVEL;
    else if (strcmp(key, "PAUSED") == 0) strKey = STR_PAUSED;
    else if (strcmp(key, "PRESS_RESUME") == 0) strKey = STR_PRESS_RESUME;
    else if (strcmp(key, "OUT_OF_AMMO") == 0) strKey = STR_OUT_OF_AMMO;
    else if (strcmp(key, "LANG_ENGLISH") == 0) strKey = STR_LANG_ENGLISH;
    else if (strcmp(key, "LANG_POLISH") == 0) strKey = STR_LANG_POLISH;
    else if (strcmp(key, "LANG_UKRAINIAN") == 0) strKey = STR_LANG_UKRAINIAN;

    if (strKey != STR_COUNT) {
        // Allocate and store the translation
        loc.translations[lang][strKey] = (char*)malloc(strlen(value) + 1);
        if (loc.translations[lang][strKey]) {
            strcpy(loc.translations[lang][strKey], value);
        }
    }
}

// Load translations from INI file
static void LoadTranslations(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        printf("Warning: Could not open translations file: %s\n", filename);
        return;
    }

    char line[1024];
    Language currentLang = LANG_ENGLISH;

    while (fgets(line, sizeof(line), file)) {
        // Check for language section headers
        if (strncmp(line, "[English]", 9) == 0) {
            currentLang = LANG_ENGLISH;
        } else if (strncmp(line, "[Polish]", 8) == 0) {
            currentLang = LANG_POLISH;
        } else if (strncmp(line, "[Ukrainian]", 11) == 0) {
            currentLang = LANG_UKRAINIAN;
        } else {
            ParseTranslationLine(line, currentLang);
        }
    }

    fclose(file);
}

// Initialize localization system
static void InitLocalization(const char* filename, Language defaultLang) {
    // Initialize all pointers to NULL
    for (int i = 0; i < LANG_COUNT; i++) {
        for (int j = 0; j < STR_COUNT; j++) {
            loc.translations[i][j] = NULL;
        }
    }

    loc.currentLanguage = defaultLang;
    LoadTranslations(filename);
}

// Set current language
static void SetLanguage(Language lang) {
    if (lang >= 0 && lang < LANG_COUNT) {
        loc.currentLanguage = lang;
    }
}

// Get current language
static Language GetCurrentLanguage(void) {
    return loc.currentLanguage;
}

// Cleanup localization system
static void CleanupLocalization(void) {
    for (int i = 0; i < LANG_COUNT; i++) {
        for (int j = 0; j < STR_COUNT; j++) {
            if (loc.translations[i][j]) {
                free(loc.translations[i][j]);
                loc.translations[i][j] = NULL;
            }
        }
    }
}

#endif // LOCALIZATION_H
