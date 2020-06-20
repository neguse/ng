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

enum class ngAlignX {
  LEFT,
  CENTER,
  RIGHT,
};

enum class ngAlignY {
  TOP,
  CENTER,
  BOTTOM,
};

struct ngRect {
  vec2 pos;
  vec2 size;

  float Left() const { return pos.x - size.x; }
  float Right() const { return pos.x + size.x; }
  float Bottom() const { return pos.y - size.y; }
  float Top() const { return pos.y + size.y; }
  void AlignX(float x, ngAlignX align) {
    switch (align) {
      case ngAlignX::LEFT:
        pos.x = x + size.x;
        break;
      case ngAlignX::CENTER:
        pos.x = x;
      case ngAlignX::RIGHT:
        pos.x = x - size.x;
        break;
    }
  }
  void AlignY(float y, ngAlignY align) {
    switch (align) {
      case ngAlignY::TOP:
        pos.y = y - size.y;
        break;
      case ngAlignY::CENTER:
        pos.y = y;
      case ngAlignY::BOTTOM:
        pos.y = y + size.y;
        break;
    }
  }
};

class ngMath {
 public:
  // TRS returns Translate * Rotate * Scale matrix.
  static mat3 TRS(vec2 pos, float rad, vec2 scale);

  static bool IsCollideRect(const ngRect& r1, const ngRect& r2) {
    if (r1.Right() > r2.Left() && r1.Left() < r2.Right()) {
      if (r1.Top() > r2.Bottom() && r1.Bottom() < r2.Top()) {
        return true;
      }
    }
    return false;
  }
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
