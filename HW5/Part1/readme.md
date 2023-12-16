# Game Controls

- **A:** Move Left (Triggers modifying object modify_color script)
- **D:** Move Right
- **Space:** Jump
- **E:** Resize
- **P:** Pause
- **R:** Replay
- **Q:** Change Game Speed (0.5x, 1x, 1.5x)
- **Chord (A,D):** Character Dies (Triggers Raise Event)
- **Handle Event:** Character death count increases on death event.

# How to Run

## Start the Server

1. Navigate to the server directory:
    ```bash
    cd server
    ```

2. Compile the server code:
    ```bash
    g++ -c game_server.cpp -lzmq
    ```

3. Link and create the server executable:
    ```bash
    g++ game_server.o -o game_server -lzmq -pthread -lsfml-system -lsfml-graphics -lsfml-window
    ```

4. Run the server:
    ```bash
    ./game_server
    ```

## Start the Client

1. Navigate to the client directory:
    ```bash
    cd client
    ```

2. Build the client:
    ```bash
    make
    ```

3. Run the client:
    ```bash
    ./main
    ```

Now you have both the server and client running, and you can interact with the game using the specified controls.

**Note:** Ensure that you have the necessary dependencies installed and configured on your system before running the server and client.

