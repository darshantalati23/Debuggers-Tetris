// File: main.cpp
#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>

#define WIDTH 10
#define HEIGHT 22 // Increased Height
#define BLOCK "\u2588" // Using Full Block Unicode character
#define EMPTY " "

enum class TetrominoType { I, O, T, S, Z, J, L };

// ANSI color codes
#define ANSI_COLOR_RESET   "\x1b[0m"
#define ANSI_COLOR_WHITE   "\x1b[37m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_ORANGE  "\x1b[38;5;208m" // Using extended color for orange

class Tetromino {
private:
    TetrominoType type;
    int rotation;
    int x, y;
    std::vector<std::vector<int>> shape;

    void initShape() {
        switch(type) {
            case TetrominoType::I:
                shape = {{0,0,0,0}, {1,1,1,1}, {0,0,0,0}, {0,0,0,0}}; break;
            case TetrominoType::O:
                shape = {{1,1}, {1,1}}; break;
            case TetrominoType::T:
                shape = {{0,1,0}, {1,1,1}, {0,0,0}}; break;
            case TetrominoType::S:
                shape = {{0,1,1}, {1,1,0}, {0,0,0}}; break;
            case TetrominoType::Z:
                shape = {{1,1,0}, {0,1,1}, {0,0,0}}; break;
            case TetrominoType::J:
                shape = {{1,0,0}, {1,1,1}, {0,0,0}}; break;
            case TetrominoType::L:
                shape = {{0,0,1}, {1,1,1}, {0,0,0}}; break;
        }
    }

public:
    Tetromino(TetrominoType t) : type(t), rotation(0), x(WIDTH/2-2), y(0) { initShape(); }

    void rotate() {
        rotation = (rotation + 1) % 4;
        std::vector<std::vector<int>> newShape(shape[0].size(), std::vector<int>(shape.size()));
        for (size_t i = 0; i < shape.size(); ++i)
            for (size_t j = 0; j < shape[0].size(); ++j)
                newShape[j][shape.size()-1-i] = shape[i][j];
        shape = newShape;
    }

    const std::vector<std::vector<int>>& getShape() const { return shape; }
    int getX() const { return x; }
    int getY() const { return y; }
    TetrominoType getType() const { return type; } // Expose type for color selection
    void move(int dx, int dy) { x += dx; y += dy; }
};

class Grid {
private:
    std::vector<std::vector<bool>> grid;

public:
    Grid() : grid(HEIGHT, std::vector<bool>(WIDTH, false)) {}

    bool isCollision(const Tetromino& t) const {
        for (size_t i = 0; i < t.getShape().size(); ++i) {
            for (size_t j = 0; j < t.getShape()[i].size(); ++j) {
                if (t.getShape()[i][j]) {
                    int nx = t.getX() + j;
                    int ny = t.getY() + i;
                    if (nx < 0 || nx >= WIDTH || ny >= HEIGHT) return true;
                    if (ny >= 0 && grid[ny][nx]) return true;
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
                    if (y >= 0) grid[y][x] = true;
                }
            }
        }
    }

    int clearLines() {
        int lines = 0;
        for (int y = HEIGHT-1; y >= 0; --y) {
            bool full = true;
            for (int x = 0; x < WIDTH; ++x)
                if (!grid[y][x]) { full = false; break; }

            if (full) {
                grid.erase(grid.begin() + y);
                grid.insert(grid.begin(), std::vector<bool>(WIDTH, false));
                lines++;
                y++;
            }
        }
        return lines;
    }

    const std::vector<std::vector<bool>>& getGrid() const { return grid; }
};

class Game {
private:
    Grid grid;
    Tetromino* current;
    int score;
    int level;
    bool gameOver;
    bool paused; // Pause state

    Tetromino* newPiece() {
        TetrominoType types[] = {TetrominoType::I, TetrominoType::O, TetrominoType::T,
                                    TetrominoType::S, TetrominoType::Z, TetrominoType::J, TetrominoType::L};
        return new Tetromino(types[rand() % 7]);
    }

    std::string getTetrominoColor(TetrominoType type) const {
        switch (type) {
            case TetrominoType::I: return ANSI_COLOR_CYAN;
            case TetrominoType::O: return ANSI_COLOR_YELLOW;
            case TetrominoType::T: return ANSI_COLOR_MAGENTA;
            case TetrominoType::S: return ANSI_COLOR_GREEN;
            case TetrominoType::Z: return ANSI_COLOR_RED;
            case TetrominoType::J: return ANSI_COLOR_BLUE;
            case TetrominoType::L: return ANSI_COLOR_ORANGE;
            default: return ANSI_COLOR_WHITE;
        }
    }

    void draw() const {
        std::system("clear");
        // Draw top border
        std::cout << ANSI_COLOR_WHITE;
        for (int x = 0; x < WIDTH + 2; ++x) std::cout << BLOCK;
        std::cout << ANSI_COLOR_RESET << std::endl;

        // Create a temporary grid to display the current tetromino
        std::vector<std::vector<bool>> tempGrid = grid.getGrid();
        const auto& shape = current->getShape();
        int tx = current->getX();
        int ty = current->getY();

        // Overlay the current tetromino onto the tempGrid
        for (size_t i = 0; i < shape.size(); ++i) {
            for (size_t j = 0; j < shape[i].size(); ++j) {
                if (shape[i][j]) {
                    int x = tx + j;
                    int y = ty + i;
                    if (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT) {
                        tempGrid[y][x] = true;
                    }
                }
            }
        }

        // Draw the combined grid with borders and colors
        for (int y = 0; y < HEIGHT; ++y) {
            std::cout << ANSI_COLOR_WHITE << BLOCK << ANSI_COLOR_RESET; // Left border
            for (int x = 0; x < WIDTH; ++x) {
                if (tempGrid[y][x]) {
                    std::cout << getTetrominoColor(current->getType()) << BLOCK << ANSI_COLOR_RESET;
                } else {
                    std::cout << EMPTY;
                }
            }
            std::cout << ANSI_COLOR_WHITE << BLOCK << ANSI_COLOR_RESET << std::endl; // Right border
        }

        // Draw bottom border
        std::cout << ANSI_COLOR_WHITE;
        for (int x = 0; x < WIDTH + 2; ++x) std::cout << BLOCK;
        std::cout << ANSI_COLOR_RESET << std::endl;

        std::cout << "Score: " << score << std::endl;
        std::cout << "Level: " << level << std::endl;
        if (paused) {
            std::cout << "Game Paused" << std::endl;
        }
    }

    char getInput() {
        struct termios oldt, newt;
        char ch = '\0';
        tcgetattr(STDIN_FILENO, &oldt);
        newt = oldt;
        newt.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);

        // Set non-blocking mode
        fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);
        read(STDIN_FILENO, &ch, 1);

        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
        fcntl(STDIN_FILENO, F_SETFL, 0); // Restore blocking mode
        return ch;
    }

public:
    Game() : score(0), level(1), gameOver(false), paused(false) {
        srand(time(0));
        current = newPiece();
    }

    ~Game() { delete current; }

    void handleInput() {
        char ch = getInput();
        if (paused) {
            if (ch == 'p' || ch == 'P') {
                paused = false; // Unpause on 'p' or 'P'
            } else if (ch == 27) {
                gameOver = true; // Quit from pause with ESC
            }
            return; // Skip game controls if paused
        }

        Tetromino temp = *current;

        switch(ch) {
            case 'a': temp.move(-1, 0); break; // Left
            case 'd': temp.move(1, 0); break; // Right
            case 'w': temp.rotate(); break;    // Rotate
            case 's': temp.move(0, 1); break; // Soft drop
            case ' ': while (!grid.isCollision(temp)) { *current = temp; temp.move(0, 1); } break; // Hard drop
            case 27: gameOver = true; return; // ESC to quit
            case 'p': case 'P': paused = true; break; // Pause on 'p' or 'P'
            case 'q': case 'Q': gameOver = true; return; // Quit on 'q' or 'Q'
        }

        if (!grid.isCollision(temp)) *current = temp;
    }

    void update() {
        if (paused) return; // Do not update game state if paused
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
            usleep(800000 / level); // Slightly faster fall speed (Reduced usleep value)
        }
        std::cout << "Game Over! Score: " << score << std::endl;
    }
};

int main() {
    Game game;
    game.run();
    return 0;
}