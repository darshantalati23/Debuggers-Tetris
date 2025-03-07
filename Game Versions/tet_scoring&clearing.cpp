#include <iostream>
#include <vector>

using namespace std;

// Board Size
const int wd = 10; // Width
const int ht = 20; // Height

// Board Class
class Board {
private:
    vector<vector<char>> grid;  // 2D Grid

public:
    Board() {
        grid.resize(ht, vector<char>(wd, '.')); // Empty board
    }

    // Display Board
    void display() {
        system("clear"); // "cls" for Windows
        for (int i = 0; i < ht; i++) {
            cout << "| ";
            for (int j = 0; j < wd; j++) {
                cout << grid[i][j] << " ";
            }
            cout << "|\n";
        }
        cout << string(wd * 2 + 3, '-') << endl; // Bottom Border
    }

    // Check if a row is full
    bool isRowFull(int row) {
        for (int j = 0; j < wd; j++) {
            if (grid[row][j] == '.') return false; // Empty cell found
        }
        return true;
    }

    // Clear a row & shift everything down
    void clearRow(int row) {
        for (int i = row; i > 0; i--) {
            grid[i] = grid[i - 1];  // Shift rows downward
        }
        grid[0] = vector<char>(wd, '.');  // New empty row
    }

    // Clear all full rows & return count
    int clearFullRows() {
        int cleared = 0;
        for (int i = 0; i < ht; i++) {
            if (isRowFull(i)) {
                clearRow(i);
                cleared++;
            }
        }
        return cleared;
    }

    // Place a test tetromino (simulate a filled row)
    void fillTestRow(int row) {
        for (int j = 0; j < wd; j++) {
            grid[row][j] = '#';
        }
    }
};

// Game Class - Handles Scoring
class Game {
public:
    int score;
    int level;
    int linesCleared;

    Game() {
        score = 0;
        level = 1;
        linesCleared = 0;
    }

    // Update Score based on cleared rows
    void updateScore(int rows) {
        int points[4] = {40, 100, 300, 1200}; // 1, 2, 3, 4-row points
        if (rows > 0) {
            score += points[rows - 1] * level;
            linesCleared += rows;
            if (linesCleared >= level * 5) { // Increase level every 5 rows
                level++;
            }
        }
    }

    // Display Score & Level
    void displayStats() {
        cout << "Score: " << score << "  Level: " << level << "\n";
    }
};

// Function to handle clearing rows & updating score
void handleClearing(Board &board, Game &game) {
    int rowsCleared = board.clearFullRows();
    game.updateScore(rowsCleared);
}

// Main Function - Testing the Line Clearing & Scoring System
int main() {
    Board board;
    Game game;

    // Simulating full rows at the bottom
    board.fillTestRow(ht - 1);
    board.fillTestRow(ht - 2);

    // Display initial state
    board.display();
    game.displayStats();

    // Simulate clearing rows
    cout << "\nClearing full rows...\n";
    handleClearing(board, game);

    // Display after clearing
    board.display();
    game.displayStats();

    return 0;
}