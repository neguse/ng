#include "ng.h"

enum {
  KEY_UP,
  KEY_DOWN,
  KEY_RIGHT,
  KEY_LEFT,
  KEY_1,
  KEY_2,
};

struct Rect {
  vec2 pos;
  vec2 size;

  float Left() const { return pos.x - size.x; }
  float Right() const { return pos.x + size.x; }
  float Bottom() const { return pos.y - size.y; }
  float Top() const { return pos.y + size.y; }
};

static bool IsCollideRect(const Rect& r1, const Rect& r2) {
  if (r1.Right() > r2.Left() && r1.Left() < r2.Right()) {
    if (r1.Top() > r2.Bottom() && r1.Bottom() < r2.Top()) {
      return true;
    }
  }
  return false;
}

// (320, 240) ~ (-320, -240)
static const vec2 WindowSize = {320, 240};

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

  std::vector<Rect> blocks;
  for (int x = 0; x < 8; x++) {
    for (int y = 0; y < 4; y++) {
      blocks.push_back(
          Rect{{x * (WindowSize.x / 4) - WindowSize.x + 40, y * 60}, {20, 10}});
    }
  }

  enum class State {
    Title,
    BeforeShoot,
    AfterShoot,
    GameOver,
  };

  State state = State::Title;

  Rect mybar{{0, -200}, {100, 30}};
  Rect myball{{0, 0}, {10, 10}};
  vec2 myballVelo{240, 240};
  int balls = 3;
  int score = 0;

  proc->Run([&](ngProcess& p, float dt) {
    // update

    // move mybar
    const float velocity = 300.f;
    if (p.IsHold(KEY_LEFT)) {
      mybar.pos.x -= velocity * dt;
    }
    if (p.IsHold(KEY_RIGHT)) {
      mybar.pos.x += velocity * dt;
    }
    if (mybar.Left() < -320) {
      mybar.pos.x = -320 + mybar.size.x;
    }
    if (mybar.Right() > 320.f) {
      mybar.pos.x = 320 - mybar.size.x;
    }

    // move myball
    if (state == State::BeforeShoot || state == State::Title) {
      myball.pos = mybar.pos + vec2{0, 50};
    } else if (state == State::AfterShoot) {
      myball.pos += myballVelo * dt;
      // reflect myball
      if (myball.Right() > WindowSize.x) {
        myball.pos.x = WindowSize.x - myball.size.x;
        myballVelo.x = -myballVelo.x;
      }
      if (myball.Left() < -WindowSize.x) {
        myball.pos.x = -WindowSize.x + myball.size.x;
        myballVelo.x = -myballVelo.x;
      }
      if (myball.Top() > WindowSize.y) {
        myball.pos.y = WindowSize.y - myball.size.y;
        myballVelo.y = -myballVelo.y;
      }
      if (myball.Bottom() < -WindowSize.y) {
        if (balls > 0) {
          balls--;
          state = State::BeforeShoot;
        } else {
          state = State::GameOver;
        }
      }
      if (IsCollideRect(myball, mybar) && myballVelo.y < 0) {
        myballVelo.y = abs(myballVelo.y);
      }
    }

    // shoot
    if (state == State::BeforeShoot && p.IsJustPressed(KEY_1)) {
      state = State::AfterShoot;
    }
    // title
    if (state == State::Title && p.IsJustPressed(KEY_1)) {
      state = State::BeforeShoot;
    }

    // collide with myball & blocks
    int i = 0;
    while (i < blocks.size()) {
      const Rect& r = blocks[i];
      if (IsCollideRect(myball, r)) {
        vec2 diffPos = myball.pos - r.pos;
        vec2 sizeSum = myball.size + r.size;
        vec2 diffPosAbs = {abs(diffPos.x) / sizeSum.x,
                           abs(diffPos.y) / sizeSum.y};
        if (diffPosAbs.x == diffPosAbs.y) {  // TODO 閾値をきめて比較
          myballVelo.x = -myballVelo.x;
          myballVelo.y = -myballVelo.y;
        } else if (diffPosAbs.x < diffPosAbs.y) {
          myballVelo.y = -myballVelo.y;
        } else {
          myballVelo.x = -myballVelo.x;
        }

        blocks.erase(blocks.begin() + i);
        score += 100;
      } else {
        i++;
      }
    }

    // drawing
    p.Clear({0x80, 0x80, 0x80, 0xff});
    const auto white = ngColor(0xff, 0xff, 0xff, 0xff);
    const auto yellow = ngColor(0xff, 0xff, 0x00, 0xff);
    const auto black = ngColor(0x00, 0x00, 0x00, 0xff);

    switch (state) {
      case State::Title:
        p.Text(black, {-200, 0}, 80, "ngBreakout");
        p.Text(black, {-200, -100}, 40, "press z key to start");
        p.Text(black, {-200, -140}, 40, "press a and d key to move");
        break;
      case State::BeforeShoot:
        p.Text(black, {-200, -100}, 40, "press z key to shoot ball");
        break;
      case State::GameOver:
        p.Text(black, {-200, 0}, 80, "GAME OVER");
    }

    // balls
    p.Text(black, {200, 220}, 30, fmt::format(u8"balls:{0}", balls).c_str());
    // score
    p.Text(black, {-WindowSize.x, 220}, 30,
           fmt::format(u8"score:{0}", score).c_str());

    if (state != State::Title) {
      // blocks
      for (Rect& r : blocks) {
        p.Rect(black, black, r.pos, r.size);
      }
    }
    // mybar
    p.Rect(white, white, mybar.pos, mybar.size);
    // myball
    p.Rect(yellow, yellow, myball.pos, myball.size);
  });
}
