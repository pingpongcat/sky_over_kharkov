#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <string.h>

//------------------------------------------------------------------------------------
// Constants
//------------------------------------------------------------------------------------
#define MAX_DRONES 15
#define MAX_PROJECTILES 10
#define DRONE_SPEED 50.0f
#define PROJECTILE_SPEED 3000.0f
#define SHOT_COST 2
#define HIT_REWARD 3
#define INITIAL_AMMO 20

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

typedef struct {
    int num1;
    int num2;
    char operation;
    int correctAnswer;
} MathEquation;

typedef struct {
    Vector2 position;
    Vector2 velocity;
    bool active;
    float lifetime;
    int targetDroneIndex;
} Projectile;

//------------------------------------------------------------------------------------
// Function Declarations
//------------------------------------------------------------------------------------
void GenerateNewEquation(MathEquation *eq, int level);
void SpawnDrones(Drone drones[], MathEquation *eq, int *activeDroneCount);
void UpdateDrones(Drone drones[], float deltaTime);
void UpdateGepard(GepardTank *gepard, float deltaTime);
void UpdateProjectiles(Projectile projectiles[], Drone drones[], int *ammo, int *score, bool *shahedActive, float deltaTime);
void SpawnProjectile(Projectile projectiles[], Vector2 start, Vector2 target, int droneIndex);
int GetTurretIndexFromMouse(int mouseX, int screenWidth);
void DrawDrone(Texture2D texture, Drone drone);
void DrawGepard(Texture2D texture, GepardTank gepard, Vector2 position);
void DrawAmmo(int ammo, int screenWidth, int screenHeight, Font font);
void DrawProjectiles(Projectile projectiles[]);

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void)
{
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 1107;
    const int screenHeight = 694;

    InitWindow(screenWidth, screenHeight, "Sky Over Kharkiv");
    SetTargetFPS(60);

    // Seed random
    srand(time(NULL));

    // Load custom font - try system Noto Sans first, then local font folder
    Font customFont = LoadFontEx("/usr/share/fonts/google-noto-vf/NotoSans[wght].ttf", 96, 0, 0);
    if (customFont.texture.id == 0) {
        printf("System font not found, trying local font folder...\n");
        customFont = LoadFontEx("font/NotoSansSyriacWestern-Regular.ttf", 96, 0, 0);
    }
    if (customFont.texture.id == 0) {
        printf("ERROR: Failed to load custom font! Using default.\n");
        customFont = GetFontDefault();
    } else {
        printf("Custom font loaded successfully!\n");
        SetTextureFilter(customFont.texture, TEXTURE_FILTER_BILINEAR);
    }

    // Load textures
    Texture2D sahedTexture = LoadTexture("images/sahed.png");
    Texture2D gepardTexture = LoadTexture("images/gepard.png");
    Texture2D backgroundTexture = LoadTexture("images/background.png");

    // Game variables
    GepardTank gepard = { 0, 0.0f, false, 0 };
    Vector2 gepardPosition = { 120.0f, (float)screenHeight - 190.0f };

    Drone drones[MAX_DRONES] = {0};
    int activeDroneCount = 0;

    Projectile projectiles[MAX_PROJECTILES] = {0};

    MathEquation currentEquation = {0};
    int ammo = INITIAL_AMMO;
    int score = 0;
    int level = 1;
    bool shahedActive = false; // Track if Shahed from current equation is still active

    float spawnTimer = 0.0f;
    float spawnInterval = 3.0f;

    bool levelSelected = false;
    bool gameStarted = false;
    bool paused = false;

    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!WindowShouldClose())
    {
        float deltaTime = GetFrameTime();

        // Update
        //----------------------------------------------------------------------------------
        if (!levelSelected) {
            // Level selection screen
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
                GenerateNewEquation(&currentEquation, level);
                SpawnDrones(drones, &currentEquation, &activeDroneCount);
                shahedActive = true;
            }
        } else if (gameStarted) {
            // Toggle pause
            if (IsKeyPressed(KEY_SPACE)) {
                paused = !paused;
            }

            if (!paused) {
                // Update turret position based on mouse
                Vector2 mousePos = GetMousePosition();
                gepard.turretIndex = GetTurretIndexFromMouse(mousePos.x, screenWidth);

                // Update gepard animation
                UpdateGepard(&gepard, deltaTime);

                // Update drones
                UpdateDrones(drones, deltaTime);

                // Update projectiles
                UpdateProjectiles(projectiles, drones, &ammo, &score, &shahedActive, deltaTime);

                // Spawn timer
                spawnTimer += deltaTime;

                // Check if Shahed is still active
                bool shahedFound = false;
                int aliveCount = 0;
                for (int i = 0; i < MAX_DRONES; i++) {
                    if (drones[i].active && drones[i].state != DRONE_DEAD) {
                        aliveCount++;
                        if (drones[i].isShahed && drones[i].state == DRONE_FLYING) {
                            shahedFound = true;
                        }
                    }
                }

                // Update shahedActive status
                if (!shahedFound) {
                    shahedActive = false;
                }

                // Spawn new wave only if Shahed has been dealt with (hit or missed)
                if (!shahedActive && spawnTimer > 1.0f) {
                    GenerateNewEquation(&currentEquation, level);
                    SpawnDrones(drones, &currentEquation, &activeDroneCount);
                    shahedActive = true;
                    spawnTimer = 0.0f;
                }

                // Handle shooting
                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && !gepard.isFiring && ammo >= SHOT_COST) {
                    // Check if clicked on a drone
                    for (int i = 0; i < MAX_DRONES; i++) {
                        if (drones[i].active && drones[i].state == DRONE_FLYING) {
                            Rectangle droneRect = { drones[i].position.x, drones[i].position.y, 100, 100 };
                            if (CheckCollisionPointRec(mousePos, droneRect)) {
                                // Fire at drone
                                ammo -= SHOT_COST;
                                gepard.isFiring = true;
                                gepard.fireTimer = 0.0f;
                                gepard.fireFrame = 1; // Start at middle frame for immediate visual feedback

                                // Spawn projectile from tank to drone
                                Vector2 barrelPos = { gepardPosition.x + 75, gepardPosition.y + 30 };
                                Vector2 droneCenter = { drones[i].position.x + 50, drones[i].position.y + 50 };
                                SpawnProjectile(projectiles, barrelPos, droneCenter, i);
                                break;
                            }
                        }
                    }
                }

                // Game over check
                if (ammo < SHOT_COST) {
                    // Check if can still win with remaining drones
                    bool canWin = false;
                    int aliveCount = 0;
                    for (int i = 0; i < MAX_DRONES; i++) {
                        if (drones[i].active && drones[i].state != DRONE_DEAD) {
                            aliveCount++;
                        }
                        if (drones[i].active && drones[i].state == DRONE_FLYING && drones[i].isShahed) {
                            canWin = true;
                        }
                    }
                    if (!canWin && aliveCount == 0) {
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
        BeginDrawing();

            ClearBackground(BLACK);

            if (!levelSelected) {
                // Level selection screen
                ClearBackground((Color){135, 206, 235, 255}); // Sky blue for menu
                DrawTextEx(customFont, "SKY OVER KHARKIV", (Vector2){screenWidth/2 - 220, screenHeight/2 - 120}, 50, 2, BLACK);
                DrawTextEx(customFont, "Defend against Shahed drones!", (Vector2){screenWidth/2 - 200, screenHeight/2 - 60}, 30, 1, DARKGRAY);
                DrawTextEx(customFont, "Solve the equation to identify the Shahed", (Vector2){screenWidth/2 - 250, screenHeight/2 - 30}, 30, 1, DARKGRAY);

                DrawTextEx(customFont, "SELECT LEVEL:", (Vector2){screenWidth/2 - 110, screenHeight/2 + 20}, 35, 1.5, BLACK);
                DrawTextEx(customFont, "Press 1: Easy (Addition & Subtraction, 0-20)", (Vector2){screenWidth/2 - 270, screenHeight/2 + 60}, 30, 1, DARKGREEN);
                DrawTextEx(customFont, "Press 2: Medium (+ Multiplication)", (Vector2){screenWidth/2 - 210, screenHeight/2 + 90}, 30, 1, ORANGE);
                DrawTextEx(customFont, "Press 3: Hard (+ Division)", (Vector2){screenWidth/2 - 160, screenHeight/2 + 120}, 30, 1, RED);
            } else if (gameStarted) {
                // Draw background
                DrawTexture(backgroundTexture, 0, 0, WHITE);

                // Draw equation
                char equationText[64];
                sprintf(equationText, "Equation: %d %c %d = ?",
                        currentEquation.num1, currentEquation.operation, currentEquation.num2);
                DrawTextEx(customFont, equationText, (Vector2){20, 20}, 40, 1, BLACK);

                // Draw score and level
                char scoreText[32];
                sprintf(scoreText, "Score: %d", score);
                DrawTextEx(customFont, scoreText, (Vector2){screenWidth - 180, 20}, 35, 1, BLACK);

                char levelText[32];
                sprintf(levelText, "Level: %d", level);
                DrawTextEx(customFont, levelText, (Vector2){screenWidth - 180, 60}, 35, 1, DARKBLUE);

                // Draw drone sprites
                for (int i = 0; i < MAX_DRONES; i++) {
                    if (drones[i].active && drones[i].state != DRONE_DEAD) {
                        DrawDrone(sahedTexture, drones[i]);
                    }
                }

                // Draw all numbers on top (so they're never hidden by other drones)
                if (!paused) {
                    for (int i = 0; i < MAX_DRONES; i++) {
                        if (drones[i].active && drones[i].state == DRONE_FLYING) {
                            char answerText[16];
                            sprintf(answerText, "%d", drones[i].answer);
                            Vector2 textSize = MeasureTextEx(customFont, answerText, 30, 1);
                            DrawTextEx(customFont, answerText, (Vector2){drones[i].position.x + 50 - textSize.x/2,
                                   drones[i].position.y - 30}, 30, 1, RED);
                        }
                    }
                }

                // Draw Gepard tank
                DrawGepard(gepardTexture, gepard, gepardPosition);

                // Draw projectiles
                DrawProjectiles(projectiles);

                // Draw ammo
                DrawAmmo(ammo, screenWidth, screenHeight, customFont);

                // Draw pause message
                if (paused) {
                    DrawRectangle(0, 0, screenWidth, screenHeight, (Color){0, 0, 0, 128});
                    DrawTextEx(customFont, "PAUSED", (Vector2){screenWidth/2 - 100, screenHeight/2 - 40}, 60, 1, WHITE);
                    DrawTextEx(customFont, "Press SPACE to Resume", (Vector2){screenWidth/2 - 150, screenHeight/2 + 20}, 30, 1, WHITE);
                }

                // Draw game over message
                if (ammo < SHOT_COST) {
                    DrawTextEx(customFont, "OUT OF AMMO! Press R to Restart", (Vector2){screenWidth/2 - 250, screenHeight/2}, 35, 1, RED);
                }
            }

        EndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    UnloadFont(customFont);
    UnloadTexture(sahedTexture);
    UnloadTexture(gepardTexture);
    UnloadTexture(backgroundTexture);
    CloseWindow();
    //--------------------------------------------------------------------------------------

    return 0;
}

//------------------------------------------------------------------------------------
// Function Definitions
//------------------------------------------------------------------------------------
void GenerateNewEquation(MathEquation *eq, int level) {
    int opType;

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
                eq->num1 = rand() % 21; // 0-20
                eq->num2 = rand() % (21 - eq->num1); // Ensure sum <= 20
            } else {
                eq->num1 = 5 + rand() % 45;
                eq->num2 = 5 + rand() % 45;
            }
            eq->operation = '+';
            eq->correctAnswer = eq->num1 + eq->num2;
            break;

        case 1: // Subtraction
            if (level == 1) {
                eq->num1 = rand() % 21; // 0-20
                eq->num2 = rand() % (eq->num1 + 1); // Ensure result >= 0
            } else {
                eq->num1 = 20 + rand() % 60;
                eq->num2 = 5 + rand() % (eq->num1 - 5);
            }
            eq->operation = '-';
            eq->correctAnswer = eq->num1 - eq->num2;
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
}

void SpawnDrones(Drone drones[], MathEquation *eq, int *activeDroneCount) {
    int numDrones = 3 + rand() % 3; // 3-5 drones
    if (numDrones > MAX_DRONES) numDrones = MAX_DRONES;

    // Collect all existing answers on screen
    int existingAnswers[MAX_DRONES];
    int existingCount = 0;
    for (int i = 0; i < MAX_DRONES; i++) {
        if (drones[i].active && drones[i].state == DRONE_FLYING) {
            existingAnswers[existingCount++] = drones[i].answer;
        }
    }

    // Generate answers, avoiding duplicates with existing drones
    int answers[MAX_DRONES];
    int correctIndex = rand() % numDrones;

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
            drones[i].position.x = 1200 + spawned * 150; // Start off-screen, staggered
            drones[i].position.y = 80 + rand() % 250;
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
                if (drones[i].isShahed && drones[i].position.x < 100) {
                    drones[i].state = DRONE_FALLING;
                    drones[i].animTimer = 0.0f;
                }
                // Non-Shahed drones just disappear off screen
                else if (drones[i].position.x < -150) {
                    drones[i].active = false;
                }
                break;

            case DRONE_EXPLODING:
                drones[i].animTimer += deltaTime;
                if (drones[i].animTimer > 0.3f) { // Show explosion for 0.3 seconds
                    drones[i].state = DRONE_DEAD;
                }
                break;

            case DRONE_FALLING:
                drones[i].animTimer += deltaTime;
                drones[i].position.x -= DRONE_SPEED * 0.5f * deltaTime; // Continue moving left (slower)
                drones[i].position.y += 150.0f * deltaTime; // Fall down faster

                // If Shahed hits ground (300px above bottom = 694 - 300 = 394), explode
                if (drones[i].isShahed && drones[i].position.y >= 394.0f) {
                    drones[i].state = DRONE_EXPLODING;
                    drones[i].animTimer = 0.0f;
                }
                // Non-Shahed drones just disappear when hitting ground or going off screen
                else if (drones[i].position.y >= 494.0f || drones[i].position.x < -150) {
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

        if (gepard->fireTimer > 0.05f) { // Each frame lasts 0.05s (faster animation)
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
        case DRONE_DEAD:
            sourceRec = (Rectangle){ 200, 0, 100, 100 };
            break;
    }

    // Blink effect for falling drones near ground (within 200px from ground = y >= 494)
    if (drone.state == DRONE_FALLING && drone.position.y >= 494.0f) {
        // Blink by checking if we're in a "visible" period (on/off every 0.1 seconds)
        int blinkCycle = (int)(drone.animTimer * 10) % 2;
        if (blinkCycle == 0) {
            return; // Don't draw (creates blink effect)
        }
    }

    DrawTextureRec(texture, sourceRec, drone.position, WHITE);
}

void DrawGepard(Texture2D texture, GepardTank gepard, Vector2 position) {
    Rectangle sourceRec;

    // Determine which cell to draw
    int row = gepard.isFiring ? gepard.fireFrame : 0; // 0 = bottom, 1 = middle, 2 = top
    int col = gepard.turretIndex; // 0-4

    sourceRec.x = col * 150;
    sourceRec.y = (2 - row) * 150; // Flip because bottom row is at y=300
    sourceRec.width = 150;
    sourceRec.height = 150;

    DrawTextureRec(texture, sourceRec, position, WHITE);
}

void DrawAmmo(int ammo, int screenWidth, int screenHeight, Font font) {
    int boxWidth = 15;
    int boxHeight = 8;
    int spacing = 3;
    int startX = screenWidth - 20;
    int startY = screenHeight - 30;

    DrawTextEx(font, "AMMO:", (Vector2){screenWidth - 120, screenHeight - 55}, 30, 1, BLACK);

    for (int i = 0; i < ammo; i++) {
        int x = startX - (i % 10) * (boxWidth + spacing);
        int y = startY - (i / 10) * (boxHeight + spacing);

        Color ammoColor = (ammo > 10) ? DARKGREEN : (ammo > 5) ? ORANGE : RED;
        DrawRectangle(x, y, boxWidth, boxHeight, ammoColor);
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

void UpdateProjectiles(Projectile projectiles[], Drone drones[], int *ammo, int *score, bool *shahedActive, float deltaTime) {
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        if (projectiles[i].active) {
            projectiles[i].position.x += projectiles[i].velocity.x * deltaTime;
            projectiles[i].position.y += projectiles[i].velocity.y * deltaTime;
            projectiles[i].lifetime += deltaTime;

            // Check collision with target drone
            int targetIdx = projectiles[i].targetDroneIndex;
            if (targetIdx >= 0 && targetIdx < MAX_DRONES &&
                drones[targetIdx].active && drones[targetIdx].state == DRONE_FLYING) {

                Vector2 droneCenter = {
                    drones[targetIdx].position.x + 50,
                    drones[targetIdx].position.y + 50
                };

                float dx = projectiles[i].position.x - droneCenter.x;
                float dy = projectiles[i].position.y - droneCenter.y;
                float distance = sqrtf(dx * dx + dy * dy);

                // Hit detection (within 30 pixels of drone center)
                if (distance < 30.0f) {
                    projectiles[i].active = false;

                    if (drones[targetIdx].isShahed) {
                        // Correct hit!
                        drones[targetIdx].state = DRONE_EXPLODING;
                        drones[targetIdx].animTimer = 0.0f;
                        *ammo += HIT_REWARD;
                        *score += 10;
                        *shahedActive = false; // Shahed destroyed, can generate new equation
                    } else {
                        // Wrong hit
                        drones[targetIdx].state = DRONE_FALLING;
                        drones[targetIdx].animTimer = 0.0f;
                        *score -= 5;
                    }
                }
            }

            // Deactivate if off-screen or lived too long
            if (projectiles[i].position.x < -10 || projectiles[i].position.x > 1200 ||
                projectiles[i].position.y < -10 || projectiles[i].position.y > 750 ||
                projectiles[i].lifetime > 2.0f) {
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
                projectiles[i].position.x - projectiles[i].velocity.x * 0.02f,
                projectiles[i].position.y - projectiles[i].velocity.y * 0.02f
            };
            DrawLineEx(projectiles[i].position, end, 3.0f, YELLOW);
            DrawCircleV(projectiles[i].position, 2.0f, ORANGE);
        }
    }
}
