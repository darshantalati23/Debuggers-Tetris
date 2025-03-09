# The Tetris Game
## - By _The Debuggers_

### Team Members
#### - Aarohi Mehta *(202401002)*
#### - Alvita Thakor *(202401012)*
#### - Darshan Talati *(202401046)*
#### - Dharmesh Upadhyay *(202401049)*


### Table of Contents
- [Description & Features](#-description--features)
- [Usage](#️-usage)
  - [Installation & Running](#installation--running-ubuntu)
  - [How to Play](#how-to-play)
- [Contributors](#-contributors)
- [License](#-license)
- [Credits](#-credits)


### ✨ Description & Features
A terminal-based implementation of the classic Tetris game. The falling tetromino pieces must be arranged such that no cell in a row remains empty!

It has the following features:
- **🎮 Classic Tetris Gameplay**: Rotate, move, and drop blocks to complete lines and score points.
- **🚀 Adaptive Difficulty**: Speed increases as your score grows, making the game progressively challenging.
- **📊 Scoring System**: Earn points based on the number of lines cleared at once:
  - **1 Line**: +100 points
  - **2 Lines**: +300 points
  - **3 Lines**: +500 points
  - **4 Lines (Tetris!)**: +800 points
- **🎨 Customizable Controls**: Move and rotate blocks using configurable keys.
- **⏸ Pause/Resume**: Take a break whenever needed.
- **🔄 Restart Option**: Restart the game after a game over without closing the terminal.



### 🕹️ Usage

#### Installation & Running (Ubuntu)
  - Install the required compiler (if not installed):
    <pre>sudo apt install g++</pre>
  - Install "SoX" (Sound eXchange) software (if not installed)
    <pre>sudo apt install sox</pre>
  - Compile and Run the Game:
    *Single Player*
    <pre>tetris.cpp -o play
    ./play</pre>
    **OR**
    *1 v/s 1 (Multiplayer)*
    <pre>tetrisX2.cpp -o play
    ./play</pre>

#### How to Play

| Action        | Player 1| Player 2  |
|---------------|---------|-----------|
| **Move Left** | `A`     | `⬅ Left`  |
| **Move Right**| `D`     | `➡ Right` |
| **Rotate**    | `W`     | `⬆ Up`    |
| **Soft Drop** | `S`     | `⬇ Down`  |
| **Hard Drop** | `Space` | `Enter`   |
| **Pause**     | `P`     | `P`       |
| **Restart**   | `R`     | `R`       |



### 🤝 Contributors
The game was built and improved with contributions from:

*Aarohi & Alvita*: Implemented the tetris pieces and gameboard.<br/>
*Darshan & Dharmesh*: Implemented the piece-falling and scoring logic.<br/>
**All** worked on gameplay updates, optimization and multiplayer.


### 📜 License
This project is not licensed. All rights reserved.


### 👥 Credits
The game's design was inspired by the youtube tutorials:
- [Snake Game by Ertjan Arapi](https://youtu.be/gWq0tJLsjRs?si=GyGW5fCuE8hOVNki)
- [Snake Game by NVitanovic](https://www.youtube.com/watch?v=E_-lMZDi7Uw)

Thanks to ChatGPT & DeepSeek for providing helpful recommedations, resources and tips for making the basic idea of the game.


### 😉 Group Icon
<img src="GroupIcon.jpg" width="350px">