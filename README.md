# Game-Engine
Homework for the course Game-Engine-Foundations (CSC581).

## Name
Platforms (Since it's basically our character jumping on platforms :p)

## Description
Homework for the course Game-engine Foundations. It's currently a game with elements of a game engine such as movement, basic physics, inheritance and reusing, asynchronity and networking(multiplayer)

## Controls
A-move left <br>
D-move right <br>
space-jump <br>
E + resize button on titlebar = constant scaling <br>
Q - Change speed (0.5, 1, 2) <br>
P - Pause Unpause <br>

## How to run
First, download the zip and extract it, then go to the folder and open terminal <br>
After installing the dependencies and required libraries <br>
Enter the following commands: <br>
To start the server:
g++ -c game_server.cpp - <br>
g++ game_server.o -o (name of the output file) -lsfml-graphics -lsfml-system -lsfml-window <br>
./(name of the output file) <br>
To start the client:
g++ -c game.cpp - <br>
g++ game.o -o (name of the output file) -lsfml-graphics -lsfml-system -lsfml-window <br>
./(name of the output file) <br>

## System Specifications
Operating System:
Ubuntu 20.04 (tested)

Should work on windows, mac and other versions of linux (Haven't tested)

## Dependencies
1. x11-apps <br>
2. build-essential <br>
3. libsfml-dev <br>
4. zmq <br>

## Authors and Acknowledgement
Madhur Dixit

## Project status
Ongoing


