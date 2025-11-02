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
#define DRONE_SPEED 50.0f
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

//------------------------------------------------------------------------------------
// Function Declarations
//------------------------------------------------------------------------------------
void GenerateNewEquation(MathEquation *eq, int level);
void SpawnDrones(Drone drones[], MathEquation *eq, int *activeDroneCount);
void UpdateDrones(Drone drones[], float deltaTime);
void UpdateGepard(GepardTank *gepard, float deltaTime);
int GetTurretIndexFromMouse(int mouseX, int screenWidth);
void DrawDrone(Texture2D texture, Drone drone);
void DrawGepard(Texture2D texture, GepardTank gepard, Vector2 position);
void DrawAmmo(int ammo, int screenWidth, int screenHeight);

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void)
{
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 1024;
    const int screenHeight = 600;

    InitWindow(screenWidth, screenHeight, "Sky Over Kharkiv");
    SetTargetFPS(60);

    // Seed random
    srand(time(NULL));

    // Load textures
    Texture2D sahedTexture = LoadTexture("images/sahed.png");
    Texture2D gepardTexture = LoadTexture("images/gepard.png");

    // Game variables
    GepardTank gepard = { 0, 0.0f, false, 0 };
    Vector2 gepardPosition = { 50.0f, (float)screenHeight - 150.0f };

    Drone drones[MAX_DRONES] = {0};
    int activeDroneCount = 0;

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
            if (IsKeyPressed(KEY_P)) {
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
                                // Hit a drone
                                ammo -= SHOT_COST;
                                gepard.isFiring = true;
                                gepard.fireTimer = 0.0f;
                                gepard.fireFrame = 0;

                                if (drones[i].isShahed) {
                                    // Correct hit!
                                    drones[i].state = DRONE_EXPLODING;
                                    drones[i].animTimer = 0.0f;
                                    ammo += HIT_REWARD;
                                    score += 10;
                                    shahedActive = false; // Shahed destroyed, can generate new equation
                                } else {
                                    // Wrong hit
                                    drones[i].state = DRONE_FALLING;
                                    drones[i].animTimer = 0.0f;
                                    score -= 5;
                                }
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

            ClearBackground((Color){135, 206, 235, 255}); // Sky blue

            if (!levelSelected) {
                // Level selection screen
                DrawText("SKY OVER KHARKIV", screenWidth/2 - 200, screenHeight/2 - 120, 40, BLACK);
                DrawText("Defend against Shahed drones!", screenWidth/2 - 180, screenHeight/2 - 60, 20, DARKGRAY);
                DrawText("Solve the equation to identify the Shahed", screenWidth/2 - 220, screenHeight/2 - 30, 20, DARKGRAY);

                DrawText("SELECT LEVEL:", screenWidth/2 - 100, screenHeight/2 + 20, 25, BLACK);
                DrawText("Press 1: Easy (Addition & Subtraction, 0-20)", screenWidth/2 - 240, screenHeight/2 + 60, 20, DARKGREEN);
                DrawText("Press 2: Medium (+ Multiplication)", screenWidth/2 - 180, screenHeight/2 + 90, 20, ORANGE);
                DrawText("Press 3: Hard (+ Division)", screenWidth/2 - 140, screenHeight/2 + 120, 20, RED);
            } else if (gameStarted) {
                // Draw equation
                char equationText[64];
                sprintf(equationText, "Equation: %d %c %d = ?",
                        currentEquation.num1, currentEquation.operation, currentEquation.num2);
                DrawText(equationText, 20, 20, 30, BLACK);

                // Draw score and level
                char scoreText[32];
                sprintf(scoreText, "Score: %d", score);
                DrawText(scoreText, screenWidth - 150, 20, 25, BLACK);

                char levelText[32];
                sprintf(levelText, "Level: %d", level);
                DrawText(levelText, screenWidth - 150, 50, 25, DARKBLUE);

                // Draw drones
                for (int i = 0; i < MAX_DRONES; i++) {
                    if (drones[i].active && drones[i].state != DRONE_DEAD) {
                        DrawDrone(sahedTexture, drones[i]);

                        // Draw answer above drone (hide when paused)
                        if (drones[i].state == DRONE_FLYING && !paused) {
                            char answerText[16];
                            sprintf(answerText, "%d", drones[i].answer);
                            int textWidth = MeasureText(answerText, 20);
                            DrawText(answerText, drones[i].position.x + 50 - textWidth/2,
                                   drones[i].position.y - 25, 20, RED);
                        }
                    }
                }

                // Draw Gepard tank
                DrawGepard(gepardTexture, gepard, gepardPosition);

                // Draw ammo
                DrawAmmo(ammo, screenWidth, screenHeight);

                // Draw pause message
                if (paused) {
                    DrawRectangle(0, 0, screenWidth, screenHeight, (Color){0, 0, 0, 128});
                    DrawText("PAUSED", screenWidth/2 - 80, screenHeight/2 - 40, 50, WHITE);
                    DrawText("Press P to Resume", screenWidth/2 - 100, screenHeight/2 + 20, 20, WHITE);
                }

                // Draw game over message
                if (ammo < SHOT_COST) {
                    DrawText("OUT OF AMMO! Press R to Restart", screenWidth/2 - 200, screenHeight/2, 25, RED);
                }
            }

        EndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    UnloadTexture(sahedTexture);
    UnloadTexture(gepardTexture);
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

    // Generate wrong answers
    int answers[MAX_DRONES];
    int correctIndex = rand() % numDrones;

    for (int i = 0; i < numDrones; i++) {
        if (i == correctIndex) {
            answers[i] = eq->correctAnswer;
        } else {
            // Generate wrong answer
            int offset = (rand() % 20) - 10;
            if (offset == 0) offset = 5;
            answers[i] = eq->correctAnswer + offset;
        }
    }

    // Find free slots and spawn drones
    int spawned = 0;
    for (int i = 0; i < MAX_DRONES && spawned < numDrones; i++) {
        if (!drones[i].active || drones[i].state == DRONE_DEAD) {
            drones[i].position.x = 1100 + spawned * 150; // Start off-screen, staggered
            drones[i].position.y = 80 + rand() % 200;
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
                if (drones[i].position.x < -150) {
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
                drones[i].position.y += 100.0f * deltaTime; // Fall down
                if (drones[i].position.y > 650) {
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

        if (gepard->fireTimer > 0.1f) { // Each frame lasts 0.1s
            gepard->fireFrame++;
            gepard->fireTimer = 0.0f;

            if (gepard->fireFrame >= 3) { // bottom -> middle -> top
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

void DrawAmmo(int ammo, int screenWidth, int screenHeight) {
    int boxWidth = 15;
    int boxHeight = 8;
    int spacing = 3;
    int startX = screenWidth - 20;
    int startY = screenHeight - 30;

    DrawText("AMMO:", screenWidth - 100, screenHeight - 50, 20, BLACK);

    for (int i = 0; i < ammo; i++) {
        int x = startX - (i % 10) * (boxWidth + spacing);
        int y = startY - (i / 10) * (boxHeight + spacing);

        Color ammoColor = (ammo > 10) ? DARKGREEN : (ammo > 5) ? ORANGE : RED;
        DrawRectangle(x, y, boxWidth, boxHeight, ammoColor);
    }
}
