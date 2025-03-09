#include <iostream>
#include <vector>
#include <string>
#include <cstdlib>
#include <ctime>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <sstream>
#include <time.h>
#include <algorithm>
using namespace std;

#define WIDTH 11
#define HEIGHT 22
#define BLOCK "\u2588\u2588"
#define EMPTY "  "
#define BOLD "\x1b[1m"
#define UNBOLD "\x1b[22m"
#define ANSI_CLEAR "\x1b[2J\x1b[H"
#define ANSI_COLOR_RESET "\x1b[0m"
#define ANSI_COLOR_BORDER "\x1b[38;5;245m"

#define COLOR_CYAN    "\x1b[38;5;87m"
#define COLOR_YELLOW  "\x1b[38;5;226m"
#define COLOR_MAGENTA "\x1b[38;5;201m"
#define COLOR_GREEN   "\x1b[38;5;46m"
#define COLOR_RED     "\x1b[38;5;196m"
#define COLOR_BLUE    "\x1b[38;5;33m"
#define COLOR_ORANGE  "\x1b[38;5;208m"

enum class TetrominoType { I, O, T, S, Z, J, L };

char getInput() {
    termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    int oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);
    char ch = '\0';
    if (read(STDIN_FILENO, &ch, 1) < 0) ch = '\0';
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, oldf);
    return ch;
}

string getKeySequence() {
    char ch = getInput();
    if(ch == '\0') return "";
    string seq;
    seq.push_back(ch);
    if(ch == 27) {
        usleep(50000); // increased wait time to 50ms
        char ch1 = getInput();
        if(ch1 != '\0') seq.push_back(ch1);
        char ch2 = getInput();
        if(ch2 != '\0') seq.push_back(ch2);
    }
    return seq;
}

vector<string> splitString(const string &s) {
    vector<string> lines;
    istringstream iss(s);
    string line;
    while(getline(iss, line)) lines.push_back(line);
    return lines;
}

class Tetromino {
    TetrominoType type;
    int rotation, x, y;
    vector<vector<int>> shape;
    string color;
    void initShape() {
        switch(type) {
            case TetrominoType::I:
                shape = {{0,0,0,0},{1,1,1,1},{0,0,0,0},{0,0,0,0}};
                color = COLOR_CYAN; break;
            case TetrominoType::O:
                shape = {{1,1},{1,1}};
                color = COLOR_YELLOW; break;
            case TetrominoType::T:
                shape = {{0,1,0},{1,1,1},{0,0,0}};
                color = COLOR_MAGENTA; break;
            case TetrominoType::S:
                shape = {{0,1,1},{1,1,0},{0,0,0}};
                color = COLOR_GREEN; break;
            case TetrominoType::Z:
                shape = {{1,1,0},{0,1,1},{0,0,0}};
                color = COLOR_RED; break;
            case TetrominoType::J:
                shape = {{1,0,0},{1,1,1},{0,0,0}};
                color = COLOR_BLUE; break;
            case TetrominoType::L:
                shape = {{0,0,1},{1,1,1},{0,0,0}};
                color = COLOR_ORANGE; break;
        }
    }
public:
    Tetromino(TetrominoType t) : type(t), rotation(0), x(WIDTH/2-2), y(0) { initShape(); }
    void rotate() {
        rotation = (rotation + 1) % 4;
        vector<vector<int>> tmp(shape[0].size(), vector<int>(shape.size()));
        for(size_t i=0; i<shape.size(); i++)
            for(size_t j=0; j<shape[i].size(); j++)
                tmp[j][shape.size()-1-i] = shape[i][j];
        shape = tmp;
    }
    const vector<vector<int>>& getShape() const { return shape; }
    string getColor() const { return color; }
    int getX() const { return x; }
    int getY() const { return y; }
    void move(int dx, int dy) { x += dx; y += dy; }
};

class Grid {
    vector<vector<string>> cells;
public:
    Grid() : cells(HEIGHT, vector<string>(WIDTH, "")) {}
    bool isCollision(const Tetromino &t) const {
        auto &shape = t.getShape();
        for(size_t i=0;i<shape.size(); i++)
            for(size_t j=0;j<shape[i].size(); j++)
                if(shape[i][j]) {
                    int nx = t.getX()+j, ny = t.getY()+i;
                    if(nx<0 || nx>=WIDTH || ny>=HEIGHT) return true;
                    if(ny>=0 && cells[ny][nx]!="") return true;
                }
        return false;
    }
    void merge(const Tetromino &t) {
        auto &shape = t.getShape();
        for(size_t i=0;i<shape.size(); i++)
            for(size_t j=0;j<shape[i].size(); j++)
                if(shape[i][j]) {
                    int nx = t.getX()+j, ny = t.getY()+i;
                    if(ny>=0) cells[ny][nx] = t.getColor();
                }
    }
    int clearLines() {
        int cleared=0;
        for(int r=HEIGHT-1; r>=0; r--) {
            bool full=true;
            for(int c=0; c<WIDTH; c++)
                if(cells[r][c]=="") { full=false; break; }
            if(full) {
                cells.erase(cells.begin()+r);
                cells.insert(cells.begin(), vector<string>(WIDTH,""));
                cleared++;
                r++;
            }
        }
        return cleared;
    }
    const vector<vector<string>>& getCells() const { return cells; }
};

class Game {
    Grid grid;
    Tetromino *current;
    int score, level;
    bool over, paused;
    string playerName, frame;
    void playSound() const { system("aplay -q pop.wav &>/dev/null"); }
    Tetromino* newPiece() {
        TetrominoType arr[] = {TetrominoType::I, TetrominoType::O, TetrominoType::T,
                               TetrominoType::S, TetrominoType::Z, TetrominoType::J, TetrominoType::L};
        return new Tetromino(arr[rand()%7]);
    }
public:
    Game(const string &n) : current(nullptr), score(0), level(1), over(false), paused(false), playerName(n) {
        srand(time(nullptr));
        current = newPiece();
    }
    ~Game(){ delete current; }
    bool isOver() const { return over; }
    int getScore() const { return score; }
    string getName() const { return playerName; }
    string getFrame() const { return frame; }
    void handleInputP1(const string &key) {
        if(over) return;
        if(paused) {
            if(tolower(key[0])=='p') paused=false;
            if(tolower(key[0])=='q') { over=true; playSound(); }
            return;
        }
        Tetromino tmp = *current;
        char ch = tolower(key[0]);
        if(ch=='a') tmp.move(-1,0);
        else if(ch=='d') tmp.move(1,0);
        else if(ch=='s') tmp.move(0,1);
        else if(ch=='w') tmp.rotate();
        else if(ch==' ') { while(!grid.isCollision(tmp)) { *current = tmp; tmp.move(0,1); } }
        else if(ch=='p') paused = true;
        else if(ch=='q') { over = true; playSound(); }
        if(!grid.isCollision(tmp)) *current = tmp;
    }
    void handleInputP2(const string &key) {
        if(over) return;
        if(paused) {
            if(tolower(key[0])=='p') paused=false;
            if(tolower(key[0])=='q') { over=true; playSound(); }
            return;
        }
        Tetromino tmp = *current;
        if(key.size()>=2 && key.substr(0,2)=="\x1b[") {
            if(key=="\x1b[D") tmp.move(-1,0);
            else if(key=="\x1b[C") tmp.move(1,0);
            else if(key=="\x1b[B") tmp.move(0,1);
            else if(key=="\x1b[A") tmp.rotate();
        }
        else if(key=="\n" || key=="\r") { while(!grid.isCollision(tmp)) { *current = tmp; tmp.move(0,1); } }
        else {
            char ch = tolower(key[0]);
            if(ch=='p') paused = true;
            if(ch=='q') { over = true; playSound(); }
        }
        if(!grid.isCollision(tmp)) *current = tmp;
    }
    void update() {
        if(paused || over) return;
        Tetromino tmp = *current;
        tmp.move(0,1);
        if(grid.isCollision(tmp)) {
            grid.merge(*current);
            int lines = grid.clearLines();
            if(lines>0) { playSound(); score += lines*100*level; level += lines/5; }
            delete current; current = newPiece();
            if(grid.isCollision(*current)) { over = true; playSound(); }
        } else { *current = tmp; }
    }
    void draw(int secsLeft) {
        frame.clear();
        frame += ANSI_CLEAR;
        frame += playerName + " | Score:" + to_string(score) + " | Level:" + to_string(level) + " | Time:";
        int mm = secsLeft/60, ss = secsLeft%60;
        frame += to_string(mm) + ":" + (ss<10?"0":"") + to_string(ss) + "\n";
        if(paused) { frame += "PAUSED (p to resume, q to quit)\n"; drawBoard(grid.getCells()); return; }
        vector<vector<string>> temp = grid.getCells();
        if(!over && current) {
            auto &sh = current->getShape();
            for(size_t i=0; i<sh.size(); i++)
                for(size_t j=0; j<sh[i].size(); j++)
                    if(sh[i][j]) {
                        int nx = current->getX()+j, ny = current->getY()+i;
                        if(nx>=0 && nx<WIDTH && ny>=0 && ny<HEIGHT)
                            temp[ny][nx] = current->getColor();
                    }
        }
        drawBoard(temp);
    }
    void drawBoard(const vector<vector<string>> &cells) {
        frame += "┌";
        for(int i=0;i<WIDTH;i++) frame += "──";
        frame += "┐\n";
        for(int r=0;r<HEIGHT;r++){
            frame += "│";
            for(int c=0;c<WIDTH;c++){
                if(cells[r][c]!="") frame += cells[r][c] + BLOCK + ANSI_COLOR_RESET;
                else frame += EMPTY;
            }
            frame += "│\n";
        }
        frame += "└";
        for(int i=0;i<WIDTH;i++) frame += "──";
        frame += "┘\n";
    }
};

int main(){
    cout << ANSI_CLEAR;
    string n1, n2;
    cout << "Enter Player 1 Name: ";
    getline(cin, n1);
    cout << "Enter Player 2 Name: ";
    getline(cin, n2);
    Game g1(n1), g2(n2);
    const int totalTime = 600;
    time_t start = time(nullptr);
    timespec ts {0,200000000};
    while(true){
        int elapsed = (int)difftime(time(nullptr), start);
        int left = totalTime - elapsed;
        if(left <= 0) break;
        if(!g1.isOver()){ 
            string key1 = "";
            char ch1 = getInput();
            if(ch1 != '\0') key1.push_back(ch1);
            if(!key1.empty()) g1.handleInputP1(key1);
            g1.update();
        }
        if(!g2.isOver()){
            string key2 = getKeySequence();
            if(!key2.empty()) g2.handleInputP2(key2);
            g2.update();
        }
        if(g1.isOver() && g2.isOver()) break;
        g1.draw(left);
        string buf1 = g1.getFrame();
        g2.draw(left);
        string buf2 = g2.getFrame();
        vector<string> lines1 = splitString(buf1), lines2 = splitString(buf2);
        size_t mx = max(lines1.size(), lines2.size());
        string comb;
        for(size_t i = 0; i < mx; i++){
            if(i < lines1.size()) comb += lines1[i];
            comb += "    ";
            if(i < lines2.size()) comb += lines2[i];
            comb += "\n";
        }
        cout << ANSI_CLEAR << comb << flush;
        nanosleep(&ts, nullptr);
    }
    cout << ANSI_CLEAR;
    cout << "=== GAME OVER ===\n\n";
    cout << "Final Scores:\n";
    cout << " - " << n1 << ": " << g1.getScore() << "\n";
    cout << " - " << n2 << ": " << g2.getScore() << "\n\n";
    cout << BOLD << "Created by The Debuggers" << UNBOLD << "\n(See GroupIcon.jpg)\n\n";
    return 0;
}