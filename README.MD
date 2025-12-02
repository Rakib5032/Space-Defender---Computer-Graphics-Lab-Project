💻 CSE 422: Computer Graphics Lab Project
This project, Space Defender, serves as a practical application for the fundamental concepts taught in your Computer Graphics course ($\mathbf{CSE 422}$). It uses the OpenGL fixed-function pipeline to create an interactive 2D environment.
💡 Overall Idea
The project demonstrates how basic geometric algorithms and transformations are combined within a continuous game loop to create a dynamic, real-time application. The player controls a ship to destroy incoming enemies in a scrolling space environment, with difficulty increasing through three defined levels.

📊 Core Implemented Algorithms
The project directly utilizes classical graphics algorithms for drawing the fundamental shapes (rasterization):
Algorithm
Role in Project
Concept Demonstrated
DDA Line Algorithm
Draws the linear segments of the DDA Line Diamond enemy shape.
Efficient calculation of points along a line using floating-point math.
Bresenham's Line Algorithm
Typically implemented as a fast alternative to DDA (used for drawing lines where integer-only precision is required).
Integer-only calculation for faster, precise line rasterization.
Midpoint Circle Algorithm
Used by drawFilledCircle and drawCircleMidpoint to render the Circle Enemy and the Power-up shapes.
Using symmetry to minimize calculations (drawing only one octant).


⚙️ How the Project Works (The Game Loop)
The entire project operates using a continuous $\mathbf{60 \text{ FPS}}$ Game Loop, structured around three key functions:
1. Input Handlers (keyboardDown, specialDown)
They update the input state array (keys[]) whenever a key is pressed or released.
2. Update Function (update())
This is the physics and logic hub, called every 16 milliseconds.
It processes the input (keys[]) to move the player's coordinates.
It handles all game logic: moving enemies and bullets, checking collisions, increasing scores, and advancing the $\mathbf{15\text{-second levels}}$ by adjusting enemySpawnRate and enemy.speed.
3. Display Function (display())
This function redraws the entire scene after every update.
It uses OpenGL Transformations (glTranslatef, glScalef) to move the drawing origin to the correct position (e.g., the ship's center) before drawing the object's local geometry (vertices).
It renders the background, enemies, player, and the HUD in sequence.

