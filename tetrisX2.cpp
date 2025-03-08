#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <string>
#include <cctype>
#include <sstream>
#include <time.h>
#include <algorithm>
using namespace std;

////////////////////
//  Configuration //
////////////////////
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

//////////////////////////////////////////////
// Non-blocking Input and Key Sequence Code //
//////////////////////////////////////////////

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

// getKeySequence() collects an escape sequence if present.
string getKeySequence() {
    char ch = getInput();
    if(ch == '\0') return "";
    string seq;
    seq.push_back(ch);
    if(ch == 27) { // possible arrow key sequence; wait a short time for remaining characters
        usleep(30000); // wait 30ms for next chars
        char ch1 = getInput();
        if(ch1 != '\0') { seq.push_back(ch1); }
        char ch2 = getInput();
        if(ch2 != '\0') { seq.push_back(ch2); }
    }
    return seq;
}

////////////////////////////////////////
// Helper: Split string by newline  //
////////////////////////////////////////
vector<string> splitString(const string &s, char delimiter='\n') {
    vector<string> lines;
    istringstream iss(s);
    string line;
    while(getline(iss, line, delimiter)) {
        lines.push_back(line);
    }
    return lines;
}

////////////////////////////////////////
//         Tetromino Class            //
////////////////////////////////////////
class Tetromino {
private:
    TetrominoType type;
    int rotation;
    int x, y; // Position (top-left of the shape matrix)
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

////////////////////////////////////////
//            Grid Class              //
////////////////////////////////////////
class Grid {
private:
    // Each cell: empty string "" means empty; otherwise store ANSI color string.
    vector<vector<string>> cells;
public:
    Grid() : cells(HEIGHT, vector<string>(WIDTH, "")) {}
    
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
                y++; // re-check same row index
            }
        }
        return linesCleared;
    }
    
    const vector<vector<string>>& getGrid() const { return cells; }
};

////////////////////////////////////////
//           Game Class               //
////////////////////////////////////////
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
    
    // Print instructions (used at start and when paused)
    void printInstructions() const {
        cout << BOLD "HOW TO PLAY:\n" UNBOLD;
        cout << "Player 1 (Left): W/A/S/D, Space for hard drop\n";
        cout << "Player 2 (Right): Arrow keys, Enter for hard drop\n";
        cout << "P - Pause/Resume, Q or ESC - Quit\n\n";
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
    
    // Draw the game state (HUD, grid, current tetromino) into the buffer.
    void draw() {
        buffer = ANSI_CLEAR ANSI_COLOR_BG;
        // HUD
        buffer += BOLD ANSI_COLOR_BORDER;
        buffer += "Player: " + playerName + "\n";
        buffer += "Score:  " + to_string(score) + "\n";
        buffer += "Level:  " + to_string(level) + "\n" UNBOLD "\n";
        
        if(paused) {
            buffer += BOLD "\n   PAUSED\n" UNBOLD;
            buffer += "Press 'P' to resume or 'Q'/ESC to quit.\n\n";
            drawBorder(grid.getGrid());
            return;
        }
        
        // Overlay current tetromino onto a temporary grid.
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
    }
    
public:
    Game(const string &name) : score(0), level(1), gameOver(false), paused(false), playerName(name) {
        srand(time(nullptr));
        cout << ANSI_CLEAR;
        printInstructions();
        cout << "Press any key to start...";
        getchar();
        current = newPiece();
    }
    
    ~Game() { delete current; }
    
    bool isGameOver() const { return gameOver; }
    string getBuffer() const { return buffer; }
    
    // Player 1 input: WASD and space.
    void handleInputP1(const string &key) {
        if (paused) {
            if (tolower(key[0]) == 'p') paused = false;
            if (tolower(key[0]) == 'q' || key[0] == 27) gameOver = true;
            return;
        }
        Tetromino temp = *current;
        char ch = tolower(key[0]);
        switch(ch) {
            case 'a': temp.move(-1, 0); break;
            case 'd': temp.move(1, 0); break;
            case 's': temp.move(0, 1); break;
            case 'w': temp.rotate(); break;
            case ' ':
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
    
    // Player 2 input: Arrow keys and Enter.
    void handleInputP2(const string &key) {
        if (paused) {
            if (tolower(key[0]) == 'p') paused = false;
            if (tolower(key[0]) == 'q' || key[0] == 27) gameOver = true;
            return;
        }
        Tetromino temp = *current;
        // Only process key if key sequence length is >1 (to avoid global dispatch issues)
        if (key.size() >= 2 && key.substr(0,2) == "\x1b[") {
            if (key == "\x1b[D") { // Left arrow
                temp.move(-1, 0);
            } else if (key == "\x1b[C") { // Right arrow
                temp.move(1, 0);
            } else if (key == "\x1b[B") { // Down arrow
                temp.move(0, 1);
            } else if (key == "\x1b[A") { // Up arrow (rotate)
                temp.rotate();
            } else if (key == "\n" || key == "\r") { // Enter for hard drop
                while (!grid.isCollision(temp)) {
                    *current = temp;
                    temp.move(0, 1);
                }
            }
        } else {
            // Global commands for P2 if single character.
            char ch = tolower(key[0]);
            if (ch == 'p') paused = true;
            if (ch == 'q' || key[0] == 27) gameOver = true;
        }
        if (!grid.isCollision(temp))
            *current = temp;
    }
    
    // Update game state: move tetromino down, merge when landed, clear lines, spawn new piece.
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
        update();
    }
};

int main() {
    cout << ANSI_CLEAR;
    string name1, name2;
    cout << "Enter name for Player 1 (Left): ";
    getline(cin, name1);
    cout << "Enter name for Player 2 (Right): ";
    getline(cin, name2);
    
    // Create two game instances
    Game game1(name1);
    Game game2(name2);
    
    // Main game loop: run frames and dispatch input.
    // Increase frame delay to 200ms (1/4th the original speed)
    struct timespec ts {0, 200000000}; // 200ms per frame
    while (!game1.isGameOver() && !game2.isGameOver()) {
        string keySeq = getKeySequence();
        if (!keySeq.empty()) {
            // If key sequence starts with escape and length > 1, it's for Player 2.
            if (keySeq.size() > 1 && keySeq.substr(0,2) == "\x1b[") {
                game2.handleInputP2(keySeq);
            } else {
                // Otherwise, assume it's for Player 1.
                game1.handleInputP1(keySeq);
            }
            // Global commands (only if single character) for both players.
            if (keySeq.size() == 1 && (tolower(keySeq[0]) == 'p' || tolower(keySeq[0]) == 'q' || keySeq[0] == 27)) {
                game1.handleInputP1(keySeq);
                game2.handleInputP2(keySeq);
            }
        }
        
        game1.runFrame();
        game2.runFrame();
        
        // Combine the two game buffers side-by-side.
        vector<string> lines1 = splitString(game1.getBuffer());
        vector<string> lines2 = splitString(game2.getBuffer());
        string combined;
        size_t maxLines = max(lines1.size(), lines2.size());
        for (size_t i = 0; i < maxLines; i++) {
            if (i < lines1.size()) combined += lines1[i];
            else combined += string(lines1[0].size(), ' ');
            combined += "    "; // gap between boards
            if (i < lines2.size()) combined += lines2[i];
            combined += "\n";
        }
        
        cout << ANSI_CLEAR << combined << flush;
        nanosleep(&ts, nullptr);
    }
    
    cout << ANSI_CLEAR;
    system("aplay -q pop2.wav & 2>/dev/null");
    cout << "GAME OVER!\n";
    return 0;
}