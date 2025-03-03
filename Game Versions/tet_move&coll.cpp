#include <bits/stdc++.h>
using namespace std;

// Define Tetromino Shapes
const vector<vector<vector<int>>> tetrominoShapes = {
    { {0, 1}, {1, 1}, {2, 1}, {3, 1} },  // I
    { {0, 0}, {0, 1}, {1, 0}, {1, 1} },  // O
    { {1, 0}, {0, 1}, {1, 1}, {2, 1} },  // T
    { {0, 1}, {1, 1}, {1, 0}, {2, 0} },  // S
    { {0, 0}, {1, 0}, {1, 1}, {2, 1} },  // Z
    { {0, 1}, {0, 0}, {1, 0}, {2, 0} },  // J
    { {0, 0}, {1, 0}, {2, 0}, {2, 1} }   // L
};

// Tetromino Class
class Tetromino {
public:
    vector<pair<int, int>> blocks;  // Stores the blocks of a piece
    char symbol;  // Character representing the piece

    Tetromino(int type, char sym) {
        symbol = sym;
        for (auto block : tetrominoShapes[type]) {
            blocks.push_back({block[0], block[1]});
        }
    }    

    // Rotate (90 degrees clockwise)
    /*
    How Does Rotation Work?
        90° Clockwise Rotation Formula:
            x' = y
            y' = -x
        For each block (x, y), update it to (y, -x).
    */
    void rotate() {
        for (auto &block : blocks) {
            int temp = block.first;
            block.first = block.second;
            block.second = -temp;
        }
    }

    // Display the Tetromino (for testing)
    void display() {
        cout << "Tetromino Shape (" << symbol << "):\n";
        for (auto block : blocks) {
            cout << "(" << block.first << ", " << block.second << ")\n";
        }
        cout << endl;
    }
};

// Testing
int main() {
    Tetromino T(2, 'T');  // Create a T Tetromino
    T.display();

    cout << "Rotating...\n";
    T.rotate();
    T.display();

    return 0;
}