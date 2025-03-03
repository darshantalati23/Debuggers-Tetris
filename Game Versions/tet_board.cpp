#include <bits/stdc++.h>
using namespace std;

class Board {
private:
    static const int wd = 25; // Coloumns
    static const int ht = 35; // Rows
    vector<vector<char>> grid;

public:
    // Constructor: Initialise an empty grid
    Board() {
        grid.resize(ht, vector<char>(wd, '.')); // '.' means empty
    }

    void display() {
        #ifdef _WIN32
            system("cls");   // Windows
        #else
            system("clear"); // Linux/Mac
        #endif
        for (int i = 0; i < ht; i++) {
            cout << "| ";
            for (int j = 0; j < wd; j++) {
                cout << grid[i][j] << " ";
            }
            cout << "|\n";
        }
        cout << string(wd * 2 + 3, '-') << endl; // Bottom border
    }

    bool isRowFull(int row) {
        for (int j = 0; j < wd; j++) {
            if (grid[row][j] == '.') return false; // If any cell is empty, row is not full
        }
        return true;
    }

    // Clear a full row and shift above rows down
    void clearRow(int row) {
        for (int i = row; i > 0; i--) {
            grid[i] = grid[i - 1];  // Move each row downward
        }
        grid[0] = vector<char>(wd, '.'); // New empty row at the top
    }
};

int main() {
    Board board;
    board.display();
    return 0;
}