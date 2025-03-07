// File: main.cpp
#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>

#define WIDTH 10
#define HEIGHT 20
#define BLOCK "#"
#define EMPTY " "

enum class TetrominoType { I, O, T, S, Z, J, L };

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
    
    Tetromino* newPiece() {
        TetrominoType types[] = {TetrominoType::I, TetrominoType::O, TetrominoType::T, 
                                TetrominoType::S, TetrominoType::Z, TetrominoType::J, TetrominoType::L};
        return new Tetromino(types[rand() % 7]);
    }
    
    void draw() const {
        std::system("clear");
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

        // Draw the combined grid
        for (int y = 0; y < HEIGHT; ++y) {
            for (int x = 0; x < WIDTH; ++x) {
                std::cout << (tempGrid[y][x] ? BLOCK : EMPTY);
            }
            std::cout << std::endl;
        }
        std::cout << "Score: " << score << std::endl;
        std::cout << "Level: " << level << std::endl;
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
    Game() : score(0), level(1), gameOver(false) {
        srand(time(0));
        current = newPiece();
    }
    
    ~Game() { delete current; }
    
    void handleInput() {
        char ch = getInput();
        Tetromino temp = *current;
        
        switch(ch) {
            case 'a': temp.move(-1, 0); break; // Left
            case 'd': temp.move(1, 0); break; // Right
            case 'w': temp.rotate(); break;   // Rotate
            case 's': temp.move(0, 1); break; // Soft drop
            case ' ': while (!grid.isCollision(temp)) { *current = temp; temp.move(0, 1); } break; // Hard drop
            case 27: gameOver = true; return; // ESC to quit
        }
        
        if (!grid.isCollision(temp)) *current = temp;
    }
    
    void update() {
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
            usleep(1000000 / level); // Adjust speed based on level
        }
        std::cout << "Game Over! Score: " << score << std::endl;
    }
};

int main() {
    Game game;
    game.run();
    return 0;
}