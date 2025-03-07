#include <bits/stdc++.h>
using namespace std;
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
    
        void updateScore(int rows) {
            int points[4] = {40, 100, 300, 1200}; // 1, 2, 3, 4 row clear points
            if (rows > 0) {
                score += points[rows - 1] * level;
                linesCleared += rows;
                if (linesCleared >= level * 5) {  // Increase level every 5 clears
                    level++;
                }
            }
        }
    
        void displayStats() {
            cout << "Score: " << score << "  Level: " << level << "\n";
        }
    };
    