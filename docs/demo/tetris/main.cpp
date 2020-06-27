#include "ng.h"

enum {
  KEY_UP,
  KEY_DOWN,
  KEY_RIGHT,
  KEY_LEFT,
  KEY_1,
  KEY_2,
};

// (320, 240) ~ (-320, -240)
static const vec2 kWindowSize = {320, 240};

static const vec2 kCellSize = {18, 18};
static const ivec2 kBoardSize = {10, 20};

ngColor kBlack(0x00, 0x00, 0x00, 0xff);
ngColor kWhite(0xff, 0xff, 0xff, 0xff);
ngColor kGray(0x80, 0x80, 0x80, 0xff);

enum class Cell {
  None,
  Block,  // TODO Color
};

// 67
// 45
// 23
// 01

const int kTetriminoTypes = 7;

const ivec2 kTetriminoXY = {2, 4};
const int kTetriminoS = 8;
const bool kTetrimino[kTetriminoTypes][kTetriminoS] = {
    {1, 0, 1, 0, 1, 0, 1, 0},  // I
    {1, 1, 1, 1, 0, 0, 0, 0},  // O
    {0, 1, 1, 1, 1, 0, 0, 0},  // S
    {1, 0, 1, 1, 0, 1, 0, 0},  // Z
    {1, 1, 0, 1, 0, 1, 0, 0},  // J
    {1, 1, 1, 0, 1, 0, 0, 0},  // L
    {0, 1, 1, 1, 0, 1, 0, 0},  // T
};

struct Board : public ngBoard<Cell> {
  int minoType;
  int minoRot;
  ivec2 minoPos;

  virtual void Init(ivec2 s, Cell c) override {
    ngBoard<Cell>::Init(s, c);
    NextMino();
  }

  virtual Cell GetAt(ivec2 pos) override {
    if (IsInside(pos)) {
      return ngBoard<Cell>::GetAt(pos);
    } else if (pos.y >= kBoardSize.y) {
      return Cell::None;
    } else {
      return Cell::Block;
    }
  }

  void RenderCell(ngProcess& p, ivec2 pos, Cell c) {
    ngColor col;
    switch (c) {
      case Cell::Block:
        col = kBlack;
        break;
      case Cell::None:
        col = kGray;
    }

    p.Rect(col, col, vec2{pos.x * (kCellSize.x + 2), pos.y * (kCellSize.y + 2)},
           kCellSize * 0.5f);
  }

  bool NextMino() {
    minoPos = ivec2{kBoardSize.x / 2, kBoardSize.y - kTetriminoXY.y};
    minoRot = 0;
    minoType = rand() % kTetriminoTypes;

    if (MinoCollision()) {
      return false;
    }
    return true;
  }

  void EraseLine(int yo) {
    for (int y = yo; y < kBoardSize.y; y++) {
      for (int x = 0; x < kBoardSize.x; x++) {
        SetAt({x, y}, GetAt({x, y + 1}));
      }
    }
  }
  int Erase() {
    int lines = 0;
    for (int y = 0; y < kBoardSize.y;) {
      int n = 0;
      for (int x = 0; x < kBoardSize.x; x++) {
        if (GetAt({x, y}) == Cell::None) {
          break;
        }
        n++;
      }
      if (n == kBoardSize.x) {
        EraseLine(y);
        lines++;
      } else {
        y++;
      }
    }
    return lines;
  }
  void FixMino() {
    for (int x = 0; x < kTetriminoXY.x; x++) {
      for (int y = 0; y < kTetriminoXY.y; y++) {
        int index = x + y * kTetriminoXY.x;
        if (kTetrimino[minoType][index]) {
          ivec2 pos = minoPos + MinoRotated(ivec2({x, y}));
          SetAt(pos, Cell::Block);
        }
      }
    }
  }

  bool MinoCollision() {
    for (int x = 0; x < kTetriminoXY.x; x++) {
      for (int y = 0; y < kTetriminoXY.y; y++) {
        int index = x + y * kTetriminoXY.x;
        if (kTetrimino[minoType][index]) {
          ivec2 pos = minoPos + MinoRotated(ivec2({x, y}));
          if (GetAt(pos) == Cell::Block) return true;
        }
      }
    }
    return false;
  }

  bool MoveMino(ivec2 dpos) {
    ivec2 tmpPos = minoPos;
    minoPos += dpos;
    if (MinoCollision()) {
      minoPos = tmpPos;
      return false;
    }
    return true;
  }

  bool RotMino(int r) {
    int tmpRot = minoRot;
    minoRot = (minoRot + r + 4) % 4;
    if (MinoCollision()) {
      minoRot = tmpRot;
      return false;
    }
    return true;
  }

  ivec2 MinoRotated(ivec2 pos) {
    ivec2 offset = {1, 1};
    pos -= offset;
    switch (minoRot % 4) {
      case 0:
        return pos + offset;
      case 1:
        return ivec2{-pos.y, pos.x} + offset;
      case 2:
        return ivec2{-pos.x, -pos.y} + offset;
      case 3:
        return ivec2{pos.y, -pos.x} + offset;
      default:
        abort();
    }
  }

  void Render(ngProcess& p) {
    p.Push(translate(mat3(1.f), {-kWindowSize.x + 30, -kWindowSize.y + 30}));
    // render board
    for (int x = 0; x < kBoardSize.x; x++) {
      for (int y = 0; y < kBoardSize.y; y++) {
        RenderCell(p, {x, y}, GetAt({x, y}));
      }
    }

    // render mino
    for (int x = 0; x < kTetriminoXY.x; x++) {
      for (int y = 0; y < kTetriminoXY.y; y++) {
        int index = x + y * kTetriminoXY.x;
        if (kTetrimino[minoType][index]) {
          ivec2 pos = minoPos + MinoRotated(ivec2({x, y}));
          RenderCell(p, pos, Cell::Block);
        }
      }
    }
    p.Pop();
  }
};

int main(void) {
  auto proc = ngProcess::NewProcess();
  proc->MapKeyboard('w', KEY_UP);
  proc->MapKeyboard('a', KEY_LEFT);
  proc->MapKeyboard('s', KEY_DOWN);
  proc->MapKeyboard('d', KEY_RIGHT);
  proc->MapKeyboard('z', KEY_1);
  proc->MapKeyboard('x', KEY_2);
  proc->MapMouseButton(1, KEY_1);
  proc->MapMouseButton(3, KEY_2);  // right mouse button

  if (!proc->Init()) {
    return 1;
  }

  Board b;
  b.Init(kBoardSize, Cell::None);
  const float kTimeToDrop = 0.8;
  float timeToDrop = kTimeToDrop;
  int scores = 0;
  bool drop = false;
  bool gameover = false;

  proc->Run([&](ngProcess& p, float dt) {
    // update
    p.Clear(kWhite);

    if (!gameover) {
      if (p.IsJustPressed(KEY_LEFT)) {
        b.MoveMino({-1, 0});
      }
      if (p.IsJustPressed(KEY_RIGHT)) {
        b.MoveMino({1, 0});
      }
      if (p.IsJustPressed(KEY_1)) {
        b.RotMino(1);
      }
      if (p.IsJustPressed(KEY_2)) {
        b.RotMino(-1);
      }
      if (p.IsJustPressed(KEY_DOWN)) {
        drop = true;
      }

      timeToDrop -= dt;
      if (timeToDrop < 0 || drop) {
        if (!b.MoveMino({0, -1})) {
          b.FixMino();

          int lines = b.Erase();
          scores += lines;

          if (!b.NextMino()) {
            gameover = true;
          }
          drop = false;
        }
        timeToDrop = kTimeToDrop;
      }
    }

    b.Render(p);
    p.Text(kBlack, {150, 200}, 30, fmt::format(u8"score:{0}", scores).c_str());

    p.Text(kBlack, {0, 100}, 50, u8"ngTRIS");
    p.Text(kBlack, {30, 0}, 30, u8"A or D key: move");
    p.Text(kBlack, {30, -30}, 30, u8"Z or X key : rotate");
    p.Text(kBlack, {30, -60}, 30, u8"S key: drop");
  });
}
