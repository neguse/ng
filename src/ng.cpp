#include "ng.h"

#if defined(__EMSCRIPTEN__)
#include <GLES3/gl3.h>
#include <emscripten.h>
#else
#include <GL/gl3w.h>
#endif

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include <functional>
//
#include <ft2build.h>
#include FT_FREETYPE_H
//
#include <fmt/format.h>

#include <map>
#include <stdexcept>

#include "utf8.h"
//

namespace {

class TickCounter {
 public:
  static const uint32_t kTickCounterInterval = 512;

 private:
  std::vector<uint32_t> tick_times;
  uint32_t tick_times_index;

 public:
  void Reset() {
    tick_times.clear();
    tick_times_index = 0;
  }
  uint32_t PrevTick() {
    uint32_t prev_index =
        (kTickCounterInterval + tick_times_index - 1) % kTickCounterInterval;
    return tick_times[prev_index];
  }
  uint32_t MostOldTick() {
    uint32_t old_index = (tick_times_index + 1) % kTickCounterInterval;
    return tick_times[old_index];
  }
  uint32_t NowTick() { return tick_times[tick_times_index]; }
  bool IsLogEnough() { return tick_times.size() == kTickCounterInterval; }
  void Remember() {
    uint32_t now = SDL_GetTicks();
    if (!IsLogEnough()) {
      tick_times.push_back(now);
      tick_times_index = tick_times.size() - 1;
      return;
    }
    tick_times_index = (tick_times_index + 1) % kTickCounterInterval;
    tick_times[tick_times_index] = now;
  }
  float FPS() {
    if (!IsLogEnough()) {
      return 0.f;
    } else {
      uint32_t old = MostOldTick();
      uint32_t now = NowTick();
      return kTickCounterInterval * 1000.f / float(now - old);
    }
  }
  uint32_t ElapsedTickMsec() {
    if (tick_times.size() < 2) {
      return 0;
    } else {
      return NowTick() - PrevTick();
    }
  }
};

bool CompileShader(const char* code, GLenum type, GLuint* out_shader_id) {
  GLuint shader_id = glCreateShader(type);
  glShaderSource(shader_id, 1, &code, nullptr);
  glCompileShader(shader_id);

  GLint result;
  glGetShaderiv(shader_id, GL_COMPILE_STATUS, &result);
  if (result == GL_TRUE) {
    *out_shader_id = shader_id;
    return true;
  }

  GLint log_length;
  glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &log_length);
  std::vector<GLchar> log(log_length);
  glGetShaderInfoLog(shader_id, log_length, &log_length, &log[0]);
  fprintf(stderr, "CompileShaderError %s", &log[0]);
  glDeleteShader(shader_id);
  return false;
}

bool CompileShaderProgram(const char* vertex_shader_code,
                          const char* fragment_shader_code,
                          GLuint* out_program_id) {
  GLuint vertex_shader_id, fragment_shader_id;
  if (!CompileShader(vertex_shader_code, GL_VERTEX_SHADER, &vertex_shader_id)) {
    return false;
  }
  if (!CompileShader(fragment_shader_code, GL_FRAGMENT_SHADER,
                     &fragment_shader_id)) {
    return false;
  }

  GLuint program_id = glCreateProgram();
  glAttachShader(program_id, vertex_shader_id);
  glAttachShader(program_id, fragment_shader_id);
  glLinkProgram(program_id);

  GLint result = GL_FALSE;
  int log_length;
  glGetProgramiv(program_id, GL_LINK_STATUS, &result);
  glGetProgramiv(program_id, GL_INFO_LOG_LENGTH, &log_length);
  if (result != GL_TRUE) {
    std::vector<char> error_msg(log_length);
    glGetProgramInfoLog(program_id, log_length, NULL, &error_msg[0]);
    fprintf(stderr, "%s\n", &error_msg[0]);
    return false;
  }

  glDeleteShader(vertex_shader_id);
  glDeleteShader(fragment_shader_id);
  *out_program_id = program_id;

  return true;
}

static const std::vector<vec2> kSquareVertex = {
    {-1, -1}, {-1, 1}, {1, -1}, {1, 1}};

static const std::vector<vec2> kLineVertex = {{0, 0}, {1, 0}};

static vec4 color256To1_0(const ngColor& col) {
  return vec4(col) * (1.f / 255.f);
}

struct InputState {
  std::vector<uint8_t> key;
  std::vector<bool> mouse_button;
  ivec2 mouse_position;

  bool GetKeyState(SDL_KeyCode key_code) const {
    auto scan_code = SDL_GetScancodeFromKey(key_code);
    return !!key[scan_code];
  }
  bool GetMouseButtonState(int num) const { return mouse_button[num]; }
};

InputState GetInput() {
  InputState state;

  // key
  int num_keys;
  const Uint8* key_state = SDL_GetKeyboardState(&num_keys);
  state.key.assign(key_state, key_state + num_keys);

  // mouse
  int button =
      SDL_GetMouseState(&state.mouse_position.x, &state.mouse_position.y);
  const int kMouseButtonNum = 5 + 1;
  state.mouse_button.resize(kMouseButtonNum);
  for (int i = 0; i < kMouseButtonNum; i++) {
    state.mouse_button[i] = button & SDL_BUTTON(i);
  }

  return state;
}

}  // namespace

glm::mat3 ngMath::TRS(vec2 pos, float rad, vec2 s) {
  return scale(rotate(translate(mat3(1.f), pos), rad), s);
}

class ngProcessImpl : public ngProcess {
 private:
  SDL_Window* window_;
  SDL_GLContext gl_context_;
  GLuint vao_id_;
  GLuint primitive_program_id_;
  GLuint primitive_uniform_matrix_id_;
  GLuint primitive_uniform_color_id_;

  GLuint textured_program_id_;
  GLuint textured_uniform_matrix_id_;
  GLuint textured_uniform_texture_id_;
  GLuint textured_uniform_color_id_;

  vec2 window_size_;
  std::vector<mat3> trans_stack_;
  GLuint square_vertex_buffer_id_;
  GLuint line_vertex_buffer_id_;
  std::vector<vec2> circle_vertex_;
  GLuint circle_vertex_buffer_id_;

  GLuint text_vertex_buffer_id_;
  GLuint text_uv_buffer_id_;
  GLuint text_texture_id_;

  InputState current_state_;
  InputState prev_state_;
  std::map<ngKeyCode, char> keyboard_mapping_;
  std::map<ngKeyCode, uint8_t> mouse_button_mapping_;

  FT_Library library_;
  FT_Face face_;
  FT_GlyphSlot slot_;

  TickCounter tick_counter_;

  ngUpdater updater_;

  bool exit_;

 public:
  virtual ~ngProcessImpl();
  bool Init() override;
  void Run(ngUpdater updater) override;
  void ExitLoop() override;
  void Push(const mat3& mat) override;

  void Pop() override;
  void Clear(const ngColor& col) override {
    glClearColor(col.r / 255.f, col.g / 255.f, col.b / 255.f, col.a / 255.f);
    glClear(GL_COLOR_BUFFER_BIT);
  }

  void Rect(const ngColor& border, const ngColor& fill, const vec2& center,
            const vec2& size) override {
    Push(ngMath::TRS(center, 0, size));

    glUseProgram(primitive_program_id_);

    // setup uniform
    glUniformMatrix3fv(primitive_uniform_matrix_id_, 1, GL_FALSE,
                       &trans_stack_.back()[0][0]);
    vec4 col = color256To1_0(fill);
    glUniform4fv(primitive_uniform_color_id_, 1, (GLfloat*)&col);

    // setup vertex
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, square_vertex_buffer_id_);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, kSquareVertex.size());

    glDisableVertexAttribArray(0);
    Pop();
  }

  void Square(const ngColor& border, const ngColor& fill, const vec2& center,
              const float size) override {
    Rect(border, fill, center, {size, size});
  }

  void Line(const ngColor& col, const vec2& pos1, const vec2& pos2) override {
    vec2 d = pos2 - pos1;
    float a = atan2(d.y, d.x);
    float l = length(d);
    Push(ngMath::TRS(pos1, a, {l, l}));
    glUseProgram(primitive_program_id_);

    // setup uniform
    glUniformMatrix3fv(primitive_uniform_matrix_id_, 1, GL_FALSE,
                       &trans_stack_.back()[0][0]);
    vec4 col1_0 = color256To1_0(col);
    glUniform4fv(primitive_uniform_color_id_, 1, (GLfloat*)&col1_0);

    // setup vertex
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, line_vertex_buffer_id_);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);

    glDrawArrays(GL_LINE_STRIP, 0, kLineVertex.size());

    glDisableVertexAttribArray(0);
    Pop();
  }

  void Circle(const ngColor& border, const ngColor& fill, const ngCoord& center,
              const float& length) override {
    Push(ngMath::TRS(center, 0.f, {length, length}));
    glUseProgram(primitive_program_id_);

    // setup uniform
    glUniformMatrix3fv(primitive_uniform_matrix_id_, 1, GL_FALSE,
                       &trans_stack_.back()[0][0]);
    vec4 col1_0 = color256To1_0(fill);
    glUniform4fv(primitive_uniform_color_id_, 1, (GLfloat*)&col1_0);

    // setup vertex
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, circle_vertex_buffer_id_);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);

    glDrawArrays(GL_TRIANGLE_FAN, 0, circle_vertex_.size());

    glDisableVertexAttribArray(0);
    Pop();
  }

  void Text(const ngColor& col, const ngCoord& pos, float length,
            const char* str) override {
    uint32_t codepoint;
    uint32_t state = 0;
    float x_advance = 0.f;
    for (; *str; ++str) {
      if (!utf8decode(&state, &codepoint, *str)) {
        FT_Error err;
        if (err = FT_Load_Glyph(face_, FT_Get_Char_Index(face_, codepoint),
                                FT_LOAD_RENDER | FT_LOAD_COLOR),
            err != 0) {
          fprintf(stderr, "failed to FT_Load_Glyph\n");
          return;
        };

        std::vector<u8> pixels((64 + 4) * (64 + 4));

        const FT_GlyphSlot& glyph = face_->glyph;
        const FT_Bitmap& bitmap = glyph->bitmap;
        const FT_Glyph_Metrics& metrics = glyph->metrics;

        for (int y = 0; y < bitmap.rows; y++) {
          for (int x = 0; x < bitmap.width; x++) {
            pixels[(bitmap.rows - 1 - y) * 64 + x] =
                bitmap.buffer[y * bitmap.width + x];
          }
        }

        glBindTexture(GL_TEXTURE_2D, text_texture_id_);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, 64, 64, 0, GL_RED,
                     GL_UNSIGNED_BYTE, &pixels[0]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        // glGenerateMipmap(GL_TEXTURE_2D);

        Push(ngMath::TRS(pos, 0.f, {length / 64, length / 64}));
        glUseProgram(textured_program_id_);

        // setup uniform
        glUniformMatrix3fv(textured_uniform_matrix_id_, 1, GL_FALSE,
                           &trans_stack_.back()[0][0]);
        vec4 col1_0 = color256To1_0(col);
        glUniform4fv(textured_uniform_color_id_, 1, (GLfloat*)&col1_0);
        glUniform1i(textured_uniform_texture_id_, 0);

        // setup vertex
        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, text_vertex_buffer_id_);
        vec2 origin = {metrics.horiBearingX + x_advance,
                       -metrics.height + metrics.horiBearingY};
        vec2 right = {metrics.width, 0};
        vec2 up = {0, metrics.height};
        float scale = 1.f / 64.f;
        std::vector<vec2> text_vertex{
            scale * origin,
            scale * (origin + up),
            scale * (origin + right),
            scale * (origin + right + up),
        };
        glBufferData(GL_ARRAY_BUFFER,
                     sizeof(text_vertex[0]) * text_vertex.size(),
                     &text_vertex[0], GL_DYNAMIC_DRAW);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);

        glEnableVertexAttribArray(1);
        glBindBuffer(GL_ARRAY_BUFFER, text_uv_buffer_id_);
        float uv_w = bitmap.width / 64.f;
        float uv_h = bitmap.rows / 64.f;

        std::vector<vec2> uv_vertex{
            {0, 0},
            {0, uv_h},
            {uv_w, 0},
            {uv_w, uv_h},
        };
        glBufferData(GL_ARRAY_BUFFER, sizeof(uv_vertex[0]) * uv_vertex.size(),
                     &uv_vertex[0], GL_DYNAMIC_DRAW);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);

        glDrawArrays(GL_TRIANGLE_STRIP, 0, text_vertex.size());

        glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(1);
        Pop();
        x_advance += metrics.horiAdvance;
      }
    }
  }

  void MapKeyboard(char key, ngKeyCode code) override {
    keyboard_mapping_.insert_or_assign(code, key);
  }

  void MapMouseButton(uint8_t num, ngKeyCode code) override {
    mouse_button_mapping_.insert_or_assign(code, num);
  }

  // void MapPadButton(uint8_t padIndex, uint8_t buttonIndex,
  //                  ngKeyCode code) override {
  //  throw std::logic_error("The method or operation is not implemented.");
  //}

  bool GetButtonState(const InputState& state, ngKeyCode code) {
    bool button = false;
    if (auto it = keyboard_mapping_.find(code); it != keyboard_mapping_.end()) {
      button |= !!state.GetKeyState(SDL_KeyCode(it->second));
    }
    if (auto it = mouse_button_mapping_.find(code);
        it != mouse_button_mapping_.end()) {
      button |= !!state.GetMouseButtonState(it->second);
    }
    return button;
  }

  bool IsHold(ngKeyCode code) override {
    return GetButtonState(current_state_, code);
  }

  bool IsJustPressed(ngKeyCode code) override {
    return GetButtonState(current_state_, code) &&
           !GetButtonState(prev_state_, code);
  }

  ngCoord CursorPos() override {
    return ngCoord(current_state_.mouse_position);
  }

  void Tick();
};

ngProcessImpl::~ngProcessImpl() {
  glDeleteVertexArrays(1, &vao_id_);
  SDL_GL_DeleteContext(gl_context_);
  SDL_DestroyWindow(window_);
  SDL_Quit();
}

bool ngProcessImpl::Init() {
  exit_ = false;

  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_AUDIO) != 0) {
    fprintf(stderr, "ERROR: %s\n", SDL_GetError());
    return false;
  }

  int flags = IMG_INIT_JPG | IMG_INIT_PNG;
  if (!(IMG_Init(flags) & flags)) {
    fprintf(stderr, "Error: %s\n", IMG_GetError());
    return false;
  }

  window_size_ = vec2(320, 240);

#if defined(__EMSCRIPTEN__)
#define SHADER_HEADER "#version 300 es"
  NG_VERIFY(!SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0));
  NG_VERIFY(!SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
                                 SDL_GL_CONTEXT_PROFILE_ES));
  NG_VERIFY(!SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3));
  NG_VERIFY(!SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0));
#else
#define SHADER_HEADER "#version 330 core"
  NG_VERIFY(!SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0));
  NG_VERIFY(!SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
                                 SDL_GL_CONTEXT_PROFILE_CORE));
  NG_VERIFY(!SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3));
  NG_VERIFY(!SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3));
#endif

  // Create window with graphics context
  NG_VERIFY(!SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1));
  NG_VERIFY(!SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24));
  NG_VERIFY(!SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8));
  SDL_WindowFlags window_flags = (SDL_WindowFlags)(
      SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
  window_ =
      SDL_CreateWindow("ng", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                       (int)window_size_.x, (int)window_size_.y, window_flags);
  SDL_GLContext gl_context_ = SDL_GL_CreateContext(window_);
  NG_VERIFY(!SDL_GL_MakeCurrent(window_, gl_context_));
  NG_VERIFY(!SDL_GL_SetSwapInterval(1));  // Enable vsync

#if !defined(__EMSCRIPTEN__)
  NG_VERIFY(!gl3wInit());
#endif

  // create VAO
  glGenVertexArrays(1, &vao_id_);
  glBindVertexArray(vao_id_);

  // create primitive shader
  const char* PRIMITIVE_VERTEX_SHADER_CODE = SHADER_HEADER R"(
precision mediump float;
layout(location = 0) in vec2 in_position;
uniform mat3 u_matrix;
void main(){
  vec3 pos = u_matrix * vec3(in_position, 1);
	gl_Position =  vec4(pos, 1);
}
)";
  const char* PRIMITIVE_FRAGMENT_SHADER_CODE = SHADER_HEADER R"(
precision mediump float;
uniform vec4 u_color;
out vec4 out_color;
void main(){
    out_color = u_color;
}
)";

  if (!CompileShaderProgram(PRIMITIVE_VERTEX_SHADER_CODE,
                            PRIMITIVE_FRAGMENT_SHADER_CODE,
                            &primitive_program_id_)) {
    return false;
  }
  primitive_uniform_matrix_id_ =
      glGetUniformLocation(primitive_program_id_, "u_matrix");
  primitive_uniform_color_id_ =
      glGetUniformLocation(primitive_program_id_, "u_color");

  // create textured shader
  const char* TEXTURED_VERTEX_SHADER_CODE = SHADER_HEADER R"(
precision mediump float;
layout(location = 0) in vec2 in_position;
layout(location = 1) in vec2 in_uv;
out vec2 uv;
uniform mat3 u_matrix;
void main(){
  vec3 pos = u_matrix * vec3(in_position, 1);
	gl_Position = vec4(pos, 1);
  uv = in_uv;
}
)";
  const char* TEXTURED_FRAGMENT_SHADER_CODE = SHADER_HEADER R"(
precision mediump float;
in vec2 uv;
out vec4 out_color;
uniform vec4 u_color;
uniform sampler2D u_texture;
void main(){
  float a = texture(u_texture, uv).r;
  out_color = vec4(u_color.rgb, u_color.a * a);
}
)";
#undef SHADER_HEADER

  if (!CompileShaderProgram(TEXTURED_VERTEX_SHADER_CODE,
                            TEXTURED_FRAGMENT_SHADER_CODE,
                            &textured_program_id_)) {
    return false;
  }
  textured_uniform_matrix_id_ =
      glGetUniformLocation(textured_program_id_, "u_matrix");
  textured_uniform_texture_id_ =
      glGetUniformLocation(textured_program_id_, "u_texture");
  textured_uniform_color_id_ =
      glGetUniformLocation(textured_program_id_, "u_color");

  // create vertex buffer
  glGenBuffers(1, &square_vertex_buffer_id_);
  glBindBuffer(GL_ARRAY_BUFFER, square_vertex_buffer_id_);
  glBufferData(GL_ARRAY_BUFFER, sizeof(kSquareVertex[0]) * kSquareVertex.size(),
               &kSquareVertex[0], GL_STATIC_DRAW);
  glGenBuffers(1, &line_vertex_buffer_id_);
  glBindBuffer(GL_ARRAY_BUFFER, line_vertex_buffer_id_);
  glBufferData(GL_ARRAY_BUFFER, sizeof(kLineVertex[0]) * kLineVertex.size(),
               &kLineVertex[0], GL_STATIC_DRAW);

  circle_vertex_.clear();
  circle_vertex_.push_back({0.f, 0.f});
  for (int i = 0; i <= 32; i++) {
    float rad = pi<float>() * 2.f * float(i) / 32.f;
    circle_vertex_.push_back({cos(rad), sin(rad)});
  }
  glGenBuffers(1, &circle_vertex_buffer_id_);
  glBindBuffer(GL_ARRAY_BUFFER, circle_vertex_buffer_id_);
  glBufferData(GL_ARRAY_BUFFER,
               sizeof(circle_vertex_[0]) * circle_vertex_.size(),
               &circle_vertex_[0], GL_STATIC_DRAW);

  glGenBuffers(1, &text_vertex_buffer_id_);
  glGenBuffers(1, &text_uv_buffer_id_);
  glGenTextures(1, &text_texture_id_);

  // glEnable(GL_DEPTH_TEST);
  // glDepthFunc(GL_LESS);

  // glDisable(GL_CULL_FACE);

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  if (FT_Init_FreeType(&library_) != 0) {
    fprintf(stderr, "failed to FT_Init_FreeType\n");
    return false;
  }

  const char* kFontFilePath = "asset/font/NotoSansJP-Regular.otf";
  if (FT_New_Face(library_, kFontFilePath, 0, &face_) != 0) {
    fprintf(stderr, "failed to FT_New_Face\n");
    return false;
  }

  if (FT_Set_Pixel_Sizes(face_, 0, 64) != 0) {
    fprintf(stderr, "failed to FT_Set_Pixel_Sizes\n");
    return false;
  }

  tick_counter_.Reset();

  return true;
}

#if defined(__EMSCRIPTEN__)
void esMainLoop(void* arg) {
  auto* proc = reinterpret_cast<ngProcessImpl*>(arg);
  proc->Tick();
}
#endif

void ngProcessImpl::Run(ngUpdater updater) {
  updater_ = updater;
#if defined(__EMSCRIPTEN__)
  emscripten_set_main_loop_arg(esMainLoop, this, 0, true);
#else
  while (!exit_) {
    Tick();
  }
#endif
}

void ngProcessImpl::ExitLoop() { exit_ = true; }

void ngProcessImpl::Push(const mat3& mat) {
  trans_stack_.push_back(trans_stack_.back() * mat);
}

void ngProcessImpl::Pop() { trans_stack_.pop_back(); }

void ngProcessImpl::Tick() {
  // update tick counter
  tick_counter_.Remember();
  if (tick_counter_.IsLogEnough()) {
    // std::string title = fmt::format("ng fps:{0}", tick_counter_.FPS());
    // SDL_SetWindowTitle(window_, title.c_str());
  }

  // process all event
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    switch (event.type) {
      case SDL_QUIT: {
        exit_ = true;
        break;
      }
      case SDL_WINDOWEVENT: {
        if (event.window.event == SDL_WINDOWEVENT_CLOSE &&
            event.window.windowID == SDL_GetWindowID(window_)) {
          exit_ = true;
        }
        break;
      }
    }
  }

  // input
  prev_state_ = current_state_;
  current_state_ = GetInput();

  // initialize matrix
  trans_stack_.clear();
  trans_stack_.push_back(
      scale(mat3(1.f), vec2(1.f / window_size_.x, 1.f / window_size_.y)));
  glViewport(0, 0, window_size_.x, window_size_.y);

  // update game
  updater_(*this, (float)(tick_counter_.ElapsedTickMsec()) * 0.001f);
  SDL_GL_SwapWindow(window_);
}

std::unique_ptr<ngProcess> ngProcess::NewProcess() {
  return std::make_unique<ngProcessImpl>();
}
