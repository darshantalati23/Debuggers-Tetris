#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <string>
using namespace std;

#define WIDTH 10
#define HEIGHT 22
#define BLOCK "\u2588\u2588"
#define GHOST "\u2591\u2591"
#define EMPTY "  "

enum class TetrominoType { I, O, T, S, Z, J, L };

// ANSI color codes
#define ANSI_COLOR_RESET   "\x1b[0m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_ORANGE  "\x1b[38;5;208m"
#define ANSI_COLOR_WHITE   "\x1b[37m"
#define ANSI_COLOR_GHOST   "\x1b[37;2m"

class Tetromino {
private:
    TetrominoType type;
    int rotation;
    int x, y;
    vector<vector<int>> shape;
    string color;

    void initShape() {
        switch(type) {
            case TetrominoType::I:
                shape = {{0,0,0,0}, {1,1,1,1}, {0,0,0,0}, {0,0,0,0}};
                color = ANSI_COLOR_CYAN; break;
            case TetrominoType::O:
                shape = {{1,1}, {1,1}};
                color = ANSI_COLOR_YELLOW; break;
            case TetrominoType::T:
                shape = {{0,1,0}, {1,1,1}, {0,0,0}};
                color = ANSI_COLOR_MAGENTA; break;
            case TetrominoType::S:
                shape = {{0,1,1}, {1,1,0}, {0,0,0}};
                color = ANSI_COLOR_GREEN; break;
            case TetrominoType::Z:
                shape = {{1,1,0}, {0,1,1}, {0,0,0}};
                color = ANSI_COLOR_RED; break;
            case TetrominoType::J:
                shape = {{1,0,0}, {1,1,1}, {0,0,0}};
                color = ANSI_COLOR_BLUE; break;
            case TetrominoType::L:
                shape = {{0,0,1}, {1,1,1}, {0,0,0}};
                color = ANSI_COLOR_ORANGE; break;
        }
    }

public:
    Tetromino(TetrominoType t) : type(t), rotation(0), x(WIDTH/2-2), y(0) { initShape(); }

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
    void move(int dx, int dy) { x += dx; y += dy; }
    Tetromino* clone() const { return new Tetromino(*this); }
};

class Grid {
private:
    vector<vector<string>> grid;

public:
    Grid() : grid(HEIGHT, vector<string>(WIDTH, "")) {}

    bool isCollision(const Tetromino& t) const {
        for (size_t i = 0; i < t.getShape().size(); ++i) {
            for (size_t j = 0; j < t.getShape()[i].size(); ++j) {
                if (t.getShape()[i][j]) {
                    int nx = t.getX() + j;
                    int ny = t.getY() + i;
                    if (nx < 0 || nx >= WIDTH || ny >= HEIGHT) return true;
                    if (ny >= 0 && grid[ny][nx] != "") return true;
                }
            }
        }
        return false;
    }

    void merge(const Tetromino& t) {
        for (size_t i = 0; i < t.getShape().size(); ++i) {
            for (size_t j = 0; j < t.getShape()[i].size(); ++j) {
                if (t.getShape()[i][j]) {
                    int x = t.getX() + j;
                    int y = t.getY() + i;
                    if (y >= 0) grid[y][x] = t.getColor();
                }
            }
        }
    }

    int clearLines() {
        int lines = 0;
        for (int y = HEIGHT-1; y >= 0; --y) {
            bool full = true;
            for (int x = 0; x < WIDTH; ++x)
                if (grid[y][x] == "") { full = false; break; }

            if (full) {
                grid.erase(grid.begin() + y);
                grid.insert(grid.begin(), vector<string>(WIDTH, ""));
                lines++;
                y++;
            }
        }
        if (lines > 0) system("aplay -q pop.wav &");
        return lines;
    }

    const vector<vector<string>>& getGrid() const { return grid; }
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

    void printInstructions() const {
        cout << "HOW TO PLAY:\n"
                  << "A - Move Left\n"
                  << "D - Move Right\n"
                  << "W - Rotate\n"
                  << "S - Soft Drop\n"
                  << "Space - Hard Drop\n"
                  << "P - Pause/Resume\n"
                  << "Q/ESC - Quit\n\n";
    }

    Tetromino* newPiece() {
        TetrominoType types[] = {TetrominoType::I, TetrominoType::O, TetrominoType::T,
                                 TetrominoType::S, TetrominoType::Z, TetrominoType::J, TetrominoType::L};
        return new Tetromino(types[rand() % 7]);
    }

    void drawGhost(Tetromino* ghost, vector<vector<string>>& tempGrid) const {
        while (!grid.isCollision(*ghost)) ghost->move(0, 1);
        ghost->move(0, -1);
        
        for (size_t i = 0; i < ghost->getShape().size(); ++i) {
            for (size_t j = 0; j < ghost->getShape()[i].size(); ++j) {
                if (ghost->getShape()[i][j]) {
                    int x = ghost->getX() + j;
                    int y = ghost->getY() + i;
                    if (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT) {
                        tempGrid[y][x] = ANSI_COLOR_GHOST;
                    }
                }
            }
        }
    }

    void draw() const {
        system("clear");
        cout << ANSI_COLOR_RESET;
        cout << "Player: " << playerName << "\n";
        
        // Center aligned score
        string scoreLine = "Score: " + to_string(score) + "  Level: " + to_string(level);
        int padding = ((WIDTH*2 + 4) - scoreLine.length()) / 2;
        cout << string(padding > 0 ? padding : 0, ' ') << scoreLine << "\n\n";

        vector<vector<string>> tempGrid = grid.getGrid();
        
        // Draw ghost piece
        Tetromino* ghost = current->clone();
        drawGhost(ghost, tempGrid);
        delete ghost;

        // Draw current piece
        const auto& shape = current->getShape();
        int tx = current->getX();
        int ty = current->getY();
        for (size_t i = 0; i < shape.size(); ++i) {
            for (size_t j = 0; j < shape[i].size(); ++j) {
                if (shape[i][j]) {
                    int x = tx + j;
                    int y = ty + i;
                    if (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT) {
                        tempGrid[y][x] = current->getColor();
                    }
                }
            }
        }

        // Draw game board
        cout << ANSI_COLOR_WHITE << BLOCK << ANSI_COLOR_RESET;
        for (int x = 0; x < WIDTH; x++) cout << BLOCK;
        cout << ANSI_COLOR_WHITE << BLOCK << ANSI_COLOR_RESET << endl;

        for (int y = 0; y < HEIGHT; ++y) {
            cout << ANSI_COLOR_WHITE << BLOCK << ANSI_COLOR_RESET;
            for (int x = 0; x < WIDTH; ++x) {
                if (tempGrid[y][x] != "") {
                    if (tempGrid[y][x] == ANSI_COLOR_GHOST) {
                        cout << ANSI_COLOR_GHOST << GHOST << ANSI_COLOR_RESET;
                    } else {
                        cout << tempGrid[y][x] << BLOCK << ANSI_COLOR_RESET;
                    }
                } else {
                    cout << EMPTY;
                }
            }
            cout << ANSI_COLOR_WHITE << BLOCK << ANSI_COLOR_RESET << endl;
        }

        cout << ANSI_COLOR_WHITE << BLOCK << ANSI_COLOR_RESET;
        for (int x = 0; x < WIDTH; x++) cout << BLOCK;
        cout << ANSI_COLOR_WHITE << BLOCK << ANSI_COLOR_RESET << endl;

        if (paused) {
            cout << "\nPAUSED\n";
            printInstructions();
        }
    }

    char getInput() {
        struct termios oldt, newt;
        char ch = '\0';
        tcgetattr(STDIN_FILENO, &oldt);
        newt = oldt;
        newt.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);
        fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);

        // Read all pending input
        char buf[128];
        int bytesRead;
        while ((bytesRead = read(STDIN_FILENO, buf, sizeof(buf))) > 0) {
            ch = buf[bytesRead-1];
        }

        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
        return ch;
    }

public:
    Game() : score(0), level(1), gameOver(false), paused(false) {
        srand(time(0));
        cout << "Enter player name: ";
        getline(cin, playerName);
        printInstructions();
        cout << "Press any key to start...";
        getchar();
        current = newPiece();
    }

    ~Game() { delete current; }

    void handleInput() {
        char ch = getInput();
        if (paused) {
            if (tolower(ch) == 'p') paused = false;
            return;
        }

        Tetromino temp = *current;
        switch(tolower(ch)) {
            case 'a': temp.move(-1, 0); break;
            case 'd': temp.move(1, 0); break;
            case 'w': temp.rotate(); break;
            case 's': temp.move(0, 1); break;
            case ' ': 
                while (!grid.isCollision(temp)) { 
                    current->move(0, 1);
                    temp = *current;
                }
                current->move(0, -1);
                break;
            case 27: case 'q': gameOver = true; break;
            case 'p': paused = true; break;
        }

        if (!grid.isCollision(temp)) *current = temp;
    }

    void update() {
        if (paused) return;
        Tetromino temp = *current;
        temp.move(0, 1);

        if (grid.isCollision(temp)) {
            grid.merge(*current);
            int lines = grid.clearLines();
            score += lines * 100 * level;
            level += lines / 5;
            delete current;
            current = newPiece();
            if (grid.isCollision(*current)) gameOver = true;
        } else {
            *current = temp;
        }
    }

    void run() {
        while (!gameOver) {
            draw();
            handleInput();
            update();
            usleep(200000 / level); // Smoother gameplay
        }
        system("clear");
        cout << "GAME OVER! Final Score: " << score << "\n";
        system("aplay -q pop2.wav &");
    }
};

int main() {
    Game game;
    game.run();
    return 0;
}