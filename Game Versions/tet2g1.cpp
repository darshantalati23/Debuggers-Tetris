#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <string>
#include <cctype>
#include <time.h>
using namespace std;

#define WIDTH 11       // Grid width (in cells)
#define HEIGHT 22      // Grid height (in cells)
#define BLOCK "\u2588\u2588" // Double block for a more square appearance
#define EMPTY "  "
#define BOLD "\x1b[1m"
#define UNBOLD "\x1b[22m"

// ANSI escape sequences
#define ANSI_CLEAR "\x1b[2J\x1b[H"
#define ANSI_COLOR_RESET "\x1b[0m"
#define ANSI_COLOR_BG "\x1b[48;5;234m"
#define ANSI_COLOR_BORDER "\x1b[38;5;245m"

// Unique colors for tetromino types
#define ANSI_COLOR_CYAN    "\x1b[38;5;87m"
#define ANSI_COLOR_YELLOW  "\x1b[38;5;226m"
#define ANSI_COLOR_MAGENTA "\x1b[38;5;201m"
#define ANSI_COLOR_GREEN   "\x1b[38;5;46m"
#define ANSI_COLOR_RED     "\x1b[38;5;196m"
#define ANSI_COLOR_BLUE    "\x1b[38;5;33m"
#define ANSI_COLOR_ORANGE  "\x1b[38;5;208m"
#define ANSI_COLOR_WHITE   "\x1b[37m"

enum class TetrominoType { I, O, T, S, Z, J, L };

// getInput() uses termios and fcntl to attempt a non-blocking read.
// If no key is pressed, it returns '\0'.
char getInput() {
    struct termios oldt, newt;
    int oldf;
    char ch = '\0';
    
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO); // disable canonical mode & echo
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    
    oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);
    
    int nread = read(STDIN_FILENO, &ch, 1);
    
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, oldf);
    
    if(nread == -1)
        return '\0';
    return ch;
}

class Tetromino {
private:
    TetrominoType type;
    int rotation;
    int x, y; // position (top-left of the shape matrix)
    vector<vector<int>> shape;
    string color;

    void initShape() {
        switch(type) {
            case TetrominoType::I:
                shape = {{0,0,0,0},
                         {1,1,1,1},
                         {0,0,0,0},
                         {0,0,0,0}};
                color = ANSI_COLOR_CYAN;
                break;
            case TetrominoType::O:
                shape = {{1,1},
                         {1,1}};
                color = ANSI_COLOR_YELLOW;
                break;
            case TetrominoType::T:
                shape = {{0,1,0},
                         {1,1,1},
                         {0,0,0}};
                color = ANSI_COLOR_MAGENTA;
                break;
            case TetrominoType::S:
                shape = {{0,1,1},
                         {1,1,0},
                         {0,0,0}};
                color = ANSI_COLOR_GREEN;
                break;
            case TetrominoType::Z:
                shape = {{1,1,0},
                         {0,1,1},
                         {0,0,0}};
                color = ANSI_COLOR_RED;
                break;
            case TetrominoType::J:
                shape = {{1,0,0},
                         {1,1,1},
                         {0,0,0}};
                color = ANSI_COLOR_BLUE;
                break;
            case TetrominoType::L:
                shape = {{0,0,1},
                         {1,1,1},
                         {0,0,0}};
                color = ANSI_COLOR_ORANGE;
                break;
        }
    }

public:
    Tetromino(TetrominoType t)
        : type(t), rotation(0), x(WIDTH/2 - 2), y(0) {
        initShape();
    }

    // Rotate the tetromino 90° clockwise
    void rotate() {
        rotation = (rotation + 1) % 4;
        vector<vector<int>> newShape(shape[0].size(), vector<int>(shape.size()));
        for (size_t i = 0; i < shape.size(); ++i)
            for (size_t j = 0; j < shape[0].size(); ++j)
                newShape[j][shape.size()-1-i] = shape[i][j];
        shape = newShape;
    }

    const vector<vector<int>>& getShape() const { return shape; }
    int getX() const { return x; }
    int getY() const { return y; }
    string getColor() const { return color; }
    TetrominoType getType() const { return type; }
    void move(int dx, int dy) { x += dx; y += dy; }
};

// We store a string in each cell. Empty cells are "".
// Filled cells store the ANSI color string of the tetromino that occupied that cell.
class Grid {
private:
    vector<vector<string>> cells;

public:
    Grid() : cells(HEIGHT, vector<string>(WIDTH, "")) {}

    // Check if placing tetromino 't' would cause a collision.
    bool isCollision(const Tetromino& t) const {
        const auto &shape = t.getShape();
        for (size_t i = 0; i < shape.size(); ++i) {
            for (size_t j = 0; j < shape[i].size(); ++j) {
                if (shape[i][j]) {
                    int nx = t.getX() + j;
                    int ny = t.getY() + i;
                    if (nx < 0 || nx >= WIDTH || ny >= HEIGHT)
                        return true;
                    if (ny >= 0 && cells[ny][nx] != "")
                        return true;
                }
            }
        }
        return false;
    }

    // Merge tetromino 't' into the grid (fix its cells permanently).
    void merge(const Tetromino& t) {
        const auto &shape = t.getShape();
        for (size_t i = 0; i < shape.size(); ++i) {
            for (size_t j = 0; j < shape[i].size(); ++j) {
                if (shape[i][j]) {
                    int nx = t.getX() + j;
                    int ny = t.getY() + i;
                    if (ny >= 0)
                        cells[ny][nx] = t.getColor();
                }
            }
        }
    }

    // Clear any full lines and return how many were cleared.
    int clearLines() {
        int linesCleared = 0;
        for (int y = HEIGHT - 1; y >= 0; --y) {
            bool full = true;
            for (int x = 0; x < WIDTH; ++x) {
                if (cells[y][x] == "") { full = false; break; }
            }
            if (full) {
                cells.erase(cells.begin() + y);
                cells.insert(cells.begin(), vector<string>(WIDTH, ""));
                linesCleared++;
                y++; // re-check same row index since rows have shifted
            }
        }
        return linesCleared;
    }

    const vector<vector<string>>& getGrid() const { return cells; }
};

class Game {
private:
    Grid grid;
    Tetromino* current;
    int score;
    int level;
    bool gameOver;
    bool paused;
    string playerName;
    string buffer;  // For drawing the frame

    // Play sound when lines are cleared (optional)
    void playSound() const {
        system("aplay -q pop.wav & 2>/dev/null");
    }

    // Print the instructions (used at start and when paused)
    void printInstructions() const {
        cout << BOLD "HOW TO PLAY:\n" UNBOLD;
        cout << "A - Move Left\n"
                  << "D - Move Right\n"
                  << "W - Rotate\n"
                  << "S - Soft Drop\n"
                  << "Space - Hard Drop\n"
                  << "P - Pause/Resume\n"
                  << "Q or ESC - Quit\n\n";
    }

    // Create a new random tetromino.
    Tetromino* newPiece() {
        TetrominoType types[] = { TetrominoType::I, TetrominoType::O, TetrominoType::T,
                                  TetrominoType::S, TetrominoType::Z, TetrominoType::J, TetrominoType::L };
        return new Tetromino(types[rand() % 7]);
    }

    // Draw the border and grid into the buffer.
    void drawBorder(const vector<vector<string>> &displayGrid) {
        buffer += ANSI_COLOR_BORDER;
        buffer += "╔";
        for (int x = 0; x < WIDTH; x++) buffer += "══";
        buffer += "╗\n";
        for (int y = 0; y < HEIGHT; y++) {
            buffer += "║";
            for (int x = 0; x < WIDTH; x++) {
                if (displayGrid[y][x] != "")
                    buffer += displayGrid[y][x] + BLOCK + ANSI_COLOR_RESET;
                else
                    buffer += EMPTY;
            }
            buffer += "║\n";
        }
        buffer += "╚";
        for (int x = 0; x < WIDTH; x++) buffer += "══";
        buffer += "╝" ANSI_COLOR_RESET "\n";
    }

    // Draw the game state (grid, current tetromino, HUD) into the buffer.
    void draw() {
        buffer = ANSI_CLEAR ANSI_COLOR_BG;
        // HUD
        buffer += BOLD ANSI_COLOR_BORDER;
        buffer += "Player: " + playerName + "\n";
        buffer += "Score:  " + to_string(score) + "\n";
        buffer += "Level:  " + to_string(level) + "\n" UNBOLD "\n";
        
        // If paused, show pause message and instructions.
        if(paused) {
            buffer += BOLD "\n   PAUSED\n" UNBOLD;
            buffer += "Press 'P' to resume or 'Q'/ESC to quit.\n\n";
            printInstructions();
            cout << buffer << flush;
            return;
        }
        
        // Create a temporary grid with the current tetromino overlaid.
        vector<vector<string>> tempGrid = grid.getGrid();
        const auto& shape = current->getShape();
        int tx = current->getX();
        int ty = current->getY();
        for (size_t i = 0; i < shape.size(); i++) {
            for (size_t j = 0; j < shape[i].size(); j++) {
                if (shape[i][j]) {
                    int x = tx + j;
                    int y = ty + i;
                    if (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT) {
                        tempGrid[y][x] = current->getColor();
                    }
                }
            }
        }
        drawBorder(tempGrid);
        cout << buffer << flush;
    }

public:
    Game() : score(0), level(1), gameOver(false), paused(false) {
        srand(time(nullptr));
        cout << ANSI_CLEAR;
        cout << "Enter player name: ";
        getline(cin, playerName);
        printInstructions();
        cout << "Press any key to start...";
        getchar(); // Wait for key press
        current = newPiece();
    }

    ~Game() { delete current; }

    bool isGameOver() const { return gameOver; }

    // Handle user input (non-blocking)
    void handleInput() {
        char ch = getInput();
        if (paused) {
            if (tolower(ch) == 'p') paused = false;
            if (tolower(ch) == 'q' || ch == 27) gameOver = true;
            return;
        }
        if(ch == '\0') return;
        Tetromino temp = *current;
        switch (tolower(ch)) {
            case 'a': temp.move(-1, 0); break;
            case 'd': temp.move(1, 0); break;
            case 's': temp.move(0, 1); break;
            case 'w': temp.rotate(); break;
            case ' ': // Hard drop
                while (!grid.isCollision(temp)) { 
                    *current = temp; 
                    temp.move(0, 1); 
                }
                break;
            case 'p': paused = true; break;
            case 'q': case 27: gameOver = true; break;
            default: break;
        }
        if (!grid.isCollision(temp))
            *current = temp;
    }

    // Update game state (move tetromino down, merge when landed, clear lines, spawn new piece)
    void update() {
        if (paused) return;
        Tetromino temp = *current;
        temp.move(0, 1);
        if (grid.isCollision(temp)) {
            grid.merge(*current);
            int lines = grid.clearLines();
            if (lines > 0) {
                playSound();
                score += lines * 100 * level;
                level += lines / 5;
            }
            delete current;
            current = newPiece();
            if (grid.isCollision(*current)) gameOver = true;
        } else {
            *current = temp;
        }
    }

    // Run one frame (draw, handle input, update)
    void runFrame() {
        draw();
        handleInput();
        update();
    }
    
    // Main game loop
    void run() {
        struct timespec ts {0, 200000000}; // 50ms per frame
        while(!gameOver) {
            runFrame();
            nanosleep(&ts, nullptr);
        }
        system("aplay -q pop2.wav & 2>/dev/null");
        cout << ANSI_CLEAR << "GAME OVER! Final Score: " << score << "\n";
    }
};

int main() {
    Game game;
    game.run();
    return 0;
}
