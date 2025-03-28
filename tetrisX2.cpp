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

// Tetromino Class [unchanged]
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
    Tetromino(TetrominoType t) : type(t), rotation(0), x(WIDTH/2 - 2), y(0) { initShape(); }

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
    void setPosition(int newX, int newY) { x = newX; y = newY; }
};

// Grid Class (unchanged)
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
                    if (nx < 0 || nx >= WIDTH || ny >= HEIGHT)
                        return true;
                    if (ny >= 0 && grid[ny][nx] != "")
                        return true;
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
                    if (y >= 0)
                        grid[y][x] = t.getColor();
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
                y++; // check same row index again
            }
        }
        if (lines > 0) system("aplay -q pop.wav &");
        return lines;
    }

    const vector<vector<string>>& getGrid() const { return grid; }
};

// ----- Player Class -----
// Encapsulates a single player's board, current tetromino, score, etc.
class Player {
private:
    Grid grid;
    Tetromino* current;
    string colorForGhost = ANSI_COLOR_GHOST;
    bool gameOverSoundPlayed = false; // ensure we play the game-over sound once

    // Returns a new random tetromino.
    Tetromino* newPiece() {
        TetrominoType types[] = {TetrominoType::I, TetrominoType::O, TetrominoType::T,
                                 TetrominoType::S, TetrominoType::Z, TetrominoType::J, TetrominoType::L};
        return new Tetromino(types[rand() % 7]);
    }

    // Draw ghost piece for current tetromino.
    void drawGhost(vector<vector<string>>& tempGrid) const {
        Tetromino* ghost = current->clone();
        while (!grid.isCollision(*ghost))
            ghost->move(0, 1);
        ghost->move(0, -1);
        for (size_t i = 0; i < ghost->getShape().size(); ++i) {
            for (size_t j = 0; j < ghost->getShape()[i].size(); ++j) {
                if (ghost->getShape()[i][j]) {
                    int x = ghost->getX() + j;
                    int y = ghost->getY() + i;
                    if (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT)
                        tempGrid[y][x] = colorForGhost;
                }
            }
        }
        delete ghost;
    }

public:
    string name;
    int score;
    int level;
    bool gameOver;
    bool paused;
    int playerId; // 1 or 2

    Player(int id, const string& n) : name(n), score(0), level(1),
        gameOver(false), paused(false), playerId(id) { current = newPiece(); }

    ~Player() { delete current; }

    // Process input command for this player.
    // cmd: "L", "R", "rotate", "soft", "hard", "pause", "quit"
    void processCommand(const string& cmd) {
        if (paused) {
            if (cmd == "pause")
                paused = false;
            return;
        }
        Tetromino temp = *current;
        if (cmd == "L")           temp.move(-1, 0);
        else if (cmd == "R")      temp.move(1, 0);
        else if (cmd == "rotate") temp.rotate();
        else if (cmd == "soft")   temp.move(0, 1);
        else if (cmd == "hard") {
            // hard drop: move until collision
            while (!grid.isCollision(temp)) {
                current->move(0, 1);
                temp = *current;
            }
            current->move(0, -1);
        }
        else if (cmd == "pause") { paused = true; return; }
        else if (cmd == "quit") { gameOver = true; return; }
        if (!grid.isCollision(temp))
            *current = temp;
    }

    // Update the player's board.
    void update() {
        if (paused || gameOver) return;
        Tetromino temp = *current;
        temp.move(0, 1);
        if (grid.isCollision(temp)) {
            grid.merge(*current);
            int lines = grid.clearLines();
            score += lines * 100 * level;
            level += lines / 5;
            delete current;
            current = newPiece();
            // Check for game over if any block exists in the top row.
            const auto& gridData = grid.getGrid();
            for (int x = 0; x < WIDTH; x++) {
                if (gridData[0][x] != "") {
                    gameOver = true;
                    break;
                }
            }
            // Also check if the new piece immediately collides.
            if (grid.isCollision(*current))
                gameOver = true;
        } else {
            *current = temp;
        }

        // If this player's game just ended, play pop2.wav once.
        if (gameOver && !gameOverSoundPlayed) {
            system("aplay -q pop2.wav &");
            gameOverSoundPlayed = true;
        }
    }

    // Render the player's board (including header) into a vector of strings.
    vector<string> render() const {
        vector<string> lines;
        stringstream ss;
        // Prepare temporary grid including ghost and current piece.
        vector<vector<string>> tempGrid = grid.getGrid();
        // Draw ghost piece
        {
            Tetromino* ghost = current->clone();
            while (!grid.isCollision(*ghost))
                ghost->move(0, 1);
            ghost->move(0, -1);
            for (size_t i = 0; i < ghost->getShape().size(); ++i) {
                for (size_t j = 0; j < ghost->getShape()[i].size(); ++j) {
                    if (ghost->getShape()[i][j]) {
                        int x = ghost->getX() + j;
                        int y = ghost->getY() + i;
                        if (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT)
                            tempGrid[y][x] = ANSI_COLOR_GHOST;
                    }
                }
            }
            delete ghost;
        }
        // Draw current piece
        const auto& shape = current->getShape();
        int tx = current->getX();
        int ty = current->getY();
        for (size_t i = 0; i < shape.size(); ++i)
            for (size_t j = 0; j < shape[i].size(); ++j)
                if (shape[i][j]) {
                    int x = tx + j;
                    int y = ty + i;
                    if (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT)
                        tempGrid[y][x] = current->getColor();
                }
        // Build header line.
        string header = name + "  Score: " + to_string(score) + "  Level: " + to_string(level);
        int headerPad = (WIDTH*2 + 4 - header.length())/2;
        ss << string(headerPad > 0 ? headerPad : 0, ' ') << header;
        lines.push_back(ss.str());
        ss.str("");
        // Build top border.
        ss << ANSI_COLOR_WHITE << BLOCK;
        for (int x = 0; x < WIDTH; x++) ss << BLOCK;
        ss << ANSI_COLOR_WHITE << BLOCK << ANSI_COLOR_RESET;
        lines.push_back(ss.str());
        ss.str("");
        // Build each row of the board.
        for (int y = 0; y < HEIGHT; ++y) {
            ss << ANSI_COLOR_WHITE << BLOCK << ANSI_COLOR_RESET;
            for (int x = 0; x < WIDTH; ++x) {
                if (tempGrid[y][x] != "") {
                    if (tempGrid[y][x] == ANSI_COLOR_GHOST)
                        ss << ANSI_COLOR_GHOST << GHOST << ANSI_COLOR_RESET;
                    else
                        ss << tempGrid[y][x] << BLOCK << ANSI_COLOR_RESET;
                } else {
                    ss << EMPTY;
                }
            }
            ss << ANSI_COLOR_WHITE << BLOCK << ANSI_COLOR_RESET;
            lines.push_back(ss.str());
            ss.str("");
        }
        // Build bottom border.
        ss << ANSI_COLOR_WHITE << BLOCK;
        for (int x = 0; x < WIDTH; x++) ss << BLOCK;
        ss << ANSI_COLOR_WHITE << BLOCK << ANSI_COLOR_RESET;
        lines.push_back(ss.str());
        // If paused, add a pause message.
        if (paused) {
            lines.push_back("  PAUSED");
        }
        return lines;
    }
};

// ----- Input Handling -----
// Nonblocking input read that returns a string containing the latest key sequence.
string getInput() {
    string input = "";
    struct termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);
    char buf[16];
    int bytesRead;
    while ((bytesRead = read(STDIN_FILENO, buf, sizeof(buf))) > 0) {
        for (int i = 0; i < bytesRead; i++)
            input.push_back(buf[i]);
    }
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return input;
}

// ----- MultiplayerGame Class -----
class MultiplayerGame {
private:
    Player player1;
    Player player2;
    bool globalQuit;
public:
    MultiplayerGame(const string& name1, const string& name2)
        : player1(1, name1), player2(2, name2), globalQuit(false) {
        srand(time(0));
    }

    // Dispatch input characters to the appropriate player commands.
    // For player1: keys: a (L), d (R), w (rotate), s (soft), space (hard)
    // For player2: arrow keys and Enter: ESC+[+ 'D' (L), ESC+[+'C' (R), ESC+[+'A' (rotate), ESC+[+'B' (soft), Enter (hard))
    void handleInput(const string& input) {
        size_t i = 0;
        while (i < input.size()) {
            char ch = input[i];
            // Check for escape sequence (arrow keys for Player2)
            if (ch == '\033' && i + 2 < input.size() && input[i+1]=='[') {
                char arrow = input[i+2];
                if (arrow == 'D') player2.processCommand("L");
                else if (arrow == 'C') player2.processCommand("R");
                else if (arrow == 'A') player2.processCommand("rotate");
                else if (arrow == 'B') player2.processCommand("soft");
                i += 3;
            } else {
                if (ch == 'a' || ch == 'A') {
                    player1.processCommand("L");
                } else if (ch == 'd' || ch == 'D') {
                    player1.processCommand("R");
                } else if (ch == 'w' || ch == 'W') {
                    player1.processCommand("rotate");
                } else if (ch == 's' || ch == 'S') {
                    player1.processCommand("soft");
                } else if (ch == ' ') {
                    player1.processCommand("hard");
                } else if (ch == '\n' || ch == '\r') { // Enter for Player2 hard drop
                    player2.processCommand("hard");
                } else if (tolower(ch) == 'p') {
                    // Global pause toggle.
                    player1.processCommand("pause");
                    player2.processCommand("pause");
                } else if (ch == 'q' || ch == 27) {
                    player1.processCommand("quit");
                    player2.processCommand("quit");
                    globalQuit = true;
                }
                i++;
            }
        }
    }

    // Draw both players side-by-side by combining their rendered lines.
    void draw() {
        system("clear");
        vector<string> board1 = player1.render();
        vector<string> board2 = player2.render();
        size_t maxLines = max(board1.size(), board2.size());
        for (size_t i = 0; i < maxLines; i++) {
            string line1 = (i < board1.size()) ? board1[i] : "";
            string line2 = (i < board2.size()) ? board2[i] : "";
            // Adjust spacing between the two boards.
            cout << line1 << "    " << line2 << "\n";
        }
        cout << "\nPress 'q' or ESC to quit.\n";
    }

    // Update both players.
    void update() {
        if (!player1.paused && !player1.gameOver) player1.update();
        if (!player2.paused && !player2.gameOver) player2.update();
    }

    // Check if both players are finished (or global quit was requested).
    bool isGameOver() {
        return globalQuit || (player1.gameOver && player2.gameOver);
    }

    void run() {
        while (!isGameOver()) {
            draw();
            string inp = getInput();
            if (!inp.empty()) handleInput(inp);
            update();
            usleep(300000 / ((player1.level + player2.level)/2 + 1));
        }
        system("clear");
        cout << "GAME OVER!\n";
        cout << player1.name << " Score: " << player1.score << "\n";
        cout << player2.name << " Score: " << player2.score << "\n";
        // If any game over sound hasn't been played (should not occur, but for safety)
        system("aplay -q pop2.wav &");
    }
};

int main() {
    string name1, name2;
    cout << "Enter Player 1 name (WASD & Spacebar): ";
    getline(cin, name1);
    cout << "Enter Player 2 name (Arrow Keys & Enter): ";
    getline(cin, name2);
    cout << "\nHOW TO PLAY:\n"
              << "Player 1: A - Left, D - Right, W - Rotate, S - Soft Drop, Space - Hard Drop\n"
              << "Player 2: Arrow Left/Right - Move, Arrow Up - Rotate, Arrow Down - Soft Drop, Enter - Hard Drop\n"
              << "P - Pause, Q/ESC - Quit\n\n"
              << "Press any key to start...";
    getchar();
    MultiplayerGame game(name1, name2);
    game.run();
    return 0;
}
