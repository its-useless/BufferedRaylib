#include "BufferedRaylib.hpp"
#include "raylib.h"
#include "raymath.h"

#include <iostream>

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void) {
  // Initialization
  //--------------------------------------------------------------------------------------
  const int screenWidth = 800;
  const int screenHeight = 450;
  const int fps = 60;

  InitWindow(screenWidth, screenHeight, "Buffered Raylib Example");

  Vector2 ballPosition = {(float)screenWidth / 2, (float)screenHeight / 2};

  SetTargetFPS(fps);
  // =====================================================

  raylib::BufferedInput input;

  bool keyW = false, keyA = false, keyS = false, keyD = false;
  bool keyUp = false, keyDown = false, keyLeft = false, keyRight = false;
  Vector2 dir = {0, 0};

  input.keyboard_callback = [&](KeyboardKey key, bool isDown) {
    if (key == KEY_W)
      keyW = isDown;
    else if (key == KEY_A)
      keyA = isDown;
    else if (key == KEY_S)
      keyS = isDown;
    else if (key == KEY_D)
      keyD = isDown;
    else if (key == KEY_UP)
      keyUp = isDown;
    else if (key == KEY_DOWN)
      keyDown = isDown;
    else if (key == KEY_LEFT)
      keyLeft = isDown;
    else if (key == KEY_RIGHT)
      keyRight = isDown;

    dir = {0, 0};
    if (keyW || keyUp)
      dir.y += 1;
    if (keyS || keyDown)
      dir.y -= 1;
    if (keyA || keyLeft)
      dir.x += 1;
    if (keyD || keyRight)
      dir.x -= 1;

    if (dir.x != 0 || dir.y != 0) {
      dir = Vector2Normalize(dir);
      std::cout << "Movement: {" << dir.x << ", " << dir.y << "}" << std::endl;
    }
  };

  input.mouse_position_callback = [](Vector2 pos, Vector2 delta) {
    auto rlDelta = GetMouseDelta();
    std::cout << "{" << pos.x << ", " << pos.y << "} - {" << delta.x << ", "
              << delta.y << "} - {" << rlDelta.x << ", " << rlDelta.y << "}"
              << std::endl;
  };

  input.mouse_button_callback = [](MouseButton button, bool isDown) {
    std::cout << "Bang! Mouse button " << button
              << (isDown ? " pressed!" : " released!") << std::endl;
  };

  input.mouse_wheel_callback = [](float value, float delta) {
    std::cout << "Mouse wheel: value=" << value << ", delta=" << delta
              << std::endl;
  };

  input.gamepad_button_callback = [](int gamepadId, GamepadButton button,
                                     bool isDown) {
    std::cout << "gamepad " << gamepadId << " " << button << " " << isDown
              << std::endl;
  };

  input.gamepad_axis_callback = [](int gamepadId, GamepadAxis axis, float value,
                                   float delta) {
    // std::cout << "(axis) gamepad " << gamepadId << " " << axis << " " <<
    // value
    //<< " " << delta << std::endl;
  };

  // Main game loop
  while (!WindowShouldClose()) { // Detect window close button or ESC key
    // Processing and callback invocation occur whenever messages are pumped
    input.PollEvents();

    // Draw
    //----------------------------------------------------------------------------------
    BeginDrawing();
    {
      ClearBackground(RAYWHITE);

      DrawText("move the ball with arrow keys", 10, 10, 20, DARKGRAY);

      DrawCircleV(ballPosition, 50, MAROON);

      ballPosition = Vector2Add(Vector2Scale(dir, -10), ballPosition);
    }
    EndDrawing();
    //----------------------------------------------------------------------------------
  }

  // De-Initialization
  //--------------------------------------------------------------------------------------
  CloseWindow(); // Close window and OpenGL context
  //--------------------------------------------------------------------------------------

  return 0;
}
