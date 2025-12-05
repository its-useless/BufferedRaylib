#include "BufferedRaylib.hpp"

#include "raymath.h"

#include <array>
#include <cassert>
#include <concepts>
#include <map>
#include <set>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#define MAX_GAMEPADS 16

namespace raylib {

  bool Button::operator<(const Button& o) const {
    if (type != o.type)
      return type < o.type;
    if (type == Type::Gamepad && gamepad.id == o.gamepad.id)
      return gamepad.button < o.gamepad.button;
    return keyboard < o.keyboard; // They should all be castable to a keyboard
                                  // key, thus comparing only it should be fine!
  }

  bool Button::IsPressed(const Button& button) {
    switch (button.type) {
      break;
    case Button::Type::Keyboard:
      return IsKeyDown(button.keyboard);
      break;
    case Button::Type::Mouse:
      return IsMouseButtonDown(button.mouse);
      break;
    case Button::Type::Gamepad: {
      return IsGamepadButtonDown(button.gamepad.id, button.gamepad.button);
    } break;
    default:
      assert(button.type != Button::Type::Invalid);
    }
    return false;
  }

  uint8_t Button::IsSetPressed(const std::set<Button>& buttons) {
    uint8_t state = 0;
    for (auto& button : buttons)
      state += IsPressed(button);
    return state;
  }

  Action& Action::operator=(Action&& o) noexcept {
    type = o.type;
    data = std::move(o.data);
    keyboardCallback = std::exchange(o.keyboardCallback, nullptr);
    mouseButtonCallback = std::exchange(o.mouseButtonCallback, nullptr);
    gamepadButtonCallback = std::exchange(o.gamepadButtonCallback, nullptr);
    mouseWheelCallback = std::exchange(o.mouseWheelCallback, nullptr);
    gamepadAxisCallback = std::exchange(o.gamepadAxisCallback, nullptr);
    vectorCallback = std::exchange(o.vectorCallback, nullptr);
    if (o.type == Type::Button)
      data.button.buttons = std::exchange(o.data.button.buttons, nullptr);
    else if (o.type == Type::MultiButton)
      data.multi.quadButtons = std::exchange(o.data.multi.quadButtons, nullptr);
    return *this;
  }

  void Action::PumpButton(std::string_view name) {
    assert(data.button.buttons);
    uint8_t state = Button::IsSetPressed(*data.button.buttons);
    bool isDown = state > 0;

    if (state != data.button.last_state) {
      if (data.button.combo) {
        bool comboState = state == data.button.buttons->size();
        bool lastComboState =
            data.button.last_state == data.button.buttons->size();
        if (comboState != lastComboState) {
          // Invoke callbacks for each button in the set
          for (const auto& button : *data.button.buttons) {
            if (button.type == Button::Type::Keyboard && keyboardCallback) {
              keyboardCallback(button.keyboard, comboState);
            } else if (button.type == Button::Type::Mouse && mouseButtonCallback) {
              mouseButtonCallback(button.mouse, comboState);
            } else if (button.type == Button::Type::Gamepad && gamepadButtonCallback) {
              gamepadButtonCallback(button.gamepad.id, button.gamepad.button, comboState);
            }
          }
        }
      } else {
        // Invoke callbacks for each button in the set
        for (const auto& button : *data.button.buttons) {
          if (button.type == Button::Type::Keyboard && keyboardCallback) {
            keyboardCallback(button.keyboard, isDown);
          } else if (button.type == Button::Type::Mouse && mouseButtonCallback) {
            mouseButtonCallback(button.mouse, isDown);
          } else if (button.type == Button::Type::Gamepad && gamepadButtonCallback) {
            gamepadButtonCallback(button.gamepad.id, button.gamepad.button, isDown);
          }
        }
      }
      data.button.last_state = state;
    }
  }

  void Action::PumpAxis(std::string_view name) {
    float state = data.axis.last_state;
    switch (data.axis.type) {
      break;
    case Data::Axis::Type::Gamepad: {
      float movement =
          GetGamepadAxisMovement(data.axis.gamepad.id, data.axis.gamepad.axis);
      if (movement != 0)
        state += movement;
      if (state != data.axis.last_state) {
        float delta = state - data.axis.last_state;
        if (gamepadAxisCallback) {
          gamepadAxisCallback(data.axis.gamepad.id, data.axis.gamepad.axis, state, delta);
        }
        data.axis.last_state = state;
      }
      return;
    } break;
    case Data::Axis::Type::MouseWheel: {
      float movement = GetMouseWheelMove();
      if (movement != 0)
        state += movement;
      if (state != data.axis.last_state) {
        float delta = state - data.axis.last_state;
        if (mouseWheelCallback) {
          mouseWheelCallback(state, delta);
        }
        data.axis.last_state = state;
      }
      return;
    } break;
    default:
      assert(data.axis.type != Data::Axis::Type::Invalid);
    }
  }

  void Action::PumpVector(std::string_view name) {
    Vector2 state = data.vector.last_state;
    switch (data.vector.type) {
      break;
    case Data::Vector::Type::MouseWheel:
      state = GetMouseWheelMoveV();
      break;
    case Data::Vector::Type::MousePosition:
      state = GetMousePosition();
      // break; case Type::MouseDelta:
      //     state = GetMouseDelta();
      break;
    case Data::Vector::Type::GamepadAxes: {
      state.x += GetGamepadAxisMovement(data.vector.gamepad.horizontal.id,
                                        data.vector.gamepad.horizontal.axis);
      state.y += GetGamepadAxisMovement(data.vector.gamepad.vertical.id,
                                        data.vector.gamepad.vertical.axis);
    } break;
    default:
      assert(data.vector.type != Data::Vector::Type::Invalid);
    }
    if (!Vector2Equals(state, data.vector.last_state)) {
      Vector2 delta = Vector2Subtract(state, data.vector.last_state);
      if (vectorCallback) {
        vectorCallback(state, delta);
      }
      data.vector.last_state = state;
    }
  }

  Action Action::gamepad_axes(GamepadAxis horizontal /*= GAMEPAD_AXIS_LEFT_X*/,
                              GamepadAxis vertical /*= GAMEPAD_AXIS_LEFT_Y*/,
                              int gamepadHorizontal /*= 0*/,
                              int gamepadVertical /*= -1*/) {
    if (gamepadVertical < 0)
      gamepadVertical = gamepadHorizontal;

    Action out = {Action::Type::Vector,
                  {.vector = {Data::Vector::Type::GamepadAxes}}};
    out.data.vector.gamepad = {{gamepadHorizontal, horizontal},
                               {gamepadVertical, vertical}};
    return out;
  }

  void Action::PumpMultiButton(std::string_view name) {
    Vector2 state = data.multi.last_state;
    {
      auto type = data.multi.type;
      std::array<uint8_t, 4> buttonState = {};
      for (uint8_t i = 0;
           i < (type == Data::MultiButton::Type::QuadButtons ? 4 : 2); i++) {
        buttonState[i] =
            Button::IsSetPressed(data.multi.quadButtons->directions[i]);
        if (data.multi.quadButtons->normalize && buttonState[i] > 0)
          buttonState[i] = 1;
      }
      if (type == Data::MultiButton::Type::QuadButtons)
        state.x = (float)buttonState[MultiButtonData<4>::Direction::Left] -
                  (float)buttonState[MultiButtonData<4>::Direction::Right];
      state.y = (float)buttonState[MultiButtonData<4>::Direction::Up] -
                (float)buttonState[MultiButtonData<4>::Direction::Down];

      if (type == Data::MultiButton::Type::ButtonPair)
        state.x = state.y;
    }
    if (!Vector2Equals(state, data.multi.last_state)) {
      Vector2 delta = Vector2Subtract(state, data.multi.last_state);
      if (vectorCallback) {
        vectorCallback(state, delta);
      }
      data.multi.last_state = state;
    }
  }

  void Action::PollEvents(std::string_view name) {
    switch (type) {
      break;
    case Action::Type::Button:
      PumpButton(name);
      break;
    case Action::Type::Axis:
      PumpAxis(name);
      break;
    case Action::Type::Vector:
      PumpVector(name);
      break;
    case Action::Type::MultiButton:
      PumpMultiButton(name);
      break;
    default:
      assert(type != Action::Type::Invalid);
    }
  }

  void BufferedInput::PollEvents(bool whileUnfocused /*= false*/) {
    if (!whileUnfocused && !IsWindowFocused())
      return;

    // Poll keyboard keys
    if (keyboard_callback) {
      for (int key = KEY_NULL; key <= KEY_KP_EQUAL; key++) {
        bool isDown = IsKeyDown((KeyboardKey)key);
        auto it = keyboard_states.find((KeyboardKey)key);
        if (it == keyboard_states.end() || it->second != isDown) {
          keyboard_states[(KeyboardKey)key] = isDown;
          keyboard_callback((KeyboardKey)key, isDown);
        }
      }
    }

    // Poll mouse buttons (check all possible mouse buttons)
    if (mouse_button_callback) {
      for (int button = MOUSE_BUTTON_LEFT; button <= MOUSE_BUTTON_EXTRA; button++) {
        bool isDown = IsMouseButtonDown((MouseButton)button);
        auto it = mouse_button_states.find((MouseButton)button);
        if (it == mouse_button_states.end() || it->second != isDown) {
          mouse_button_states[(MouseButton)button] = isDown;
          mouse_button_callback((MouseButton)button, isDown);
        }
      }
    }

    // Poll gamepad buttons
    if (gamepad_button_callback) {
      for (int gamepad = 0; gamepad < MAX_GAMEPADS; gamepad++) {
        if (!IsGamepadAvailable(gamepad))
          continue;
        for (int button = GAMEPAD_BUTTON_UNKNOWN; button <= GAMEPAD_BUTTON_RIGHT_THUMB; button++) {
          bool isDown = IsGamepadButtonDown(gamepad, (GamepadButton)button);
          auto key = std::make_pair(gamepad, (GamepadButton)button);
          auto it = gamepad_button_states.find(key);
          if (it == gamepad_button_states.end() || it->second != isDown) {
            gamepad_button_states[key] = isDown;
            gamepad_button_callback(gamepad, (GamepadButton)button, isDown);
          }
        }
      }
    }

    // Poll mouse wheel
    if (mouse_wheel_callback) {
      float movement = GetMouseWheelMove();
      if (movement != 0) {
        mouse_wheel_value += movement;
        float delta = movement;
        mouse_wheel_callback(mouse_wheel_value, delta);
      }
    }

    // Poll gamepad axes
    if (gamepad_axis_callback) {
      for (int gamepad = 0; gamepad < MAX_GAMEPADS; gamepad++) {
        if (!IsGamepadAvailable(gamepad))
          continue;
        for (int axis = GAMEPAD_AXIS_LEFT_X; axis <= GAMEPAD_AXIS_RIGHT_TRIGGER; axis++) {
          float movement = GetGamepadAxisMovement(gamepad, (GamepadAxis)axis);
          if (movement != 0) {
            auto key = std::make_pair(gamepad, (GamepadAxis)axis);
            auto it = gamepad_axis_states.find(key);
            float lastValue = (it != gamepad_axis_states.end()) ? it->second : 0.0f;
            float newValue = lastValue + movement;
            float delta = movement;
            gamepad_axis_states[key] = newValue;
            gamepad_axis_callback(gamepad, (GamepadAxis)axis, newValue, delta);
          }
        }
      }
    }

    // Poll mouse position
    if (mouse_position_callback) {
      Vector2 newPos = GetMousePosition();
      if (!Vector2Equals(newPos, mouse_position)) {
        Vector2 delta = Vector2Subtract(newPos, mouse_position);
        mouse_position = newPos;
        mouse_position_callback(newPos, delta);
      }
    }
  }
} // namespace raylib
