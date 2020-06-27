#pragma once

#include <stdio.h>

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtx/matrix_transform_2d.hpp>
using namespace glm;

#include <fmt/format.h>

#include <functional>
#include <memory>

#include "splitmix64.h"
#include "xoshiro256plusplus.h"

typedef uvec4 ngColor;
typedef glm::vec2 ngCoord;
typedef uint8_t ngKeyCode;

#define NG_VERIFY(x)                                                   \
  if (!x) {                                                            \
    printf("failed NG_VERIFY %s at %s(%d)\n", #x, __FILE__, __LINE__); \
  }

#define NG_ASSERT(x)                                                   \
  if (!x) {                                                            \
    printf("failed NG_ASSERT %s at %s(%d)\n", #x, __FILE__, __LINE__); \
    std::abort();                                                      \
  }

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

struct ngRand {
 public:
  ngRand();
  void Seed(uint64_t seed);
  uint64_t UInt64();
  int Int();
  // [0..n)
  int IntN(int n);
  // [min..max)
  int Int1D(ivec2 minmax);
  // [min..max)
  ivec2 Int2D(ivec2 min, ivec2 max);
  // [0..1)
  float Float();
  // [0..n)
  float FloatN(float n);
  // [min..max)
  float Float1D(vec2 minmax);
  // [min..max)
  vec2 Float2D(vec2 min, vec2 max);

 private:
  splitmix64 smix64_;
  xoshiro256plusplus xo_;
};

template <typename T>
struct ngShuffledBuffer {
 protected:
  std::vector<T> buffer_;

 public:
  virtual void Fill() = 0;
  void Shuffle(ngRand& rng) {
    for (int i = buffer_.size() - 1; i > 0; i--) {
      int n = rng.IntN(i);
      // swap buffer_[i] and buffer_[n]
      T tmp = buffer_[i];
      buffer_[i] = buffer_[n];
      buffer_[n] = tmp;
    }
  }
  bool Empty() const { return buffer_.empty(); }
  T Pop(ngRand& rng) {
    if (Empty()) {
      Fill();
      NG_ASSERT(!Empty());
      Shuffle(rng);
    }
    T v = buffer_.back();
    buffer_.resize(buffer_.size() - 1);
    return v;
  }
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

template <typename Cell>
struct ngBoard {
  ivec2 size;
  std::vector<Cell> cells;
  virtual void Init(ivec2 s, Cell init) {
    size = s;
    cells.resize(s.x * s.y, init);
  }

  virtual Cell GetAt(ivec2 pos) {
    NG_ASSERT(IsInside(pos));
    return cells[pos.x + pos.y * size.x];
  }

  virtual bool SetAt(ivec2 pos, const Cell& c) {
    NG_ASSERT(IsInside(pos));
    if (0 <= pos.x && pos.x < size.x) {
      if (0 <= pos.y && pos.y < size.y) {
        cells[pos.x + pos.y * size.x] = c;
        return true;
      }
    }
    return false;
  };

  virtual bool IsInside(ivec2 pos) {
    return (0 <= pos.x && pos.x < size.x) && (0 <= pos.y && pos.y < size.y);
  }
};  // struct ngBoard<Cell>

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
