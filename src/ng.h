#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtx/matrix_transform_2d.hpp>
using namespace glm;

#include <fmt/format.h>

#include <functional>
#include <memory>

typedef uvec4 ngColor;
typedef glm::vec2 ngCoord;
typedef uint8_t ngKeyCode;

class ngMath {
 public:
  // TRS returns Translate * Rotate * Scale matrix.
  static mat3 TRS(vec2 pos, float rad, vec2 scale);
};

class ngProcess;
typedef std::function<void(ngProcess&, float)> ngUpdater;

class ngProcess {
 public:
  static std::unique_ptr<ngProcess> NewProcess();

  virtual bool Init() = 0;
  virtual void Run(ngUpdater updater) = 0;
  virtual void ExitLoop() = 0;

  // rendering methods

  virtual void Push(const mat3& mat) = 0;
  virtual void Pop() = 0;

  virtual void Clear(const ngColor& col) = 0;
  virtual void Rect(const ngColor& border, const ngColor& fill,
                    const vec2& center, const vec2& size) = 0;
  virtual void Square(const ngColor& border, const ngColor& fill,
                      const vec2& center, const float size) = 0;
  virtual void Line(const ngColor& col, const vec2& pos1, const vec2& pos2) = 0;
  virtual void Circle(const ngColor& border, const ngColor& fill,
                      const ngCoord& center, const float& length) = 0;
  virtual void Text(const ngColor& col, const ngCoord& pos, float length,
                    const char* str) = 0;

  // input methods

  virtual void MapKeyboard(char key, ngKeyCode code) = 0;
  virtual void MapMouseButton(uint8_t num, ngKeyCode code) = 0;
  // virtual void MapPadButton(uint8_t padIndex, uint8_t buttonIndex,
  //                          ngKeyCode code) = 0;

  // IsHold returns true while key is pressed.
  virtual bool IsHold(ngKeyCode code) = 0;
  // IsKeyPress returns true only in one frame when key is pressed.
  virtual bool IsJustPressed(ngKeyCode code) = 0;

  virtual ngCoord CursorPos() = 0;
};
