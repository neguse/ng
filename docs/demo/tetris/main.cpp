#include "ng.h"

enum {
  KEY_UP,
  KEY_DOWN,
  KEY_RIGHT,
  KEY_LEFT,
  KEY_1,
  KEY_2,
};

const vec2 kWindowSize(320, 240);

const ngColor kBlack(0x00, 0x00, 0x00, 0xff);
const ngColor kWhite(0xff, 0xff, 0xff, 0xff);
const ngColor kLightGray(0xde, 0xde, 0xde, 0xff);
const ngColor kGray(0x80, 0x80, 0x80, 0xff);
const ngColor kLightBlue(0xaf, 0xdf, 0xe4, 0xff);
const ngColor kYellow(0xFF, 0xD4, 0x00, 0xff);
const ngColor kGreen(0x00, 0x80, 0x00, 0xff);
const ngColor kRed(0xED, 0x1A, 0x3D, 0xff);
const ngColor kBlue(0x00, 0x67, 0xC0, 0xff);
const ngColor kOrange(0xf3, 0x98, 0x00, 0xff);
const ngColor kPurple(0xA7, 0x57, 0xA8, 0xff);

const int kTetriminoPatternNum = 8;
const int kTetriminoPatternX = 2;
const int kTetriminoPatternY = 4;
const int kTetriminoPatternS = kTetriminoPatternX * kTetriminoPatternY;
const bool kTetriminoPattern[kTetriminoPatternNum][kTetriminoPatternS] = {
    {0},                       // none
    {1, 0, 1, 0, 1, 0, 1, 0},  // I
    {1, 1, 1, 0, 1, 0, 0, 0},  // L
    {1, 1, 0, 1, 0, 1, 0, 0},  // J
    {1, 1, 1, 1, 0, 0, 0, 0},  // O
    {1, 0, 1, 1, 0, 1, 0, 0},  // S
    {0, 1, 1, 1, 1, 0, 0, 0},  // Z
    {1, 0, 1, 1, 1, 0, 0, 0},  // T
};

enum Cell {
  None,
  Block1,
  Block2,
  Block3,
  Block4,
  Block5,
  Block6,
  Block7,
};

struct MinoSelector : public ngShuffledBuffer<int> {
  void Fill() override {
    for (int i = 1; i < kTetriminoPatternNum; i++) {
      buffer_.push_back(i);
    }
  }
};

const float kCellSize = 20.f;

const ivec2 kBoardSize(10, 24);
struct Board : public ngBoard<Cell> {
  int minoPattern;
  int minoRot;
  ivec2 minoPos;
  ngRand rng;
  MinoSelector selector;

  virtual void Init(ivec2 s, Cell init) {
    rng.Seed(0);
    ngBoard<Cell>::Init(s, init);
    NextMino();
  }

  virtual Cell GetAt(ivec2 pos) override {
    if (IsInside(pos)) {
      return ngBoard<Cell>::GetAt(pos);
    }
    if (pos.y >= size.y) {
      return Cell::None;
    } else {
      return Cell::Block1;
    }
  }

  void DrawBlock(ngProcess& p, ivec2 pos, Cell c) {
    static const ngColor colors[] = {
        kLightGray, kLightBlue, kOrange, kBlue, kYellow, kGreen, kRed, kPurple,
    };
    ngColor col = colors[c];

    p.Rect(col, col,
           {-kWindowSize.x + (pos.x + 1) * kCellSize,
            -kWindowSize.y + (pos.y + 1) * kCellSize},
           {kCellSize / 2 - 1, kCellSize / 2 - 1});
  }

  void Fix() {
    for (int x = 0; x < kTetriminoPatternX; x++) {
      for (int y = 0; y < kTetriminoPatternY; y++) {
        if (kTetriminoPattern[minoPattern][y * kTetriminoPatternX + x]) {
          SetAt(Rotated(ivec2{x, y}, minoRot) + minoPos, Cell(minoPattern));
        }
      }
    }
  }

  bool Move(ivec2 delta) {
    ivec2 nextPos = minoPos + delta;
    if (IsCollision(minoPattern, minoRot, nextPos)) {
      return false;
    }
    minoPos = nextPos;
    return true;
  }
  bool Rot(int delta) {
    int nextRot = (minoRot + delta + 4) % 4;
    if (IsCollision(minoPattern, nextRot, minoPos)) {
      return false;
    }
    minoRot = nextRot;
    return true;
  }

  // return false if gameover.
  bool NextMino() {
    minoPattern = selector.Pop(rng);
    minoRot = 0;
    minoPos = {4, 20};
    return !(IsCollision(minoPattern, minoRot, minoPos));
  }

  bool IsCollision(int pat, int rot, ivec2 pos) {
    for (int x = 0; x < kTetriminoPatternX; x++) {
      for (int y = 0; y < kTetriminoPatternY; y++) {
        if (kTetriminoPattern[pat][y * kTetriminoPatternX + x]) {
          if (GetAt(Rotated(ivec2{x, y}, rot) + pos) != Cell::None) {
            return true;
          }
        }
      }
    }
    return false;
  }

  void EraseLine(int yo) {
    for (int y = yo; y < size.y; y++) {
      for (int x = 0; x < size.x; x++) {
        SetAt({x, y}, GetAt({x, y + 1}));
      }
    }
  }

  int EraseLines() {
    int lines = 0;
    for (int y = 0; y < size.y;) {
      int blocks = 0;
      for (int x = 0; x < size.x; x++) {
        if (GetAt({x, y}) == Cell::None) {
          break;
        }
        blocks++;
      }
      if (blocks == size.x) {
        EraseLine(y);
        lines++;
      } else {
        y++;
      }
    }
    return lines;
  }

  ivec2 Rotated(ivec2 pos, int rot) {
    switch (rot % 4) {
      case 0:
        return pos;
      case 1:
        return {-pos.y, pos.x};
      case 2:
        return -pos;
      case 3:
        return {pos.y, -pos.x};
    }
  }

  void Draw(ngProcess& p) {
    for (int x = 0; x < size.x; x++) {
      for (int y = 0; y < size.y; y++) {
        DrawBlock(p, {x, y}, GetAt({x, y}));
      }
    }

    for (int x = 0; x < kTetriminoPatternX; x++) {
      for (int y = 0; y < kTetriminoPatternY; y++) {
        if (kTetriminoPattern[minoPattern][y * kTetriminoPatternX + x]) {
          DrawBlock(p, Rotated(ivec2{x, y}, minoRot) + minoPos,
                    Cell(minoPattern));
        }
      }
    }
  }
};  // struct Board

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
  float dropTime = 1.f;
  int score = 0;
  bool gameOver = false;
  proc->Run([&](ngProcess& p, float dt) {
    // update
    if (!gameOver) {
      if (proc->IsJustPressed(KEY_LEFT)) {
        b.Move({-1, 0});
      }
      if (proc->IsJustPressed(KEY_RIGHT)) {
        b.Move({1, 0});
      }
      if (proc->IsHold(KEY_DOWN)) {
        if (b.Move({0, -1})) {
          dropTime = 1.f;
        }
      }
      if (proc->IsJustPressed(KEY_1)) {
        b.Rot(1);
      }
      if (proc->IsJustPressed(KEY_2)) {
        b.Rot(-1);
      }

      dropTime -= dt;
      if (dropTime <= 0) {
        dropTime = 1.f;
        if (!b.Move({0, -1})) {
          b.Fix();
          score += b.EraseLines();
          if (!b.NextMino()) {
            gameOver = true;
          }
        }
      }
    }

    // drawing
    p.Clear(kWhite);
    b.Draw(p);

    p.Text(kBlack, {100, 200}, 30.f, fmt::format("score:{0}", score).c_str());
    p.Text(kBlack, {0, 100}, 50.f, u8"ngTRIS");
    p.Text(kBlack, {50, 0}, 30.f, u8"a d key : move");
    p.Text(kBlack, {50, -40}, 30.f, u8"z x key : rotate");
    p.Text(kBlack, {50, -80}, 30.f, u8"s key : drop");

    if (gameOver) {
      p.Text(kBlack, {-kWindowSize.x, 0}, 100.f, u8"GAMEOVER");
    }
  });
}
