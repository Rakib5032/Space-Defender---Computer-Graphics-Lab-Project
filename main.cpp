#include <GL/glut.h>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <vector>
#include <string>
#include <cstdio> 
#include <algorithm> // Required for std::remove_if

// Window dimensions
const int WIDTH = 800;
const int HEIGHT = 600;

// Game states
enum GameState { MENU, PLAYING, GAME_OVER };
GameState gameState = MENU;

// Player properties
struct Player {
    float x, y;
    float size;
    float speed;
    int lives;
    int score;
} player;

// Bullet structure
struct Bullet {
    float x, y;
    float speed;
    bool active;
};

// Enemy structure
struct Enemy {
    float x, y;
    float speed;
    bool active;
    int type; // 0: circle, 1: triangle, 2: square
};

// Power-up structure
struct PowerUp {
    float x, y;
    float speed;
    bool active;
};

// Vector dynamic data type (Containers for game objects)
std::vector<Bullet> bullets;
std::vector<Enemy> enemies;
std::vector<PowerUp> powerUps;

// Animation variables
float starOffset = 0;
float enemySpawnTimer = 0;
float powerUpTimer = 0;

// === GAME LEVEL & DIFFICULTY VARIABLES ===
float gameLevelTimer = 0;
int currentLevel = 1;
int enemySpawnRate = 60; // Initial rate: 60 frames = 1 enemy/second
float lastHitTimer = 0;  // Timer for no-hit penalty
int playerShape = 0;     // Locked to Triangle (0)

// Controls
bool keys[256] = { false };

// --- Function Prototypes ---
void drawCircleMidpoint(float cx, float cy, float r);
void drawFilledCircle(float cx, float cy, float r);
void drawLineBresenham(int x1, int y1, int x2, int y2);
void drawText(float x, float y, const char* text);
void drawStars();
void update(int value);


// Initialize game
void init() {
    glClearColor(0.0, 0.0, 0.1, 1.0);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    // Set 2D coordinates: (0, 0) is bottom-left
    gluOrtho2D(0, WIDTH, 0, HEIGHT);
    glMatrixMode(GL_MODELVIEW);

    // Initialize player
    player.x = WIDTH / 2;
    player.y = 50;
    player.size = 20;
    player.speed = 5.0f;
    player.lives = 3;
    player.score = 0;

    srand(time(NULL));
}

// DDA Line Algorithm (Not used for primary drawing, but kept for completeness)
void drawLineDDA(float x1, float y1, float x2, float y2) {
    float dx = x2 - x1;
    float dy = y2 - y1;
    float steps = fabs(dx) > fabs(dy) ? fabs(dx) : fabs(dy);

    float xInc = dx / steps;
    float yInc = dy / steps;

    float x = x1, y = y1;

    glBegin(GL_POINTS);
    for (int i = 0; i <= steps; i++) {
        glVertex2f(round(x), round(y));
        x += xInc;
        y += yInc;
    }
    glEnd();
}

// Bresenham's Line Algorithm (Used for Ship Wings)
void drawLineBresenham(int x1, int y1, int x2, int y2) {
    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;
    int err = dx - dy;

    glBegin(GL_POINTS);
    while (true) {
        glVertex2i(x1, y1);

        if (x1 == x2 && y1 == y2) break;

        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x1 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y1 += sy;
        }
    }
    glEnd();
}

// Midpoint Circle Algorithm (Used for Ship Cockpit and Enemy Outlines)
void drawCircleMidpoint(float cx, float cy, float r) {
    int x = 0;
    int y = r;
    int d = 1 - r;

    glBegin(GL_POINTS);

    auto plotCirclePoints = [&](int x, int y) {
        glVertex2f(cx + x, cy + y);
        glVertex2f(cx - x, cy + y);
        glVertex2f(cx + x, cy - y);
        glVertex2f(cx - x, cy - y);
        glVertex2f(cx + y, cy + x);
        glVertex2f(cx - y, cy + x);
        glVertex2f(cx + y, cy - x);
        glVertex2f(cx - y, cy - x);
        };

    while (x <= y) {
        plotCirclePoints(x, y);
        x++;
        if (d < 0) {
            d += 2 * x + 1;
        }
        else {
            y--;
            d += 2 * (x - y) + 1;
        }
    }
    glEnd();
}

// Filled circle (Used for Life Icons, Power-ups, and Circular Enemy Body)
void drawFilledCircle(float cx, float cy, float r) {
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx, cy);
    for (int i = 0; i <= 360; i++) {
        float angle = i * 3.14159 / 180;
        glVertex2f(cx + r * cos(angle), cy + r * sin(angle));
    }
    glEnd();
}

// Draw text (Used for HUD and Menu Screens)
void drawText(float x, float y, const char* text) {
    glRasterPos2f(x, y);
    for (const char* c = text; *c != '\0'; c++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
    }
}

// Draw stars (Background scrolling effect)
void drawStars() {
    glColor3f(1.0, 1.0, 1.0);
    glPointSize(2.0);

    for (int i = 0; i < 100; i++) {
        float x = (i * 73) % WIDTH;
        // Use starOffset for scrolling and fmod for looping
        float y = fmod((i * 117 + starOffset), HEIGHT);
        glBegin(GL_POINTS);
        glVertex2f(x, y);
        glEnd();
    }
    glPointSize(1.0);
}

// Draw player spaceship (Triangle Ship with wobble animation)
void drawPlayer() {
    glPushMatrix();

    // 1. Translation: Move drawing origin to player's center (player.x, player.y)
    glTranslatef(player.x, player.y, 0);

    // 2. Rotation effect (wobble)
    float wobble = sin(glutGet(GLUT_ELAPSED_TIME) * 0.005) * 2;
    glRotatef(wobble, 0, 0, 1);

    glLineWidth(3.0);

    // --- Determine Ship Color (Red Alert on 1 Life) ---
    if (player.lives == 1) {
        glColor3f(1.0, 0.0, 0.0); // CRITICAL: Red
    }
    else {
        glColor3f(0.0, 0.8, 1.0); // Default Blue color
    }

    // --- Draw Triangle Ship ---
    glBegin(GL_TRIANGLES);
    glVertex2f(0, player.size); // Top point
    glVertex2f(-player.size / 2, -player.size / 2); // Bottom-left
    glVertex2f(player.size / 2, -player.size / 2); // Bottom-right
    glEnd();

    // Cockpit 
    glColor3f(0.3, 0.9, 1.0);
    drawCircleMidpoint(0, player.size / 3, player.size / 4);
    // Extra cockpits for style
    drawCircleMidpoint(10, player.size / 3, player.size / 4);
    drawCircleMidpoint(-10, player.size / 3, player.size / 4);
    drawCircleMidpoint(0, -player.size, player.size / 4);


    // Wings (Drawn using Bresenham's algorithm)
    glColor3f(0.0, 0.6, 0.8);
    drawLineBresenham(-player.size / 2, -player.size / 2, -player.size, -player.size);
    drawLineBresenham(player.size / 2, -player.size / 2, player.size, -player.size);

    glLineWidth(1.0);

    glPopMatrix();
}

// Draw bullet (Conditional appearance based on level)
void drawBullet(float x, float y) {

    // --- Determine Bullet Appearance based on Level ---
    if (currentLevel < 3) {
        // Level 1 or 2: Yellow (Original)
        glColor3f(1.0, 1.0, 0.0); // Yellow color
        glBegin(GL_QUADS);

        // Size: 4 units wide, 10 units tall
        glVertex2f(x - 2, y - 5);
        glVertex2f(x + 2, y - 5);
        glVertex2f(x + 2, y + 5);
        glVertex2f(x - 2, y + 5);

        glEnd();
    }
    else {
        // Level 3 or higher: Red (Power-up look for final level)
        glColor3f(1.0, 0.0, 0.0); // Red color
        glBegin(GL_QUADS);

        // Size: 6 units wide, 14 units tall
        glVertex2f(x - 3, y - 0);
        glVertex2f(x + 3, y - 1);
        glVertex2f(x, y + 7);
        glVertex2f(x - 3, y + 7);

        glEnd();
    }
}

// Draw enemy with different types (Circle, Triangle, Square, and DDA Line Enemy)
void drawEnemy(Enemy& enemy) {
    glPushMatrix();
    glTranslatef(enemy.x, enemy.y, 0);

    // Rotation animation
    float rotation = glutGet(GLUT_ELAPSED_TIME) * 0.1;
    glRotatef(rotation, 0, 0, 1);

    if (enemy.type == 0) { // Circle enemy
        glColor3f(1.0, 0.0, 0.0);
        drawFilledCircle(0, 0, 15);
        glColor3f(0.5, 0.0, 0.0);
        drawCircleMidpoint(0, 0, 15);
    }
    else if (enemy.type == 1) { // Triangle enemy
        glColor3f(1.0, 0.3, 0.0);
        glBegin(GL_TRIANGLES);
        glVertex2f(0, -20);
        glVertex2f(-15, 15);
        glVertex2f(15, 15);
        glEnd();
    }
    else if (enemy.type == 2) { // Square enemy
        glColor3f(0.8, 0.0, 0.8);
        glBegin(GL_QUADS);
        glVertex2f(-15, -15);
        glVertex2f(15, -15);
        glVertex2f(15, 15);
        glVertex2f(-15, 15);
        glEnd();
    }
    else { // Enemy type 3: DDA Line Diamond 🌟
        glColor3f(0.0, 1.0, 1.0); // Cyan color
        glLineWidth(2.0); // Make the lines thicker for visibility

        // Draw the four sides of a diamond using the DDA algorithm
        float size = 20.0f;

        // 1. Top to Right
        drawLineDDA(0, size, size, 0);
        // 2. Right to Bottom
        drawLineDDA(size, 0, 0, -size);
        // 3. Bottom to Left
        drawLineDDA(0, -size, -size, 0);
        // 4. Left to Top
        drawLineDDA(-size, 0, 0, size);

        glLineWidth(1.0);
    }

    glPopMatrix();
}

// Draw power-up (Green pulsing circle)
void drawPowerUp(float x, float y) {
    glPushMatrix();
    glTranslatef(x, y, 0);

    // Scaling animation (pulsing)
    float scale = 1.0 + 0.2 * sin(glutGet(GLUT_ELAPSED_TIME) * 0.01);
    glScalef(scale, scale, 1.0);

    glColor3f(0.0, 1.0, 0.0);
    drawFilledCircle(0, 0, 10);

    // Draw '+' sign
    glColor3f(1.0, 1.0, 1.0);
    glBegin(GL_LINES);
    glVertex2f(-5, 0);
    glVertex2f(5, 0);
    glVertex2f(0, -5);
    glVertex2f(0, 5);
    glEnd();

    glPopMatrix();
}

// Draw HUD (Score, Lives, Level, Life Icons)
void drawHUD() {
    glColor3f(1.0, 1.0, 1.0);

    // Lives Text
    char livesText[20];
    sprintf(livesText, "Lives: %d", player.lives);
    drawText(10, HEIGHT - 30, livesText);

    // Score Text
    char scoreText[20];
    sprintf(scoreText, "Score: %d", player.score);
    drawText(WIDTH - 120, HEIGHT - 30, scoreText);

    // Current Level Text
    char levelText[20];
    sprintf(levelText, "Level: %d", currentLevel);
    drawText(WIDTH / 2 - 40, HEIGHT - 30, levelText);

    // Draw life icons
    for (int i = 0; i < player.lives; i++) {
        glColor3f(1.0, 0.0, 0.0);
        drawFilledCircle(20 + i * 25, HEIGHT - 60, 8);
    }
}

// Draw menu
void drawMenu() {
    glColor3f(0.0, 1.0, 1.0);
    drawText(WIDTH / 2 - 100, HEIGHT / 2 + 50, "SPACE DEFENDER");

    glColor3f(1.0, 1.0, 1.0);
    drawText(WIDTH / 2 - 120, HEIGHT / 2, "Press SPACE to Start");
    drawText(WIDTH / 2 - 80, HEIGHT / 2 - 40, "Controls:");
    drawText(WIDTH / 2 - 100, HEIGHT / 2 - 70, "Arrows - Move");
    drawText(WIDTH / 2 - 100, HEIGHT / 2 - 90, "SPACE - Shoot");
    drawText(WIDTH / 2 - 100, HEIGHT / 2 - 110, "ESC - Quit");
    drawText(WIDTH / 2 - 100, HEIGHT / 2 - 130, "A/D/W/S - Also work");
}

// Draw game over screen (Including requested text)
void drawGameOver() {
    glColor3f(1.0, 0.0, 0.0);
    drawText(WIDTH / 2 - 80, HEIGHT / 2 + 70, "JOY BANGLA"); // Requested text
    drawText(WIDTH / 2 - 80, HEIGHT / 2 + 50, "GAME OVER");

    glColor3f(1.0, 1.0, 1.0);
    char scoreText[50];
    sprintf(scoreText, "Final Score: %d", player.score);
    drawText(WIDTH / 2 - 80, HEIGHT / 2, scoreText);
    drawText(WIDTH / 2 - 120, HEIGHT / 2 - 40, "Press SPACE to Restart");
    drawText(WIDTH / 2 - 80, HEIGHT / 2 - 70, "Press ESC to Quit");
}

// Update game logic (Called 60 times per second)
void update(int value) {
    if (gameState == PLAYING) {
        // Update background animation
        starOffset += 0.5;
        if (starOffset > HEIGHT) starOffset = 0;

        // --- Timer Increments ---
        enemySpawnTimer++;
        powerUpTimer++;
        gameLevelTimer++;
        lastHitTimer++;

        // --- 1. Level Management (Every 15 seconds) ---
        if (gameLevelTimer >= 900) {
            if (currentLevel < 3) {
                currentLevel++;
                // MODIFIED: Increase difficulty steeply
                enemySpawnRate = 60 - (currentLevel - 1) * 25;
            }
            gameLevelTimer = 0;
        }

        // --- 2. Player Penalty (If no hit for 5 seconds) ---
        if (lastHitTimer >= 300) {
            if (player.lives > 0) {
                player.lives--;
                lastHitTimer = 0;
            }
            if (player.lives <= 0) {
                gameState = GAME_OVER;
            }
        }

        // Update player position based on key presses
        if (keys['a'] || keys['A']) {
            player.x -= player.speed;
            if (player.x < player.size) player.x = player.size;
        }
        if (keys['d'] || keys['D']) {
            player.x += player.speed;
            if (player.x > WIDTH - player.size) player.x = WIDTH - player.size;
        }
        if (keys['w'] || keys['W']) {
            player.y += player.speed;
            if (player.y > HEIGHT - player.size) player.y = HEIGHT - player.size;
        }
        if (keys['s'] || keys['S']) {
            player.y -= player.speed;
            if (player.y < player.size) player.y = player.size;
        }

        // Update bullets
        for (auto& bullet : bullets) {
            if (bullet.active) {
                bullet.y += bullet.speed;
                if (bullet.y > HEIGHT) bullet.active = false;
            }
        }

        // Spawn enemies
        if (enemySpawnTimer > enemySpawnRate) {
            Enemy enemy;
            enemy.x = rand() % (WIDTH - 40) + rand() % 20;
            enemy.y = HEIGHT;

            // MODIFIED: Enemy speed increases with level
            enemy.speed = 2.0 + (rand() % 3) + (currentLevel * 0.5);

            enemy.active = true;
            enemy.type = rand() % 4;
            enemies.push_back(enemy);
            enemySpawnTimer = 0;
        }

        // Update enemies & Check collision with player
        for (auto& enemy : enemies) {
            if (enemy.active) {
                enemy.y -= enemy.speed;
                if (enemy.y < -30) enemy.active = false;

                // Check collision with player
                float dx = enemy.x - player.x;
                float dy = enemy.y - player.y;
                float distance = sqrt(dx * dx + dy * dy);
                if (distance < player.size + 15) {
                    enemy.active = false;
                    player.lives--;
                    if (player.lives <= 0) {
                        gameState = GAME_OVER;
                    }
                }
            }
        }

        // Check bullet-enemy collision (Resets lastHitTimer)
        for (auto& bullet : bullets) {
            if (bullet.active) {
                for (auto& enemy : enemies) {
                    if (enemy.active) {
                        float dx = bullet.x - enemy.x;
                        float dy = bullet.y - enemy.y;
                        float distance = sqrt(dx * dx + dy * dy);
                        if (distance < 20) { // Collision Radius
                            bullet.active = false;
                            enemy.active = false;
                            player.score += 10;
                            // Reset the timer on successful hit
                            lastHitTimer = 0;
                        }
                    }
                }
            }
        }

        // Spawn power-ups (Every 5 seconds/300 frames)
        if (powerUpTimer > 300) {
            PowerUp powerUp;
            powerUp.x = rand() % (WIDTH - 40) + 20;
            powerUp.y = HEIGHT;
            powerUp.speed = 1.5;
            powerUp.active = true;
            powerUps.push_back(powerUp);
            powerUpTimer = 0;
        }

        // Update power-ups
        for (auto& powerUp : powerUps) {
            if (powerUp.active) {
                powerUp.y -= powerUp.speed;
                if (powerUp.y < -20) powerUp.active = false;

                // Check collision with player
                float dx = powerUp.x - player.x;
                float dy = powerUp.y - player.y;
                float distance = sqrt(dx * dx + dy * dy);
                if (distance < player.size + 10) {
                    powerUp.active = false;
                    if (player.lives < 5) player.lives++; // Max 5 lives
                    player.score += 20;
                }
            }
        }

        // Remove inactive objects (Cleanup)
        bullets.erase(std::remove_if(bullets.begin(), bullets.end(),
            [](const Bullet& b) { return !b.active; }), bullets.end());
        enemies.erase(std::remove_if(enemies.begin(), enemies.end(),
            [](const Enemy& e) { return !e.active; }), enemies.end());
        powerUps.erase(std::remove_if(powerUps.begin(), powerUps.end(),
            [](const PowerUp& p) { return !p.active; }), powerUps.end());
    }

    glutPostRedisplay();
    glutTimerFunc(16, update, 0); // Next frame in 16ms (~60 FPS)
}

// Display function (Renders everything)
void display() {
    glClear(GL_COLOR_BUFFER_BIT);

    drawStars(); // Draw background first

    if (gameState == MENU) {
        drawMenu();
    }
    else if (gameState == PLAYING) {
        drawPlayer();

        // Draw active bullets
        for (auto& bullet : bullets) {
            if (bullet.active) {
                drawBullet(bullet.x, bullet.y);
            }
        }

        // Draw active enemies
        for (auto& enemy : enemies) {
            if (enemy.active) {
                drawEnemy(enemy);
            }
        }

        // Draw active power-ups
        for (auto& powerUp : powerUps) {
            if (powerUp.active) {
                drawPowerUp(powerUp.x, powerUp.y);
            }
        }

        drawHUD(); // Draw HUD last so it's on top
    }
    else if (gameState == GAME_OVER) {
        drawGameOver();
    }

    glutSwapBuffers();
}

// Keyboard input handlers
void keyboardDown(unsigned char key, int x, int y) {
    keys[key] = true;

    if (key == 27) { // ESC
        exit(0);
    }

    if (key == ' ') {
        if (gameState == MENU || gameState == GAME_OVER) {
            gameState = PLAYING;
            // Reset all game variables for restart
            player.lives = 3;
            player.score = 0;
            player.x = WIDTH / 2;
            player.y = 50;
            bullets.clear();
            enemies.clear();
            powerUps.clear();

            currentLevel = 1;
            enemySpawnRate = 60;
            gameLevelTimer = 0;
            lastHitTimer = 0;
            playerShape = 0;
        }
        else if (gameState == PLAYING) {
            // Shoot bullet
            Bullet bullet;
            bullet.x = player.x;
            bullet.y = player.y + player.size;
            bullet.speed = 10.0;
            bullet.active = true;
            bullets.push_back(bullet);
        }
    }
}

void keyboardUp(unsigned char key, int x, int y) {
    keys[key] = false;
}

// Special keyboard functions (For Arrow Keys)
void specialDown(int key, int x, int y) {
    switch (key) {
    case GLUT_KEY_LEFT:
        keys['a'] = true;
        break;
    case GLUT_KEY_RIGHT:
        keys['d'] = true;
        break;
    case GLUT_KEY_UP:
        keys['w'] = true;
        break;
    case GLUT_KEY_DOWN:
        keys['s'] = true;
        break;
    }
}

void specialUp(int key, int x, int y) {
    switch (key) {
    case GLUT_KEY_LEFT:
        keys['a'] = false;
        break;
    case GLUT_KEY_RIGHT:
        keys['d'] = false;
        break;
    case GLUT_KEY_UP:
        keys['w'] = false;
        break;
    case GLUT_KEY_DOWN:
        keys['s'] = false;
        break;
    }
}

// Main function
int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(WIDTH, HEIGHT);
    glutCreateWindow("Space Defender - 2D OpenGL Game");

    init();

    glutDisplayFunc(display);

    // Register keyboard functions
    glutKeyboardFunc(keyboardDown);
    glutKeyboardUpFunc(keyboardUp);

    // Register SPECIAL keyboard functions (for ARROW keys)
    glutSpecialFunc(specialDown);
    glutSpecialUpFunc(specialUp);

    glutTimerFunc(0, update, 0);

    glutMainLoop();
    return 0;
}