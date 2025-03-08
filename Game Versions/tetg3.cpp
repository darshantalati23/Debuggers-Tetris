#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
using namespace std;

////////////////////
//  Configuration //
////////////////////
#define WIDTH 10
#define HEIGHT 22
#define BLOCK "██"    // 2-char wide block to appear more square
#define EMPTY "  "    // 2 spaces for empty
#define DROP_DELAY 500000 // Base microseconds for piece to fall (level 1)

// ANSI color codes
#define ANSI_COLOR_RESET   "\x1b[0m"
#define ANSI_COLOR_WHITE   "\x1b[37m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_ORANGE  "\x1b[38;5;208m" // Extended color for orange

// Enum for the 7 Tetromino types
enum class TetrominoType { I, O, T, S, Z, J, L };

////////////////////////////////////////////
//  Low-level Input Handling (termios)    //
////////////////////////////////////////////

// Check if a key has been pressed (non-blocking)
int kbhit() {
    struct termios oldt, newt;
    int ch;
    int oldf;

    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);  // Disable canonical mode & echo
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

    ch = getchar();

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, oldf);

    if (ch != EOF) {
        ungetc(ch, stdin);
        return 1;
    }
    return 0;
}

// Get the pressed key (non-blocking)
char getKey() {
    if (kbhit()) {
        return getchar();
    }
    return '\0';
}

////////////////////////
//  Tetromino Class   //
////////////////////////

class Tetromino {
private:
    TetrominoType type;
    int rotation; // Not strictly needed if we store shape after rotation
    int x, y;     // Position of top-left corner
    vector<vector<int>> shape;

    // Initialize the shape matrix based on type
    void initShape() {
        switch(type) {
            case TetrominoType::I:
                // 4x4 for convenience
                shape = {{0,0,0,0},
                         {1,1,1,1},
                         {0,0,0,0},
                         {0,0,0,0}};
                break;
            case TetrominoType::O:
                // 2x2
                shape = {{1,1},
                         {1,1}};
                break;
            case TetrominoType::T:
                // 3x3
                shape = {{0,1,0},
                         {1,1,1},
                         {0,0,0}};
                break;
            case TetrominoType::S:
                shape = {{0,1,1},
                         {1,1,0},
                         {0,0,0}};
                break;
            case TetrominoType::Z:
                shape = {{1,1,0},
                         {0,1,1},
                         {0,0,0}};
                break;
            case TetrominoType::J:
                shape = {{1,0,0},
                         {1,1,1},
                         {0,0,0}};
                break;
            case TetrominoType::L:
                shape = {{0,0,1},
                         {1,1,1},
                         {0,0,0}};
                break;
        }
    }

public:
    Tetromino(TetrominoType t)
        : type(t), rotation(0), x(WIDTH/2 - 2), y(0) {
        initShape();
    }

    void rotate() {
        // Rotate shape 90° clockwise
        vector<vector<int>> newShape(shape[0].size(),
                                               vector<int>(shape.size()));
        for (size_t i = 0; i < shape.size(); ++i) {
            for (size_t j = 0; j < shape[i].size(); ++j) {
                newShape[j][shape.size()-1-i] = shape[i][j];
            }
        }
        shape = newShape;
    }

    // Movement
    void move(int dx, int dy) {
        x += dx;
        y += dy;
    }

    // Accessors
    TetrominoType getType() const { return type; }
    const vector<vector<int>>& getShape() const { return shape; }
    int getX() const { return x; }
    int getY() const { return y; }
};

/////////////////////
//  Grid Class     //
/////////////////////
// We'll store an integer for each cell:
// -1 means empty
// 0..6 means the cell is filled with that TetrominoType (I->0, O->1, T->2, S->3, Z->4, J->5, L->6)

class Grid {
private:
    vector<vector<int>> cells;

public:
    Grid() : cells(HEIGHT, vector<int>(WIDTH, -1)) {}

    // Check collision
    bool isCollision(const Tetromino& t) const {
        const auto& shape = t.getShape();
        for (size_t i = 0; i < shape.size(); ++i) {
            for (size_t j = 0; j < shape[i].size(); ++j) {
                if (shape[i][j] == 1) {
                    int nx = t.getX() + j;
                    int ny = t.getY() + i;
                    // Out of bounds
                    if (nx < 0 || nx >= WIDTH || ny >= HEIGHT) {
                        return true;
                    }
                    // Collides with existing block
                    if (ny >= 0 && cells[ny][nx] != -1) {
                        return true;
                    }
                }
            }
        }
        return false;
    }

    // Merge tetromino into grid
    void merge(const Tetromino& t) {
        int typeInt = static_cast<int>(t.getType());
        const auto& shape = t.getShape();
        for (size_t i = 0; i < shape.size(); ++i) {
            for (size_t j = 0; j < shape[i].size(); ++j) {
                if (shape[i][j] == 1) {
                    int nx = t.getX() + j;
                    int ny = t.getY() + i;
                    if (ny >= 0) {
                        cells[ny][nx] = typeInt;
                    }
                }
            }
        }
    }

    // Clear full lines
    int clearLines() {
        int linesCleared = 0;
        for (int row = HEIGHT - 1; row >= 0; row--) {
            bool full = true;
            for (int col = 0; col < WIDTH; col++) {
                if (cells[row][col] == -1) {
                    full = false;
                    break;
                }
            }
            if (full) {
                // Remove row
                for (int r = row; r > 0; r--) {
                    cells[r] = cells[r - 1];
                }
                // New empty row at top
                cells[0] = vector<int>(WIDTH, -1);
                linesCleared++;
                row++; // re-check same row
            }
        }
        return linesCleared;
    }

    // For rendering
    int getCell(int r, int c) const {
        return cells[r][c];
    }
};

/////////////////////
//  Game Class     //
/////////////////////
class Game {
private:
    Grid grid;
    Tetromino* current;     // Current falling piece
    bool gameOver;
    bool paused;
    int score;
    int level;
    int linesCleared;
    string playerName;

    // Create a new random tetromino
    Tetromino* newPiece() {
        TetrominoType types[] = {
            TetrominoType::I, TetrominoType::O, TetrominoType::T,
            TetrominoType::S, TetrominoType::Z, TetrominoType::J,
            TetrominoType::L
        };
        return new Tetromino(types[rand() % 7]);
    }

    // Return color string for a given tetromino type
    string getColor(int type) const {
        // Match enum order: I->0, O->1, T->2, S->3, Z->4, J->5, L->6
        switch(type) {
            case 0: return ANSI_COLOR_CYAN;     // I
            case 1: return ANSI_COLOR_YELLOW;   // O
            case 2: return ANSI_COLOR_MAGENTA;  // T
            case 3: return ANSI_COLOR_GREEN;    // S
            case 4: return ANSI_COLOR_RED;      // Z
            case 5: return ANSI_COLOR_BLUE;     // J
            case 6: return ANSI_COLOR_ORANGE;   // L
            default: return ANSI_COLOR_WHITE;
        }
    }

    // Draw the entire game state
    void draw() const {
        // Clear screen
        system("clear");

        // If paused, show instructions
        if (paused) {
            printPauseScreen();
            return;
        }

        // Print top border
        cout << ANSI_COLOR_WHITE;
        for (int x = 0; x < WIDTH + 2; ++x) {
            cout << BLOCK;
        }
        cout << ANSI_COLOR_RESET << "\n";

        // Combine the current falling piece into a temp buffer
        // so we can render everything together
        vector<vector<int>> temp(HEIGHT, vector<int>(WIDTH, -1));
        // Copy from grid
        for (int r = 0; r < HEIGHT; r++) {
            for (int c = 0; c < WIDTH; c++) {
                temp[r][c] = grid.getCell(r, c);
            }
        }
        // Overlay current tetromino
        const auto& shape = current->getShape();
        for (size_t i = 0; i < shape.size(); ++i) {
            for (size_t j = 0; j < shape[i].size(); ++j) {
                if (shape[i][j] == 1) {
                    int nx = current->getX() + j;
                    int ny = current->getY() + i;
                    if (nx >= 0 && nx < WIDTH && ny >= 0 && ny < HEIGHT) {
                        temp[ny][nx] = static_cast<int>(current->getType());
                    }
                }
            }
        }

        // Render row by row
        for (int r = 0; r < HEIGHT; r++) {
            cout << ANSI_COLOR_WHITE << BLOCK << ANSI_COLOR_RESET; // left border
            for (int c = 0; c < WIDTH; c++) {
                int cellType = temp[r][c];
                if (cellType == -1) {
                    // Empty
                    cout << EMPTY;
                } else {
                    // Color based on type
                    cout << getColor(cellType) << BLOCK << ANSI_COLOR_RESET;
                }
            }
            cout << ANSI_COLOR_WHITE << BLOCK << ANSI_COLOR_RESET << "\n"; // right border
        }

        // Print bottom border
        cout << ANSI_COLOR_WHITE;
        for (int x = 0; x < WIDTH + 2; ++x) {
            cout << BLOCK;
        }
        cout << ANSI_COLOR_RESET << "\n";

        // Display info
        cout << "Player: " << playerName << "\n";
        cout << "Score:  " << score << "\n";
        cout << "Level:  " << level << "\n";
    }

    // Print "How To Play" instructions
    void printHelp() const {
        cout << ANSI_COLOR_WHITE << "HOW TO PLAY:\n" << ANSI_COLOR_RESET;
        cout << "  A -> Move Left\n"
                  << "  D -> Move Right\n"
                  << "  W -> Rotate\n"
                  << "  S -> Soft Drop\n"
                  << "  Space -> Hard Drop\n"
                  << "  P -> Pause/Resume\n"
                  << "  ESC -> Quit\n\n";
    }

    // Print screen when paused
    void printPauseScreen() const {
        cout << "=== PAUSED ===\n\n";
        printHelp();
        cout << "Press 'P' again to resume, or 'ESC' to quit.\n\n";
        cout << "Score:  " << score << "\n";
        cout << "Level:  " << level << "\n";
    }

public:
    Game(const string& player)
        : current(nullptr),
          gameOver(false),
          paused(false),
          score(0),
          level(1),
          linesCleared(0),
          playerName(player)
    {
        srand(time(nullptr));
        current = newPiece();
    }

    ~Game() {
        delete current;
    }

    bool isGameOver() const {
        return gameOver;
    }

    // Process input
    void handleInput() {
        if (paused) {
            // If paused, only listen for unpause or quit
            char key = getKey();
            if (key == 'p' || key == 'P') {
                paused = false;
            } else if (key == 27) { // ESC
                gameOver = true;
            }
            return;
        }

        char key = getKey();
        if (key == '\0') return; // No key pressed

        Tetromino temp = *current;
        switch(key) {
            case 'a': // Left
                temp.move(-1, 0);
                if (!grid.isCollision(temp)) *current = temp;
                break;
            case 'd': // Right
                temp.move(1, 0);
                if (!grid.isCollision(temp)) *current = temp;
                break;
            case 's': // Soft drop
                temp.move(0, 1);
                if (!grid.isCollision(temp)) *current = temp;
                break;
            case 'w': // Rotate
                temp.rotate();
                if (!grid.isCollision(temp)) *current = temp;
                break;
            case ' ': // Hard drop
                while (!grid.isCollision(temp)) {
                    *current = temp;
                    temp.move(0, 1);
                }
                break;
            case 'p': case 'P': // Pause
                paused = true;
                break;
            case 27: // ESC
                gameOver = true;
                break;
            case 'q': case 'Q':
                gameOver = true;
                break;
            default:
                break;
        }
    }

    // Update game state
    void update() {
        if (paused) return;

        // Attempt to move down by 1
        Tetromino temp = *current;
        temp.move(0, 1);
        if (grid.isCollision(temp)) {
            // Merge into grid
            grid.merge(*current);
            // Clear lines
            int lines = grid.clearLines();
            if (lines > 0) {
                linesCleared += lines;
                score += lines * 100 * level;
                // Increase level every 5 lines
                if (linesCleared >= level * 5) {
                    level++;
                }
            }
            // Spawn new piece
            delete current;
            current = newPiece();
            // Check immediate collision => game over
            if (grid.isCollision(*current)) {
                gameOver = true;
            }
        } else {
            *current = temp;
        }
    }

    // Main loop iteration
    void runFrame() {
        draw();
        handleInput();
        update();
    }

    int getLevel() const {
        return level;
    }
    
};

int main() {
    // Ask player name
    string playerName;
    cout << "Enter Player Name: ";
    getline(cin, playerName);

    // Create game
    Game game(playerName);

    // Show how to play once at the start
    cout << "\n";
    game.runFrame(); // Draw once to show initial state
    cout << "\n";
    cout << "=== WELCOME TO TETRIS ===\n\n";
    cout << "Press any key to start...\n";
    game.handleInput(); // consume any leftover input
    // Show instructions
    cout << "\n";
    cout << "===== HOW TO PLAY =====\n";
    cout << "  A -> Move Left\n"
              << "  D -> Move Right\n"
              << "  W -> Rotate\n"
              << "  S -> Soft Drop\n"
              << "  Space -> Hard Drop\n"
              << "  P -> Pause/Resume\n"
              << "  ESC or Q -> Quit\n\n";
    cout << "Press any key to continue...\n";
    while (true) {
        if (kbhit()) {
            getKey(); // Just read and break
            break;
        }
        usleep(50000);
    }

    // Main loop
    while (!game.isGameOver()) {
        // Update & draw the game state
        game.runFrame();
    
        // Let's define a dynamic delay:
        // Base 500000µs (0.5s) minus 50000µs per level above 1.
        int speed = 500000 - ((game.getLevel() - 1) * 50000);
        if (speed < 100000) speed = 100000; // minimum 0.1s
    
        usleep(speed);
    }
    
    cout << "\nGame Over! Thanks for playing, " << playerName << "!\n";
    return 0;
}