# Space Invaders Game Controls

- **A:** Move Left (Triggers modifying object modify_color script)
- **D:** Move Right (Triggers modifying object modify_color script)
- **Space:** Shoot Bullets
- **Raise Event:** Demonstrated when enemies collide with the ground (raise event script runs), character death event raised
- **Every Key Input:** Triggers Raise Event, which is handled by script (key count increases in handle event)

# How to Run

1. Navigate to the SpaceInvaders directory:
    ```bash
    cd SpaceInvaders
    ```

2. Build the game:
    ```bash
    make
    ```

3. Run the game:
    ```bash
    ./main
    ```

Now you have the Space Invaders game running, and you can interact with the game using the specified controls.

**Note:** Ensure that you have the necessary dependencies installed and configured on your system before building and running the game.

