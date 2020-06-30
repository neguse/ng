#include "ng.h"

enum {
  KEY_UP,
  KEY_DOWN,
  KEY_RIGHT,
  KEY_LEFT,
  KEY_1,
  KEY_2,
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
  float x = 0.f;
  int button_press_count = 0;
  vec2 pos(0.f, 0.f);
  proc->Run([&](ngProcess& p, float dt) {
    // update
    x += dt;

    const float velocity = 100.f;
    if (p.IsHold(KEY_LEFT)) {
      pos.x -= velocity * dt;
    }
    if (p.IsHold(KEY_RIGHT)) {
      pos.x += velocity * dt;
    }
    if (p.IsHold(KEY_UP)) {
      pos.y += velocity * dt;
    }
    if (p.IsHold(KEY_DOWN)) {
      pos.y -= velocity * dt;
    }

    if (p.IsJustPressed(KEY_1)) {
      button_press_count++;
    }
    if (p.IsJustPressed(KEY_2)) {
      button_press_count--;
    }

    // drawing
    p.Clear({0x80, 0x80, 0x80, 0xff});
    const auto white = ngColor(0xff, 0xff, 0xff, 0xff);
    const auto black = ngColor(0x00, 0x00, 0x00, 0xff);
    const auto translucent_red = ngColor(0xff, 0x00, 0x00, 0x80);

    p.Line(black, {0, 0}, vec2(cos(x), sin(x)) * 100.f);
    float l = 100.f + cos(x) * 100.f;
    p.Circle(translucent_red, translucent_red, {0, 0}, l);
    auto txt = fmt::format(u8"„ÅÇ„ÅÅÅEûpos({0},{1}) btn({2})", p.CursorPos().x,
                           p.CursorPos().y, button_press_count);
    p.Text(black, {-320, 0}, 30, txt.c_str());
    p.Square(white, white, pos, 10);
  });
}
