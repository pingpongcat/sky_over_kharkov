#include "raylib.h"
#include "localization.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <string.h>

//------------------------------------------------------------------------------------
// Constants
//------------------------------------------------------------------------------------
// Game configuration
#define MAX_DRONES 15
#define MAX_PROJECTILES 10
#define INITIAL_AMMO 10

// Gameplay constants
#define SHOT_COST 2
#define HIT_REWARD 3
#define SCORE_CORRECT_HIT 10
#define SCORE_WRONG_HIT -5
#define MAX_AMMO 20

// Physics constants
#define DRONE_SPEED 70.0f
#define PROJECTILE_SPEED 3000.0f
#define DRONE_FALL_SPEED 150.0f
#define DRONE_FALL_HORIZONTAL_MULTIPLIER 0.5f
#define DRONE_MIN_SCALE 0.2f
#define PROJECTILE_HIT_RADIUS 0.3f
#define PROJECTILE_MAX_LIFETIME 2.0f

// Sprite scaling constants
#define DRONE_SCALE 2.0f        // Drone sprite scale (1.5 = 150%)
#define GEPARD_SCALE 2.0f       // Gepard tank scale (2.0 = 200%)
#define GEPARD_TEXTURE_SIZE 150 // Original Gepard texture size (150x150)
#define DRONE_TEXTURE_SIZE 100  // Original drone texture size (100x100)

// Screen and layout constants
#define SCREEN_WIDTH 1107
#define SCREEN_HEIGHT 694
#define GROUND_LEVEL 394.0f
#define GROUND_EXPLOSION_OFFSET 200.0f
#define NEAR_GROUND_LEVEL 494.0f

// Gepard barrel positions (as ratios of sprite size)
#define GEPARD_BARREL_LEFT_X 0.67f
#define GEPARD_BARREL_RIGHT_X 0.83f
#define GEPARD_BARREL_Y 0.63f

// Drone spawn constants
#define DRONE_SPAWN_X 1200.0f
#define DRONE_SPAWN_SPACING 150.0f
#define DRONE_SPAWN_Y_MIN 80.0f
#define DRONE_SPAWN_Y_RANGE 250.0f
#define DRONE_MIN_COUNT 2
#define DRONE_MAX_COUNT 2

// Drone target offsets for dual barrels
#define DRONE_TARGET_OFFSET 10.0f

// Animation timing
#define EXPLOSION_DURATION 0.3f
#define FIRE_FRAME_DURATION 0.05f
#define BLINK_FREQUENCY 10.0f

// UI constants
#define AMMO_BOX_WIDTH 15
#define AMMO_BOX_HEIGHT 8
#define AMMO_BOX_SPACING 3
#define AMMO_DISPLAY_OFFSET_X 50
#define AMMO_DISPLAY_OFFSET_Y 60
#define AMMO_WARNING_THRESHOLD 10
#define AMMO_CRITICAL_THRESHOLD 5

// Font constants for pixel-perfect rendering
#define MECHA_SPACING 8
#define SETBACK_SPACING 4
#define ROMULUS_SPACING 3
#define ALPHA_BETA_SPACING 4
#define PIXANTIQUA_SPACING 4

// Spawn timing
#define SPAWN_INTERVAL 3.0f
#define RESPAWN_DELAY 1.0f

// Drone animation constants
#define DRONE_FALL_START_Y 100.0f
#define DRONE_FALL_END_Y GROUND_LEVEL
#define DRONE_TEXT_OFFSET_X 95.0f
#define DRONE_TEXT_OFFSET_Y 30.0f

// Projectile visual constants
#define PROJECTILE_TRAIL_LENGTH 0.02f
#define PROJECTILE_LINE_THICKNESS 3.0f
#define PROJECTILE_DOT_RADIUS 2.0f

// Off-screen boundaries
#define OFF_SCREEN_LEFT -150.0f
#define OFF_SCREEN_RIGHT 1200.0f
#define OFF_SCREEN_TOP -10.0f
#define OFF_SCREEN_BOTTOM 750.0f
#define DRONE_LEFT_BOUNDARY 100.0f

//------------------------------------------------------------------------------------
// Types and Structures Definition
//------------------------------------------------------------------------------------
typedef enum {
    DRONE_FLYING = 0,
    DRONE_EXPLODING,
    DRONE_FALLING,
    DRONE_DEAD
} DroneState;

typedef struct Drone {
    Vector2 position;
    int answer;
    bool isShahed;
    DroneState state;
    float animTimer;
    bool active;
} Drone;

typedef struct {
    int turretIndex;    // 0-4, which turret position
    float fireTimer;
    bool isFiring;
    int fireFrame;      // 0 = bottom, 1 = middle, 2 = top
} GepardTank;

typedef enum {
    PART_NORMAL = 0,    // Normal display (blue)
    PART_CANCELLED,     // Cancelled pairs (red)
    PART_HIGHLIGHT      // Highlighted tens in addition (green)
} PartVisualState;

typedef struct {
    int value;
    char operatorBefore; // '+', '-', or '\0' for first element
    PartVisualState visualState;
} DecomposedPart;

typedef struct {
    int num1;
    int num2;
    char operation;
    int correctAnswer;
    char decomposed[128]; // String representation of decomposed equation
    DecomposedPart parts[20]; // Individual parts for visual rendering
    int partCount;
} MathEquation;

typedef struct {
    Vector2 position;
    Vector2 velocity;
    bool active;
    float lifetime;
    int targetDroneIndex;
} Projectile;

// Helper structures for reducing redundant calculations
typedef struct {
    float scale;
    float offsetX;
    float offsetY;
    float drawWidth;
    float drawHeight;
    Vector2 mousePos;
} RenderContext;

typedef struct {
    float width;
    float height;
    Vector2 center;
    Rectangle bounds;
} DroneBounds;

typedef struct {
    bool shahedFound;
    bool canWin;
    int aliveCount;
} DroneStatus;

//------------------------------------------------------------------------------------
// Function Declarations
//------------------------------------------------------------------------------------
// Game logic functions
void DecomposeNumber(int num, int *tens, int *ones);
void CreateDecomposedEquation(MathEquation *eq);
void GenerateNewEquation(MathEquation *eq, int level, Drone drones[], bool allowNegative);
void SpawnDrones(Drone drones[], MathEquation *eq, int *activeDroneCount);
void UpdateDrones(Drone drones[], float deltaTime);
void UpdateGepard(GepardTank *gepard, float deltaTime);
void UpdateProjectiles(Projectile projectiles[], Drone drones[], int *ammo, int *score, bool *shahedActive, float deltaTime, Sound explosionSound);
void SpawnProjectile(Projectile projectiles[], Vector2 start, Vector2 target, int droneIndex);
int GetTurretIndexFromMouse(int mouseX, int screenWidth);

// Drawing functions
void DrawDrone(Texture2D texture, Drone drone);
void DrawGepard(Texture2D texture, GepardTank gepard, Vector2 position);
void DrawAmmo(int ammo, int screenWidth, int screenHeight);
void DrawProjectiles(Projectile projectiles[]);
void DrawDecomposedEquation(MathEquation *eq, Font font, Vector2 position, float fontSize, float spacing, float blinkTimer);

// Helper functions to reduce redundant calculations
RenderContext CalculateRenderContext(int screenWidth, int screenHeight);
DroneBounds GetDroneBounds(Drone drone);
DroneStatus CheckDroneStatus(Drone drones[]);
Vector2 GetBarrelPosition(Vector2 gepardPos, bool isLeftBarrel);

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void)
{
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = SCREEN_WIDTH;
    const int screenHeight = SCREEN_HEIGHT;

    InitWindow(screenWidth, screenHeight, "Sky Over Kharkiv");
    InitAudioDevice();
    SetTargetFPS(60);
    SetWindowState(FLAG_WINDOW_RESIZABLE);
    SetMasterVolume(0.5f); // Initialize with default volume

    // Seed random
    srand(time(NULL));

    // Initialize localization system (Polish as default)
    InitLocalization("translations.ini", LANG_POLISH);

    // Load Raylib's sprite fonts
    Font mechaFont = LoadFont("fonts/mecha.png");
    Font setbackFont = LoadFont("fonts/setback.png");
    Font romulusFont = LoadFont("fonts/romulus.png");
    Font alphaBetaFont = LoadFont("fonts/alpha_beta.png");
    Font pixantiquaFont = LoadFont("fonts/pixantiqua.png");

    if (mechaFont.texture.id == 0 || setbackFont.texture.id == 0 || romulusFont.texture.id == 0 || alphaBetaFont.texture.id == 0 || pixantiquaFont.texture.id == 0) {
        printf("ERROR: Failed to load sprite fonts! Using default.\n");
        if (mechaFont.texture.id == 0) mechaFont = GetFontDefault();
        if (setbackFont.texture.id == 0) setbackFont = GetFontDefault();
        if (romulusFont.texture.id == 0) romulusFont = GetFontDefault();
        if (alphaBetaFont.texture.id == 0) alphaBetaFont = GetFontDefault();
        if (pixantiquaFont.texture.id == 0) pixantiquaFont = GetFontDefault();
    } else {
        printf("Sprite fonts loaded successfully!\n");
        SetTextureFilter(mechaFont.texture, TEXTURE_FILTER_POINT);
        SetTextureFilter(setbackFont.texture, TEXTURE_FILTER_POINT);
        SetTextureFilter(romulusFont.texture, TEXTURE_FILTER_POINT);
        SetTextureFilter(alphaBetaFont.texture, TEXTURE_FILTER_POINT);
        SetTextureFilter(pixantiquaFont.texture, TEXTURE_FILTER_POINT);
    }

    // Load textures
    Texture2D sahedTexture = LoadTexture("images/sahed.png");
    Texture2D gepardTexture = LoadTexture("images/gepard.png");
    Texture2D backgroundTexture = LoadTexture("images/background.png");

    // Load flag textures for language selection
    Texture2D flagGB = LoadTexture("images/gb.jpg");
    Texture2D flagPL = LoadTexture("images/pl.jpg");
    Texture2D flagUA = LoadTexture("images/ua.jpg");

    // Load sounds
    Sound shootSound = LoadSound("sounds/fire_burst.wav");
    Sound explosionSound = LoadSound("sounds/explosion.wav");

    // Create render texture for scaling
    RenderTexture2D target = LoadRenderTexture(screenWidth, screenHeight);
    SetTextureFilter(target.texture, TEXTURE_FILTER_BILINEAR);

    // Game variables
    GepardTank gepard = { 0, 0.0f, false, 0 };
    Vector2 gepardPosition = { 120.0f, (float)screenHeight - 40.0f - (GEPARD_TEXTURE_SIZE * GEPARD_SCALE) };

    Drone drones[MAX_DRONES] = {0};
    int activeDroneCount = 0;

    Projectile projectiles[MAX_PROJECTILES] = {0};

    MathEquation currentEquation = {0};
    int ammo = INITIAL_AMMO;
    int score = 0;
    int level = 1;
    bool shahedActive = false; // Track if Shahed from current equation is still active

    float spawnTimer = 0.0f;

    bool levelSelected = false;
    bool gameStarted = false;
    bool paused = false;
    bool showOptionsMenu = false;

    // Options/Settings
    bool showEquationBreakdown = false; // Don't show decomposed equation by default
    bool allowNegativeResults = false; // Don't allow negative results by default
    float musicVolume = 0.5f; // 0.0 to 1.0

    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!WindowShouldClose())
    {
        float deltaTime = GetFrameTime();

        // Update
        //----------------------------------------------------------------------------------

        // Fullscreen toggle with F key
        if (IsKeyPressed(KEY_F)) {
            ToggleBorderlessWindowed();
        }

        // Options menu toggle with O key
        if (IsKeyPressed(KEY_O)) {
            showOptionsMenu = !showOptionsMenu;
            if (showOptionsMenu && gameStarted) {
                paused = true; // Auto-pause when opening options during game
            }
        }

        if (!levelSelected) {
            // Level selection screen

            // Handle flag clicks for language selection
            if (!showOptionsMenu) {
                RenderContext ctx = CalculateRenderContext(screenWidth, screenHeight);

                // Define flag positions (must match drawing positions)
                const float flagSize = 60.0f;
                const float flagSpacing = 20.0f;
                const float flagY = screenHeight - 100.0f;
                Rectangle flagRectGB = {screenWidth/2 - flagSize - flagSpacing - flagSize/2, flagY, flagSize, flagSize * 0.6f};
                Rectangle flagRectPL = {screenWidth/2 - flagSize/2, flagY, flagSize, flagSize * 0.6f};
                Rectangle flagRectUA = {screenWidth/2 + flagSpacing + flagSize/2, flagY, flagSize, flagSize * 0.6f};

                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    if (CheckCollisionPointRec(ctx.mousePos, flagRectGB)) {
                        SetLanguage(LANG_ENGLISH);
                    } else if (CheckCollisionPointRec(ctx.mousePos, flagRectPL)) {
                        SetLanguage(LANG_POLISH);
                    } else if (CheckCollisionPointRec(ctx.mousePos, flagRectUA)) {
                        SetLanguage(LANG_UKRAINIAN);
                    }
                }
            }

            if (IsKeyPressed(KEY_ONE)) {
                level = 1;
                levelSelected = true;
            } else if (IsKeyPressed(KEY_TWO)) {
                level = 2;
                levelSelected = true;
            } else if (IsKeyPressed(KEY_THREE)) {
                level = 3;
                levelSelected = true;
            }

            if (levelSelected) {
                gameStarted = true;
                GenerateNewEquation(&currentEquation, level, drones, allowNegativeResults);
                SpawnDrones(drones, &currentEquation, &activeDroneCount);
                shahedActive = true;
            }
        }

        // Handle options menu interactions (works in both level select and game)
        if (showOptionsMenu) {
            // Calculate render context for mouse position
            RenderContext ctx = CalculateRenderContext(screenWidth, screenHeight);

            // Define UI element positions (must match drawing positions)
            Rectangle breakdownCheckbox = {screenWidth/2 + 180, screenHeight/2 - 80, 30, 30};
            Rectangle negativeCheckbox = {screenWidth/2 + 180, screenHeight/2 - 30, 30, 30};
            Rectangle volumeSlider = {screenWidth/2 - 100, screenHeight/2 + 50, 200, 20};

            // Handle checkbox clicks
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                // Equation breakdown toggle
                if (CheckCollisionPointRec(ctx.mousePos, breakdownCheckbox)) {
                    showEquationBreakdown = !showEquationBreakdown;
                }
                // Allow negative results toggle
                if (CheckCollisionPointRec(ctx.mousePos, negativeCheckbox)) {
                    allowNegativeResults = !allowNegativeResults;
                }
            }

            // Handle volume slider drag
            if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
                if (CheckCollisionPointRec(ctx.mousePos, volumeSlider)) {
                    float sliderValue = (ctx.mousePos.x - volumeSlider.x) / volumeSlider.width;
                    if (sliderValue < 0.0f) sliderValue = 0.0f;
                    if (sliderValue > 1.0f) sliderValue = 1.0f;
                    musicVolume = sliderValue;
                    SetMasterVolume(musicVolume);
                }
            }
        }

        if (gameStarted) {
            // Toggle pause (only when options menu is not shown and game is running)
            if (!showOptionsMenu && IsKeyPressed(KEY_SPACE)) {
                paused = !paused;
            }

            if (!paused && !showOptionsMenu) {
                // Calculate render context once per frame (eliminates duplicate calculations)
                RenderContext ctx = CalculateRenderContext(screenWidth, screenHeight);

                gepard.turretIndex = GetTurretIndexFromMouse(ctx.mousePos.x, screenWidth);

                // Update gepard animation
                UpdateGepard(&gepard, deltaTime);

                // Update drones
                UpdateDrones(drones, deltaTime);

                // Update projectiles
                UpdateProjectiles(projectiles, drones, &ammo, &score, &shahedActive, deltaTime, explosionSound);

                // Spawn timer
                spawnTimer += deltaTime;

                // Check drone status (replaces duplicate logic)
                DroneStatus droneStatus = CheckDroneStatus(drones);

                // Update shahedActive status
                if (!droneStatus.shahedFound) {
                    shahedActive = false;
                }

                // Spawn new wave only if Shahed has been dealt with (hit or missed)
                if (!shahedActive && spawnTimer > RESPAWN_DELAY) {
                    GenerateNewEquation(&currentEquation, level, drones, allowNegativeResults);
                    SpawnDrones(drones, &currentEquation, &activeDroneCount);
                    shahedActive = true;
                    spawnTimer = 0.0f;
                }

                // Handle shooting
                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && !gepard.isFiring && ammo >= SHOT_COST) {
                    // Check if clicked on a drone
                    for (int i = 0; i < MAX_DRONES; i++) {
                        if (drones[i].active && drones[i].state == DRONE_FLYING) {
                            DroneBounds bounds = GetDroneBounds(drones[i]);
                            if (CheckCollisionPointRec(ctx.mousePos, bounds.bounds)) {
                                // Fire at drone
                                ammo -= SHOT_COST;
                                gepard.isFiring = true;
                                gepard.fireTimer = 0.0f;
                                gepard.fireFrame = 1; // Start at middle frame for immediate visual feedback
                                PlaySound(shootSound);
                                // Play explosion sound only if hitting the correct drone (Shahed)
                                if (drones[i].isShahed) {
                                    PlaySound(explosionSound);
                                }

                                // Spawn THREE projectiles from tank to drone (dual barrels + center)
                                Vector2 barrelPos1 = GetBarrelPosition(gepardPosition, true);
                                Vector2 barrelPos2 = GetBarrelPosition(gepardPosition, false);
                                Vector2 barrelPosCenter = {
                                    (barrelPos1.x + barrelPos2.x) / 2.0f,
                                    (barrelPos1.y + barrelPos2.y) / 2.0f
                                };

                                // Target center of drone with slight offset for triple barrels
                                Vector2 droneTarget1 = { bounds.center.x - DRONE_TARGET_OFFSET, bounds.center.y };
                                Vector2 droneTarget2 = { bounds.center.x + DRONE_TARGET_OFFSET, bounds.center.y };
                                Vector2 droneTarget3 = { bounds.center.x, bounds.center.y };
                                SpawnProjectile(projectiles, barrelPos1, droneTarget1, i);
                                SpawnProjectile(projectiles, barrelPos2, droneTarget2, i);
                                SpawnProjectile(projectiles, barrelPosCenter, droneTarget3, i);
                                break;
                            }
                        }
                    }
                }

                // Game over check
                if (ammo < SHOT_COST) {
                    // Reuse the drone status we already calculated
                    if (!droneStatus.canWin && droneStatus.aliveCount == 0) {
                        // Game over - restart
                        if (IsKeyPressed(KEY_R)) {
                            ammo = INITIAL_AMMO;
                            score = 0;
                            levelSelected = false;
                            gameStarted = false;
                            shahedActive = false;
                            paused = false;
                            spawnTimer = 0.0f;

                            // Clear all drones
                            for (int i = 0; i < MAX_DRONES; i++) {
                                drones[i].active = false;
                            }
                        }
                    }
                }
            }
        }
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------

        // Render game to texture at native resolution
        BeginTextureMode(target);

            ClearBackground(BLACK);

            if (!levelSelected && !showOptionsMenu) {
                // Level selection screen - using Setback font
                ClearBackground((Color){135, 206, 235, 255}); // Sky blue for menu

                // Measure and center title
                Vector2 titleSize = MeasureTextEx(setbackFont, GetText(STR_GAME_TITLE), setbackFont.baseSize * 3, SETBACK_SPACING);
                DrawTextEx(setbackFont, GetText(STR_GAME_TITLE), (Vector2){screenWidth/2 - titleSize.x/2, screenHeight/2 - 120}, setbackFont.baseSize * 3, SETBACK_SPACING, BLACK);

                Vector2 subtitleSize = MeasureTextEx(setbackFont, GetText(STR_GAME_SUBTITLE), setbackFont.baseSize * 2, SETBACK_SPACING);
                DrawTextEx(setbackFont, GetText(STR_GAME_SUBTITLE), (Vector2){screenWidth/2 - subtitleSize.x/2, screenHeight/2 - 60}, setbackFont.baseSize * 2, SETBACK_SPACING, DARKGRAY);

                Vector2 instructionsSize = MeasureTextEx(setbackFont, GetText(STR_GAME_INSTRUCTIONS), setbackFont.baseSize * 2, SETBACK_SPACING);
                DrawTextEx(setbackFont, GetText(STR_GAME_INSTRUCTIONS), (Vector2){screenWidth/2 - instructionsSize.x/2, screenHeight/2 - 30}, setbackFont.baseSize * 2, SETBACK_SPACING, DARKGRAY);

                Vector2 selectLevelSize = MeasureTextEx(setbackFont, GetText(STR_SELECT_LEVEL), setbackFont.baseSize * 2, SETBACK_SPACING);
                DrawTextEx(setbackFont, GetText(STR_SELECT_LEVEL), (Vector2){screenWidth/2 - selectLevelSize.x/2, screenHeight/2 + 20}, setbackFont.baseSize * 2, SETBACK_SPACING, BLACK);

                Vector2 level1Size = MeasureTextEx(setbackFont, GetText(STR_LEVEL_1_DESC), setbackFont.baseSize * 2, SETBACK_SPACING);
                DrawTextEx(setbackFont, GetText(STR_LEVEL_1_DESC), (Vector2){screenWidth/2 - level1Size.x/2, screenHeight/2 + 60}, setbackFont.baseSize * 2, SETBACK_SPACING, DARKGREEN);

                Vector2 level2Size = MeasureTextEx(setbackFont, GetText(STR_LEVEL_2_DESC), setbackFont.baseSize * 2, SETBACK_SPACING);
                DrawTextEx(setbackFont, GetText(STR_LEVEL_2_DESC), (Vector2){screenWidth/2 - level2Size.x/2, screenHeight/2 + 90}, setbackFont.baseSize * 2, SETBACK_SPACING, ORANGE);

                Vector2 level3Size = MeasureTextEx(setbackFont, GetText(STR_LEVEL_3_DESC), setbackFont.baseSize * 2, SETBACK_SPACING);
                DrawTextEx(setbackFont, GetText(STR_LEVEL_3_DESC), (Vector2){screenWidth/2 - level3Size.x/2, screenHeight/2 + 120}, setbackFont.baseSize * 2, SETBACK_SPACING, RED);

                // Options hint
                Vector2 optionsSize = MeasureTextEx(setbackFont, GetText(STR_PRESS_OPTIONS), setbackFont.baseSize * 2, SETBACK_SPACING);
                DrawTextEx(setbackFont, GetText(STR_PRESS_OPTIONS), (Vector2){screenWidth/2 - optionsSize.x/2, screenHeight/2 + 160}, setbackFont.baseSize * 2, SETBACK_SPACING, BLUE);

                // Draw language selection flags at the bottom
                const float flagSize = 60.0f;
                const float flagSpacing = 20.0f;
                const float flagY = screenHeight - 100.0f;
                Language currentLang = GetCurrentLanguage();

                // English flag (GB)
                Rectangle flagDestGB = {screenWidth/2 - flagSize - flagSpacing - flagSize/2, flagY, flagSize, flagSize * 0.6f};
                DrawTexturePro(flagGB,
                              (Rectangle){0, 0, (float)flagGB.width, (float)flagGB.height},
                              flagDestGB,
                              (Vector2){0, 0}, 0.0f, WHITE);
                if (currentLang == LANG_ENGLISH) {
                    DrawRectangleLinesEx(flagDestGB, 3, GREEN);
                } else {
                    DrawRectangleLinesEx(flagDestGB, 2, BLACK);
                }

                // Polish flag (PL)
                Rectangle flagDestPL = {screenWidth/2 - flagSize/2, flagY, flagSize, flagSize * 0.6f};
                DrawTexturePro(flagPL,
                              (Rectangle){0, 0, (float)flagPL.width, (float)flagPL.height},
                              flagDestPL,
                              (Vector2){0, 0}, 0.0f, WHITE);
                if (currentLang == LANG_POLISH) {
                    DrawRectangleLinesEx(flagDestPL, 3, GREEN);
                } else {
                    DrawRectangleLinesEx(flagDestPL, 2, BLACK);
                }

                // Ukrainian flag (UA)
                Rectangle flagDestUA = {screenWidth/2 + flagSpacing + flagSize/2, flagY, flagSize, flagSize * 0.6f};
                DrawTexturePro(flagUA,
                              (Rectangle){0, 0, (float)flagUA.width, (float)flagUA.height},
                              flagDestUA,
                              (Vector2){0, 0}, 0.0f, WHITE);
                if (currentLang == LANG_UKRAINIAN) {
                    DrawRectangleLinesEx(flagDestUA, 3, GREEN);
                } else {
                    DrawRectangleLinesEx(flagDestUA, 2, BLACK);
                }
            } else if (showOptionsMenu) {
                // Draw options menu on top of level selection
                ClearBackground((Color){135, 206, 235, 255}); // Sky blue background

                // Dark overlay
                DrawRectangle(0, 0, screenWidth, screenHeight, (Color){0, 0, 0, 180});

                // Menu title
                Vector2 optionsTitleSize = MeasureTextEx(mechaFont, GetText(STR_OPTIONS), mechaFont.baseSize * 4, MECHA_SPACING);
                DrawTextEx(mechaFont, GetText(STR_OPTIONS), (Vector2){screenWidth/2 - optionsTitleSize.x/2, screenHeight/2 - 150}, mechaFont.baseSize * 4, MECHA_SPACING, WHITE);

                // Option 1: Show Equation Breakdown
                DrawTextEx(setbackFont, GetText(STR_SHOW_BREAKDOWN), (Vector2){screenWidth/2 - 200, screenHeight/2 - 70}, setbackFont.baseSize * 2, SETBACK_SPACING, WHITE);

                // Checkbox 1
                Rectangle checkboxRect1 = {screenWidth/2 + 180, screenHeight/2 - 80, 30, 30};
                DrawRectangleRec(checkboxRect1, WHITE);
                DrawRectangleLinesEx(checkboxRect1, 2, BLACK);
                if (showEquationBreakdown) {
                    // Draw checkmark
                    DrawRectangle(checkboxRect1.x + 5, checkboxRect1.y + 5, 20, 20, GREEN);
                }

                // Option 2: Allow Negative Results
                DrawTextEx(setbackFont, GetText(STR_ALLOW_NEGATIVE), (Vector2){screenWidth/2 - 200, screenHeight/2 - 20}, setbackFont.baseSize * 2, SETBACK_SPACING, WHITE);

                // Checkbox 2
                Rectangle checkboxRect2 = {screenWidth/2 + 180, screenHeight/2 - 30, 30, 30};
                DrawRectangleRec(checkboxRect2, WHITE);
                DrawRectangleLinesEx(checkboxRect2, 2, BLACK);
                if (allowNegativeResults) {
                    // Draw checkmark
                    DrawRectangle(checkboxRect2.x + 5, checkboxRect2.y + 5, 20, 20, GREEN);
                }

                // Option 3: Music Volume
                DrawTextEx(setbackFont, GetText(STR_MUSIC_VOLUME), (Vector2){screenWidth/2 - 200, screenHeight/2 + 40}, setbackFont.baseSize * 2, SETBACK_SPACING, WHITE);

                // Slider
                Rectangle sliderBg = {screenWidth/2 - 100, screenHeight/2 + 50, 200, 20};
                DrawRectangleRec(sliderBg, DARKGRAY);
                DrawRectangleLinesEx(sliderBg, 2, WHITE);

                // Slider fill
                Rectangle sliderFill = {sliderBg.x, sliderBg.y, sliderBg.width * musicVolume, sliderBg.height};
                DrawRectangleRec(sliderFill, SKYBLUE);

                // Slider handle
                float handleX = sliderBg.x + (sliderBg.width * musicVolume);
                DrawCircle(handleX, sliderBg.y + sliderBg.height/2, 12, WHITE);
                DrawCircleLines(handleX, sliderBg.y + sliderBg.height/2, 12, BLACK);

                // Volume percentage
                char volumeText[16];
                sprintf(volumeText, "%d%%", (int)(musicVolume * 100));
                DrawTextEx(setbackFont, volumeText, (Vector2){screenWidth/2 + 120, screenHeight/2 + 45}, setbackFont.baseSize * 2, SETBACK_SPACING, WHITE);

                // Instructions
                Vector2 closeSize = MeasureTextEx(setbackFont, GetText(STR_CLOSE_OPTIONS), setbackFont.baseSize * 2, SETBACK_SPACING);
                DrawTextEx(setbackFont, GetText(STR_CLOSE_OPTIONS), (Vector2){screenWidth/2 - closeSize.x/2, screenHeight/2 + 100}, setbackFont.baseSize * 2, SETBACK_SPACING, LIGHTGRAY);
            } else if (gameStarted) {
                // Draw background
                DrawTexture(backgroundTexture, 0, 0, WHITE);

                // Draw equation - using Pixantiqua font
                char equationText[64];
                sprintf(equationText, "%d %c %d = ?",
                        currentEquation.num1, currentEquation.operation, currentEquation.num2);
                DrawTextEx(pixantiquaFont, equationText, (Vector2){20, 20}, pixantiquaFont.baseSize * 3, PIXANTIQUA_SPACING, BLACK);

                // Draw decomposed equation with color coding (if enabled)
                if (showEquationBreakdown) {
                    DrawDecomposedEquation(&currentEquation, pixantiquaFont, (Vector2){20, 60}, pixantiquaFont.baseSize * 2, PIXANTIQUA_SPACING, 0.0f);
                }

                // Draw score and level - using Mecha font
                char scoreText[64];
                sprintf(scoreText, GetText(STR_SCORE), score);
                DrawTextEx(mechaFont, scoreText, (Vector2){screenWidth - 180, 20}, mechaFont.baseSize * 2, MECHA_SPACING, BLACK);

                char levelText[64];
                sprintf(levelText, GetText(STR_LEVEL), level);
                DrawTextEx(mechaFont, levelText, (Vector2){screenWidth - 180, 60}, mechaFont.baseSize * 2, MECHA_SPACING, DARKBLUE);

                // Draw drone sprites
                for (int i = 0; i < MAX_DRONES; i++) {
                    if (drones[i].active && drones[i].state != DRONE_DEAD) {
                        DrawDrone(sahedTexture, drones[i]);
                    }
                }

                // Draw all numbers on top (so they're never hidden by other drones) - using Pixantiqua font
                if (!paused) {
                    for (int i = 0; i < MAX_DRONES; i++) {
                        if (drones[i].active && drones[i].state == DRONE_FLYING) {
                            char answerText[16];
                            sprintf(answerText, "%d", drones[i].answer);
                            Vector2 textSize = MeasureTextEx(pixantiquaFont, answerText, pixantiquaFont.baseSize * 3, PIXANTIQUA_SPACING);
                            Vector2 textPos = {drones[i].position.x + DRONE_TEXT_OFFSET_X - textSize.x/2,
                                               drones[i].position.y + DRONE_TEXT_OFFSET_Y};
                            // Draw red text
                            DrawTextEx(pixantiquaFont, answerText, textPos, pixantiquaFont.baseSize * 3, PIXANTIQUA_SPACING, RED);
                        }
                    }
                }

                // Draw Gepard tank
                DrawGepard(gepardTexture, gepard, gepardPosition);

                // Draw projectiles
                DrawProjectiles(projectiles);

                // Draw ammo
                DrawAmmo(ammo, screenWidth, screenHeight);

                // Draw pause message - using Mecha font
                if (paused && !showOptionsMenu) {
                    DrawRectangle(0, 0, screenWidth, screenHeight, (Color){0, 0, 0, 128});
                    Vector2 pausedSize = MeasureTextEx(mechaFont, GetText(STR_PAUSED), mechaFont.baseSize * 4, MECHA_SPACING);
                    DrawTextEx(mechaFont, GetText(STR_PAUSED), (Vector2){screenWidth/2 - pausedSize.x/2, screenHeight/2 - 40}, mechaFont.baseSize * 4, MECHA_SPACING, WHITE);
                    Vector2 resumeSize = MeasureTextEx(mechaFont, GetText(STR_PRESS_RESUME), mechaFont.baseSize * 2, MECHA_SPACING);
                    DrawTextEx(mechaFont, GetText(STR_PRESS_RESUME), (Vector2){screenWidth/2 - resumeSize.x/2, screenHeight/2 + 20}, mechaFont.baseSize * 2, MECHA_SPACING, WHITE);
                }

                // Draw options menu
                if (showOptionsMenu) {
                    // Dark overlay
                    DrawRectangle(0, 0, screenWidth, screenHeight, (Color){0, 0, 0, 180});

                    // Menu title
                    Vector2 optionsTitleSize2 = MeasureTextEx(mechaFont, GetText(STR_OPTIONS), mechaFont.baseSize * 4, MECHA_SPACING);
                    DrawTextEx(mechaFont, GetText(STR_OPTIONS), (Vector2){screenWidth/2 - optionsTitleSize2.x/2, screenHeight/2 - 150}, mechaFont.baseSize * 4, MECHA_SPACING, WHITE);

                    // Option 1: Show Equation Breakdown
                    DrawTextEx(setbackFont, GetText(STR_SHOW_BREAKDOWN), (Vector2){screenWidth/2 - 200, screenHeight/2 - 70}, setbackFont.baseSize * 2, SETBACK_SPACING, WHITE);

                    // Checkbox 1
                    Rectangle checkboxRect1 = {screenWidth/2 + 180, screenHeight/2 - 80, 30, 30};
                    DrawRectangleRec(checkboxRect1, WHITE);
                    DrawRectangleLinesEx(checkboxRect1, 2, BLACK);
                    if (showEquationBreakdown) {
                        // Draw checkmark
                        DrawRectangle(checkboxRect1.x + 5, checkboxRect1.y + 5, 20, 20, GREEN);
                    }

                    // Option 2: Allow Negative Results
                    DrawTextEx(setbackFont, GetText(STR_ALLOW_NEGATIVE), (Vector2){screenWidth/2 - 200, screenHeight/2 - 20}, setbackFont.baseSize * 2, SETBACK_SPACING, WHITE);

                    // Checkbox 2
                    Rectangle checkboxRect2 = {screenWidth/2 + 180, screenHeight/2 - 30, 30, 30};
                    DrawRectangleRec(checkboxRect2, WHITE);
                    DrawRectangleLinesEx(checkboxRect2, 2, BLACK);
                    if (allowNegativeResults) {
                        // Draw checkmark
                        DrawRectangle(checkboxRect2.x + 5, checkboxRect2.y + 5, 20, 20, GREEN);
                    }

                    // Option 3: Music Volume
                    DrawTextEx(setbackFont, GetText(STR_MUSIC_VOLUME), (Vector2){screenWidth/2 - 200, screenHeight/2 + 40}, setbackFont.baseSize * 2, SETBACK_SPACING, WHITE);

                    // Slider
                    Rectangle sliderBg = {screenWidth/2 - 100, screenHeight/2 + 50, 200, 20};
                    DrawRectangleRec(sliderBg, DARKGRAY);
                    DrawRectangleLinesEx(sliderBg, 2, WHITE);

                    // Slider fill
                    Rectangle sliderFill = {sliderBg.x, sliderBg.y, sliderBg.width * musicVolume, sliderBg.height};
                    DrawRectangleRec(sliderFill, SKYBLUE);

                    // Slider handle
                    float handleX = sliderBg.x + (sliderBg.width * musicVolume);
                    DrawCircle(handleX, sliderBg.y + sliderBg.height/2, 12, WHITE);
                    DrawCircleLines(handleX, sliderBg.y + sliderBg.height/2, 12, BLACK);

                    // Volume percentage
                    char volumeText[16];
                    sprintf(volumeText, "%d%%", (int)(musicVolume * 100));
                    DrawTextEx(setbackFont, volumeText, (Vector2){screenWidth/2 + 120, screenHeight/2 + 45}, setbackFont.baseSize * 2, SETBACK_SPACING, WHITE);

                    // Instructions
                    Vector2 closeSize2 = MeasureTextEx(setbackFont, GetText(STR_CLOSE_OPTIONS), setbackFont.baseSize * 2, SETBACK_SPACING);
                    DrawTextEx(setbackFont, GetText(STR_CLOSE_OPTIONS), (Vector2){screenWidth/2 - closeSize2.x/2, screenHeight/2 + 100}, setbackFont.baseSize * 2, SETBACK_SPACING, LIGHTGRAY);
                }

                // Draw game over message - using Mecha font
                if (ammo < SHOT_COST) {
                    Vector2 gameOverSize = MeasureTextEx(mechaFont, GetText(STR_OUT_OF_AMMO), mechaFont.baseSize * 2, MECHA_SPACING);
                    DrawTextEx(mechaFont, GetText(STR_OUT_OF_AMMO), (Vector2){screenWidth/2 - gameOverSize.x/2, screenHeight/2}, mechaFont.baseSize * 2, MECHA_SPACING, RED);
                }
            }

        EndTextureMode();

        // Now draw the scaled texture to the actual window
        BeginDrawing();
            ClearBackground(BLACK);

            // Calculate render context for final display
            RenderContext finalCtx = CalculateRenderContext(screenWidth, screenHeight);

            // Draw the render texture scaled and centered
            Rectangle sourceRec = { 0, 0, (float)target.texture.width, -(float)target.texture.height };
            Rectangle destRec = { finalCtx.offsetX, finalCtx.offsetY, finalCtx.drawWidth, finalCtx.drawHeight };
            DrawTexturePro(target.texture, sourceRec, destRec, (Vector2){0, 0}, 0.0f, WHITE);

        EndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    CleanupLocalization();
    UnloadRenderTexture(target);
    UnloadFont(mechaFont);
    UnloadFont(setbackFont);
    UnloadFont(romulusFont);
    UnloadFont(alphaBetaFont);
    UnloadFont(pixantiquaFont);
    UnloadTexture(sahedTexture);
    UnloadTexture(gepardTexture);
    UnloadTexture(backgroundTexture);
    UnloadTexture(flagGB);
    UnloadTexture(flagPL);
    UnloadTexture(flagUA);
    UnloadSound(shootSound);
    UnloadSound(explosionSound);
    CloseAudioDevice();
    CloseWindow();
    //--------------------------------------------------------------------------------------

    return 0;
}

//------------------------------------------------------------------------------------
// Function Definitions
//------------------------------------------------------------------------------------
void DecomposeNumber(int num, int *tens, int *ones) {
    if (num >= 0) {
        *tens = (num / 10) * 10;
        *ones = num % 10;
    } else {
        // For negative numbers, decompose the absolute value
        int absNum = -num;
        *tens = -((absNum / 10) * 10);
        *ones = -(absNum % 10);
    }
}

void CreateDecomposedEquation(MathEquation *eq) {
    eq->partCount = 0;

    int num1_tens, num1_ones;
    int num2_tens, num2_ones;

    DecomposeNumber(eq->num1, &num1_tens, &num1_ones);
    DecomposeNumber(eq->num2, &num2_tens, &num2_ones);

    switch(eq->operation) {
        case '+': {
            // For addition: highlight all tens in green
            // Example: 18 + 23 = 10 + 8 + 10 + 10 + 3
            int idx = 0;

            // First number decomposition
            if (num1_tens != 0) {
                eq->parts[idx].value = num1_tens;
                eq->parts[idx].operatorBefore = '\0';
                eq->parts[idx].visualState = PART_HIGHLIGHT; // Green for tens
                idx++;
            }
            if (num1_ones != 0) {
                eq->parts[idx].value = num1_ones;
                eq->parts[idx].operatorBefore = (num1_tens != 0) ? '+' : '\0';
                eq->parts[idx].visualState = PART_NORMAL;
                idx++;
            }

            // Second number decomposition
            if (num2_tens != 0) {
                eq->parts[idx].value = num2_tens;
                eq->parts[idx].operatorBefore = '+';
                eq->parts[idx].visualState = PART_HIGHLIGHT; // Green for tens
                idx++;
            }
            if (num2_ones != 0) {
                eq->parts[idx].value = num2_ones;
                eq->parts[idx].operatorBefore = '+';
                eq->parts[idx].visualState = PART_NORMAL;
                idx++;
            }

            eq->partCount = idx;
            break;
        }

        case '-': {
            // For subtraction: implement pairing logic
            // Example: 11 - 20 = 10 + 1 - 10 - 10
            //                    (red)    (red)
            int idx = 0;

            // Build lists of positive and negative tens
            int positiveTens[10] = {0};
            int negativeTens[10] = {0};
            int posTensCount = 0;
            int negTensCount = 0;

            // Add parts from num1 (all positive)
            if (num1_tens != 0) {
                int count = abs(num1_tens) / 10;
                for (int i = 0; i < count; i++) {
                    positiveTens[posTensCount++] = 10;
                }
            }

            // Add parts from num2 (all negative in subtraction)
            if (num2_tens != 0) {
                int count = abs(num2_tens) / 10;
                for (int i = 0; i < count; i++) {
                    negativeTens[negTensCount++] = 10;
                }
            }

            // Mark pairs for red highlighting (cancellation)
            bool positiveCancelled[10] = {false};
            bool negativeCancelled[10] = {false};

            int pairsToMake = (posTensCount < negTensCount) ? posTensCount : negTensCount;
            for (int i = 0; i < pairsToMake; i++) {
                positiveCancelled[i] = true;
                negativeCancelled[i] = true;
            }

            // Now build the parts array
            // First number parts
            if (num1_tens != 0) {
                int count = abs(num1_tens) / 10;
                for (int i = 0; i < count; i++) {
                    eq->parts[idx].value = 10;
                    eq->parts[idx].operatorBefore = (idx == 0) ? '\0' : '+';
                    eq->parts[idx].visualState = positiveCancelled[i] ? PART_CANCELLED : PART_NORMAL;
                    idx++;
                }
            }

            if (num1_ones != 0) {
                eq->parts[idx].value = num1_ones;
                eq->parts[idx].operatorBefore = (idx == 0) ? '\0' : '+';
                eq->parts[idx].visualState = PART_NORMAL;
                idx++;
            }

            // Second number parts (subtracted)
            if (num2_tens != 0) {
                int count = abs(num2_tens) / 10;
                for (int i = 0; i < count; i++) {
                    eq->parts[idx].value = 10;
                    eq->parts[idx].operatorBefore = '-';
                    eq->parts[idx].visualState = negativeCancelled[i] ? PART_CANCELLED : PART_NORMAL;
                    idx++;
                }
            }

            if (num2_ones != 0) {
                eq->parts[idx].value = abs(num2_ones);
                eq->parts[idx].operatorBefore = '-';
                eq->parts[idx].visualState = PART_NORMAL;
                idx++;
            }

            eq->partCount = idx;
            break;
        }

        case '*':
        case '/':
            // For multiplication and division, create a single "part" with the full equation
            eq->parts[0].value = eq->num1;
            eq->parts[0].operatorBefore = '\0';
            eq->parts[0].visualState = PART_NORMAL;
            eq->partCount = 1;
            break;
    }

    // Also build the old string version for compatibility
    char buffer[128] = "";
    for (int i = 0; i < eq->partCount; i++) {
        if (eq->parts[i].operatorBefore != '\0') {
            sprintf(buffer + strlen(buffer), " %c ", eq->parts[i].operatorBefore);
        }
        sprintf(buffer + strlen(buffer), "%d", eq->parts[i].value);
    }

    if (eq->operation == '*') {
        sprintf(buffer, "%d * %d", eq->num1, eq->num2);
    } else if (eq->operation == '/') {
        sprintf(buffer, "%d / %d", eq->num1, eq->num2);
    }

    sprintf(buffer + strlen(buffer), " = ?");
    strcpy(eq->decomposed, buffer);
}

void GenerateNewEquation(MathEquation *eq, int level, Drone drones[], bool allowNegative) {
    int opType;
    int attempts = 0;
    bool isTrivial;
    bool duplicateAnswer;

    do {
        isTrivial = false;
        duplicateAnswer = false;

        // Determine available operations based on level
        if (level == 1) {
            opType = rand() % 2; // 0: add, 1: subtract
        } else if (level == 2) {
            opType = rand() % 3; // 0: add, 1: subtract, 2: multiply
        } else {
            opType = rand() % 4; // 0: add, 1: subtract, 2: multiply, 3: divide
        }

        switch(opType) {
            case 0: // Addition
                if (level == 1) {
                    eq->num1 = 1 + rand() % 20; // 1-20 (avoid 0)
                    eq->num2 = 1 + rand() % 20; // 1-20 (avoid 0)
                } else {
                    eq->num1 = 5 + rand() % 45;
                    eq->num2 = 5 + rand() % 45;
                }
                eq->operation = '+';
                eq->correctAnswer = eq->num1 + eq->num2;

                // Check for trivial cases: 0+X or X+0
                if (eq->num1 == 0 || eq->num2 == 0) {
                    isTrivial = true;
                }
                break;

            case 1: // Subtraction
                if (allowNegative) {
                    // Allow negative results
                    if (level == 1) {
                        eq->num1 = rand() % 21; // 0-20
                        eq->num2 = rand() % 21; // 0-20
                    } else {
                        eq->num1 = rand() % 80; // 0-79
                        eq->num2 = rand() % 80; // 0-79
                    }
                } else {
                    // Only positive results
                    if (level == 1) {
                        eq->num1 = rand() % 21; // 0-20
                        eq->num2 = rand() % (eq->num1 + 1); // Ensure result >= 0
                    } else {
                        eq->num1 = 20 + rand() % 60;
                        eq->num2 = 5 + rand() % (eq->num1 - 4);
                    }
                }
                eq->operation = '-';
                eq->correctAnswer = eq->num1 - eq->num2;

                // Check for trivial case: X-0
                if (eq->num2 == 0) {
                    isTrivial = true;
                }
                break;

            case 2: // Multiplication
                eq->num1 = 2 + rand() % 12;
                eq->num2 = 2 + rand() % 12;
                eq->operation = '*';
                eq->correctAnswer = eq->num1 * eq->num2;
                break;

            case 3: // Division
                // Generate answer first, then calculate dividend to ensure whole number result
                eq->correctAnswer = 2 + rand() % 10; // Answer: 2-11
                eq->num2 = 2 + rand() % 9; // Divisor: 2-10
                eq->num1 = eq->correctAnswer * eq->num2; // Dividend
                eq->operation = '/';
                break;
        }

        // Check if the correct answer already exists on any active drone
        for (int i = 0; i < MAX_DRONES; i++) {
            if (drones[i].active && drones[i].state == DRONE_FLYING) {
                if (drones[i].answer == eq->correctAnswer) {
                    duplicateAnswer = true;
                    break;
                }
            }
        }

        attempts++;
        // Prevent infinite loop, after 20 attempts just accept what we have
        if (attempts > 20) break;

    } while (isTrivial || duplicateAnswer);

    // Create decomposed version of the equation for children
    CreateDecomposedEquation(eq);
}

void SpawnDrones(Drone drones[], MathEquation *eq, int *activeDroneCount) {
    int numDrones = DRONE_MIN_COUNT + rand() % (DRONE_MAX_COUNT - DRONE_MIN_COUNT + 1);
    if (numDrones > MAX_DRONES) numDrones = MAX_DRONES;

    // Check existing drones and mark any that match the new correct answer as Shahed
    int existingAnswers[MAX_DRONES];
    int existingCount = 0;
    bool foundExistingShahed = false;
    for (int i = 0; i < MAX_DRONES; i++) {
        if (drones[i].active && drones[i].state == DRONE_FLYING) {
            existingAnswers[existingCount++] = drones[i].answer;
            // If this drone has the correct answer for the new equation, mark it as Shahed
            if (drones[i].answer == eq->correctAnswer) {
                drones[i].isShahed = true;
                foundExistingShahed = true;
            } else {
                // Make sure old drones aren't marked as Shahed anymore
                drones[i].isShahed = false;
            }
        }
    }

    // Generate answers, avoiding duplicates with existing drones
    int answers[MAX_DRONES];
    // If we found an existing drone with correct answer, don't spawn another one with same answer
    int correctIndex = foundExistingShahed ? -1 : rand() % numDrones;

    for (int i = 0; i < numDrones; i++) {
        if (i == correctIndex) {
            answers[i] = eq->correctAnswer;
        } else {
            // Generate wrong answer that doesn't duplicate existing or new answers
            bool duplicate;
            int attempts = 0;
            do {
                duplicate = false;
                int offset = (rand() % 20) - 10;
                if (offset == 0) offset = 5;
                answers[i] = eq->correctAnswer + offset;

                // Check against existing answers on screen
                for (int j = 0; j < existingCount; j++) {
                    if (answers[i] == existingAnswers[j]) {
                        duplicate = true;
                        break;
                    }
                }

                // Check against already generated answers in this wave
                if (!duplicate) {
                    for (int j = 0; j < i; j++) {
                        if (answers[i] == answers[j]) {
                            duplicate = true;
                            break;
                        }
                    }
                }

                attempts++;
                if (attempts > 50) break; // Prevent infinite loop
            } while (duplicate && attempts < 50);
        }
    }

    // Find free slots and spawn drones
    int spawned = 0;
    for (int i = 0; i < MAX_DRONES && spawned < numDrones; i++) {
        if (!drones[i].active || drones[i].state == DRONE_DEAD) {
            drones[i].position.x = DRONE_SPAWN_X + spawned * DRONE_SPAWN_SPACING;
            drones[i].position.y = DRONE_SPAWN_Y_MIN + rand() % (int)DRONE_SPAWN_Y_RANGE;
            drones[i].answer = answers[spawned];
            drones[i].isShahed = (spawned == correctIndex);
            drones[i].state = DRONE_FLYING;
            drones[i].animTimer = 0.0f;
            drones[i].active = true;
            spawned++;
        }
    }

    *activeDroneCount = spawned;
}

void UpdateDrones(Drone drones[], float deltaTime) {
    for (int i = 0; i < MAX_DRONES; i++) {
        if (!drones[i].active) continue;

        switch(drones[i].state) {
            case DRONE_FLYING:
                drones[i].position.x -= DRONE_SPEED * deltaTime;
                // If Shahed reaches left side, make it fall down
                if (drones[i].isShahed && drones[i].position.x < DRONE_LEFT_BOUNDARY) {
                    drones[i].state = DRONE_FALLING;
                    drones[i].animTimer = 0.0f;
                }
                // Non-Shahed drones just disappear off screen
                else if (drones[i].position.x < OFF_SCREEN_LEFT) {
                    drones[i].active = false;
                }
                break;

            case DRONE_EXPLODING:
                drones[i].animTimer += deltaTime;
                if (drones[i].animTimer > EXPLOSION_DURATION) {
                    drones[i].state = DRONE_DEAD;
                }
                break;

            case DRONE_FALLING:
                drones[i].animTimer += deltaTime;
                drones[i].position.x -= DRONE_SPEED * DRONE_FALL_HORIZONTAL_MULTIPLIER * deltaTime;
                drones[i].position.y += DRONE_FALL_SPEED * deltaTime;

                // If Shahed hits ground, explode
                if (drones[i].isShahed && drones[i].position.y >= GROUND_LEVEL) {
                    drones[i].state = DRONE_EXPLODING;
                    drones[i].animTimer = 0.0f;
                    drones[i].position.y += GROUND_EXPLOSION_OFFSET;
                }
                // Non-Shahed drones just disappear when hitting ground or going off screen
                else if (drones[i].position.y >= NEAR_GROUND_LEVEL || drones[i].position.x < OFF_SCREEN_LEFT) {
                    drones[i].state = DRONE_DEAD;
                }
                break;

            case DRONE_DEAD:
                drones[i].active = false;
                break;
        }
    }
}

void UpdateGepard(GepardTank *gepard, float deltaTime) {
    if (gepard->isFiring) {
        gepard->fireTimer += deltaTime;

        if (gepard->fireTimer > FIRE_FRAME_DURATION) {
            gepard->fireFrame++;
            gepard->fireTimer = 0.0f;

            if (gepard->fireFrame > 2) { // 0=normal, 1=middle, 2=top, then back to 0
                gepard->fireFrame = 0;
                gepard->isFiring = false;
            }
        }
    }
}

int GetTurretIndexFromMouse(int mouseX, int screenWidth) {
    // Map mouse X position to turret index (0-4)
    // Right edge (screenWidth) -> index 0
    // Left edge (0) -> index 4
    float ratio = (float)mouseX / (float)screenWidth;
    int index = (int)((1.0f - ratio) * 5);
    if (index < 0) index = 0;
    if (index > 4) index = 4;
    return index;
}

void DrawDrone(Texture2D texture, Drone drone) {
    Rectangle sourceRec;

    switch(drone.state) {
        case DRONE_FLYING:
            sourceRec = (Rectangle){ 0, 0, 100, 100 };
            break;
        case DRONE_EXPLODING:
            sourceRec = (Rectangle){ 100, 0, 100, 100 };
            break;
        case DRONE_FALLING:
            // Shahed uses attacking sprite (4th cell), others use damaged sprite (3rd cell)
            if (drone.isShahed) {
                sourceRec = (Rectangle){ 300, 0, 100, 100 };
            } else {
                sourceRec = (Rectangle){ 200, 0, 100, 100 };
            }
            break;
        case DRONE_DEAD:
            sourceRec = (Rectangle){ 200, 0, 100, 100 };
            break;
    }

    // Blink effect for falling drones near ground
    if (drone.state == DRONE_FALLING && drone.position.y >= NEAR_GROUND_LEVEL) {
        int blinkCycle = (int)(drone.animTimer * BLINK_FREQUENCY) % 2;
        if (blinkCycle == 0) {
            return; // Don't draw (creates blink effect)
        }
    }

    // Calculate scale based on drone state
    float scale = DRONE_SCALE;

    if (drone.state == DRONE_FALLING && drone.isShahed) {
        // For falling Shahed, shrink from DRONE_SCALE to DRONE_MIN_SCALE as it falls
        float progress = (drone.position.y - DRONE_FALL_START_Y) / (DRONE_FALL_END_Y - DRONE_FALL_START_Y);
        if (progress < 0.0f) progress = 0.0f;
        if (progress > 1.0f) progress = 1.0f;

        // Interpolate from DRONE_SCALE to DRONE_MIN_SCALE
        scale = DRONE_SCALE - (progress * (DRONE_SCALE - DRONE_MIN_SCALE));
    } else if (drone.state == DRONE_EXPLODING && drone.isShahed && drone.position.y >= GROUND_LEVEL - 50.0f) {
        // If Shahed exploded near ground, keep it at minimum scale
        scale = DRONE_MIN_SCALE;
    }

    float drawSize = DRONE_TEXTURE_SIZE * scale;
    Rectangle destRec = (Rectangle){ drone.position.x, drone.position.y, drawSize, drawSize };
    DrawTexturePro(texture, sourceRec, destRec, (Vector2){0, 0}, 0.0f, WHITE);
}

void DrawGepard(Texture2D texture, GepardTank gepard, Vector2 position) {
    Rectangle sourceRec;

    // Determine which cell to draw
    int row = gepard.isFiring ? gepard.fireFrame : 0; // 0 = bottom, 1 = middle, 2 = top
    int col = gepard.turretIndex; // 0-4

    sourceRec.x = col * GEPARD_TEXTURE_SIZE;
    sourceRec.y = (2 - row) * GEPARD_TEXTURE_SIZE; // Flip because bottom row is at y=300
    sourceRec.width = GEPARD_TEXTURE_SIZE;
    sourceRec.height = GEPARD_TEXTURE_SIZE;

    // Scale tank using global GEPARD_SCALE constant
    float scaledSize = GEPARD_TEXTURE_SIZE * GEPARD_SCALE;
    Rectangle destRec = (Rectangle){ position.x, position.y, scaledSize, scaledSize };
    DrawTexturePro(texture, sourceRec, destRec, (Vector2){0, 0}, 0.0f, WHITE);
}

void DrawAmmo(int ammo, int screenWidth, int screenHeight) {
    int startX = screenWidth - AMMO_DISPLAY_OFFSET_X;
    int startY = screenHeight - AMMO_DISPLAY_OFFSET_Y;

    for (int i = 0; i < ammo; i++) {
        int x = startX - (i % AMMO_WARNING_THRESHOLD) * (AMMO_BOX_WIDTH + AMMO_BOX_SPACING);
        int y = startY - (i / AMMO_WARNING_THRESHOLD) * (AMMO_BOX_HEIGHT + AMMO_BOX_SPACING);

        Color ammoColor = (ammo > AMMO_WARNING_THRESHOLD) ? DARKGREEN :
                         (ammo > AMMO_CRITICAL_THRESHOLD) ? ORANGE : RED;
        DrawRectangle(x, y, AMMO_BOX_WIDTH, AMMO_BOX_HEIGHT, ammoColor);
    }
}

void SpawnProjectile(Projectile projectiles[], Vector2 start, Vector2 target, int droneIndex) {
    // Find free slot
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        if (!projectiles[i].active) {
            projectiles[i].position = start;
            projectiles[i].active = true;
            projectiles[i].lifetime = 0.0f;
            projectiles[i].targetDroneIndex = droneIndex;

            // Calculate velocity toward target
            Vector2 direction = { target.x - start.x, target.y - start.y };
            float length = sqrtf(direction.x * direction.x + direction.y * direction.y);
            if (length > 0) {
                projectiles[i].velocity.x = (direction.x / length) * PROJECTILE_SPEED;
                projectiles[i].velocity.y = (direction.y / length) * PROJECTILE_SPEED;
            }
            break;
        }
    }
}

void UpdateProjectiles(Projectile projectiles[], Drone drones[], int *ammo, int *score, bool *shahedActive, float deltaTime, Sound explosionSound) {
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        if (projectiles[i].active) {
            projectiles[i].position.x += projectiles[i].velocity.x * deltaTime;
            projectiles[i].position.y += projectiles[i].velocity.y * deltaTime;
            projectiles[i].lifetime += deltaTime;

            // Check collision with target drone
            int targetIdx = projectiles[i].targetDroneIndex;
            if (targetIdx >= 0 && targetIdx < MAX_DRONES &&
                drones[targetIdx].active &&
                (drones[targetIdx].state == DRONE_FLYING || drones[targetIdx].state == DRONE_EXPLODING)) {

                DroneBounds bounds = GetDroneBounds(drones[targetIdx]);

                float dx = projectiles[i].position.x - bounds.center.x;
                float dy = projectiles[i].position.y - bounds.center.y;
                float distance = sqrtf(dx * dx + dy * dy);

                // Hit detection using constant
                if (distance < (bounds.width * PROJECTILE_HIT_RADIUS)) {
                    projectiles[i].active = false;

                    // Only apply damage effects if still flying (not already hit)
                    if (drones[targetIdx].state == DRONE_FLYING) {
                        if (drones[targetIdx].isShahed) {
                            // Correct hit!
                            drones[targetIdx].state = DRONE_EXPLODING;
                            drones[targetIdx].animTimer = 0.0f;
                            *ammo += HIT_REWARD;
                            // Cap ammo at maximum
                            if (*ammo > MAX_AMMO) {
                                *ammo = MAX_AMMO;
                            }
                            *score += SCORE_CORRECT_HIT;
                            *shahedActive = false; // Shahed destroyed, can generate new equation
                        } else {
                            // Wrong hit
                            drones[targetIdx].state = DRONE_FALLING;
                            drones[targetIdx].animTimer = 0.0f;
                            *score += SCORE_WRONG_HIT; // Note: SCORE_WRONG_HIT is -5
                        }
                    }
                }
            }

            // Deactivate if off-screen or lived too long
            if (projectiles[i].position.x < OFF_SCREEN_TOP || projectiles[i].position.x > OFF_SCREEN_RIGHT ||
                projectiles[i].position.y < OFF_SCREEN_TOP || projectiles[i].position.y > OFF_SCREEN_BOTTOM ||
                projectiles[i].lifetime > PROJECTILE_MAX_LIFETIME) {
                projectiles[i].active = false;
            }
        }
    }
}

void DrawProjectiles(Projectile projectiles[]) {
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        if (projectiles[i].active) {
            // Draw as a bright yellow/orange tracer
            Vector2 end = {
                projectiles[i].position.x - projectiles[i].velocity.x * PROJECTILE_TRAIL_LENGTH,
                projectiles[i].position.y - projectiles[i].velocity.y * PROJECTILE_TRAIL_LENGTH
            };
            DrawLineEx(projectiles[i].position, end, PROJECTILE_LINE_THICKNESS, YELLOW);
            DrawCircleV(projectiles[i].position, PROJECTILE_DOT_RADIUS, ORANGE);
        }
    }
}

void DrawDecomposedEquation(MathEquation *eq, Font font, Vector2 position, float fontSize, float spacing, float blinkTimer) {
    Vector2 currentPos = position;

    for (int i = 0; i < eq->partCount; i++) {
        DecomposedPart *part = &eq->parts[i];
        char partText[32];

        // Draw operator if not the first element
        if (part->operatorBefore != '\0') {
            char opText[4];
            sprintf(opText, " %c ", part->operatorBefore);
            DrawTextEx(font, opText, currentPos, fontSize, spacing, BLUE);
            Vector2 opSize = MeasureTextEx(font, opText, fontSize, spacing);
            currentPos.x += opSize.x;
        }

        // Draw the number with appropriate color
        sprintf(partText, "%d", part->value);

        Color textColor = BLUE; // Default color

        switch (part->visualState) {
            case PART_NORMAL:
                textColor = BLUE;
                break;

            case PART_HIGHLIGHT:
                // Green for tens in addition
                textColor = (Color){0, 150, 0, 255}; // Dark green
                break;

            case PART_CANCELLED:
                // Red for cancelled pairs in subtraction
                textColor = RED;
                break;
        }

        DrawTextEx(font, partText, currentPos, fontSize, spacing, textColor);
        Vector2 partSize = MeasureTextEx(font, partText, fontSize, spacing);
        currentPos.x += partSize.x;
    }

    // Draw " = ?" at the end
    char endText[] = " = ?";
    DrawTextEx(font, endText, currentPos, fontSize, spacing, BLUE);
}

//------------------------------------------------------------------------------------
// Helper Function Implementations
//------------------------------------------------------------------------------------

RenderContext CalculateRenderContext(int screenWidth, int screenHeight) {
    RenderContext ctx = {0};

    int windowWidth = GetScreenWidth();
    int windowHeight = GetScreenHeight();

    float scaleX = (float)windowWidth / (float)screenWidth;
    float scaleY = (float)windowHeight / (float)screenHeight;
    ctx.scale = (scaleX < scaleY) ? scaleX : scaleY;

    ctx.drawWidth = screenWidth * ctx.scale;
    ctx.drawHeight = screenHeight * ctx.scale;
    ctx.offsetX = (windowWidth - ctx.drawWidth) / 2.0f;
    ctx.offsetY = (windowHeight - ctx.drawHeight) / 2.0f;

    // Transform mouse position once per frame
    Vector2 rawMousePos = GetMousePosition();
    ctx.mousePos.x = (rawMousePos.x - ctx.offsetX) / ctx.scale;
    ctx.mousePos.y = (rawMousePos.y - ctx.offsetY) / ctx.scale;

    return ctx;
}

DroneBounds GetDroneBounds(Drone drone) {
    DroneBounds bounds;
    bounds.width = DRONE_TEXTURE_SIZE * DRONE_SCALE;
    bounds.height = DRONE_TEXTURE_SIZE * DRONE_SCALE;
    bounds.center.x = drone.position.x + bounds.width / 2.0f;
    bounds.center.y = drone.position.y + bounds.height / 2.0f;
    bounds.bounds = (Rectangle){ drone.position.x, drone.position.y, bounds.width, bounds.height };
    return bounds;
}

DroneStatus CheckDroneStatus(Drone drones[]) {
    DroneStatus status = {false, false, 0};
    for (int i = 0; i < MAX_DRONES; i++) {
        if (drones[i].active && drones[i].state != DRONE_DEAD) {
            status.aliveCount++;
            if (drones[i].isShahed && drones[i].state == DRONE_FLYING) {
                status.shahedFound = true;
                status.canWin = true;
            }
        }
    }
    return status;
}

Vector2 GetBarrelPosition(Vector2 gepardPos, bool isLeftBarrel) {
    float barrelX = isLeftBarrel ? GEPARD_BARREL_LEFT_X : GEPARD_BARREL_RIGHT_X;
    return (Vector2){
        gepardPos.x + (GEPARD_TEXTURE_SIZE * GEPARD_SCALE * barrelX),
        gepardPos.y + (GEPARD_TEXTURE_SIZE * GEPARD_SCALE * GEPARD_BARREL_Y)
    };
}
