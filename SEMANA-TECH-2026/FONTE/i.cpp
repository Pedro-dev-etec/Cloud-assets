#include <SDL2/SDL.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>

// Herculez Pool Club - um jogo 8-ball em SDL2, sem assets externos.
// Compile com: g++ i.cpp -std=c++17 $(sdl2-config --cflags --libs) -O2 -o herculez_pool

constexpr int SCREEN_W = 1280;
constexpr int SCREEN_H = 800;
constexpr float PI = 3.14159265358979323846f;
constexpr float BALL_R = 13.0f;
constexpr float POCKET_R = 27.0f;

struct Vec2 {
    float x = 0.0f;
    float y = 0.0f;
    Vec2 operator+(const Vec2& b) const { return {x + b.x, y + b.y}; }
    Vec2 operator-(const Vec2& b) const { return {x - b.x, y - b.y}; }
    Vec2 operator*(float s) const { return {x * s, y * s}; }
    Vec2& operator+=(const Vec2& b) { x += b.x; y += b.y; return *this; }
    Vec2& operator-=(const Vec2& b) { x -= b.x; y -= b.y; return *this; }
};

static float dot(const Vec2& a, const Vec2& b) { return a.x * b.x + a.y * b.y; }
static float length(const Vec2& v) { return std::sqrt(dot(v, v)); }
static Vec2 normalized(const Vec2& v) {
    const float l = length(v);
    return l > 0.0001f ? v * (1.0f / l) : Vec2{};
}
static float clampf(float v, float lo, float hi) { return std::max(lo, std::min(hi, v)); }
static SDL_Color rgba(Uint8 r, Uint8 g, Uint8 b, Uint8 a = 255) { return {r, g, b, a}; }

static const std::array<SDL_Color, 8> BALL_COLORS = {
    rgba(242, 194, 54), rgba(43, 117, 211), rgba(221, 67, 61), rgba(133, 69, 171),
    rgba(242, 143, 40), rgba(39, 148, 93), rgba(126, 60, 43), rgba(28, 28, 32)
};

struct Ball {
    Vec2 pos;
    Vec2 vel;
    int number = 0;
    bool pocketed = false;
    bool striped = false;
};

struct Theme {
    SDL_Color felt;
    SDL_Color feltLight;
    SDL_Color rail;
    SDL_Color accent;
    const char* name;
};

static const std::array<Theme, 3> THEMES = {{
    {rgba(13, 83, 71), rgba(22, 116, 95), rgba(88, 53, 34), rgba(212, 168, 82), "EMERALD"},
    {rgba(17, 48, 84), rgba(28, 79, 125), rgba(71, 42, 43), rgba(235, 112, 85), "MIDNIGHT"},
    {rgba(69, 31, 76), rgba(105, 44, 99), rgba(47, 31, 46), rgba(220, 170, 245), "ROYAL"}
}};

static std::vector<std::string> glyph(char raw) {
    const char c = static_cast<char>(std::toupper(static_cast<unsigned char>(raw)));
    static const std::unordered_map<char, std::vector<std::string>> font = {
        {'A', {"01110", "10001", "10001", "11111", "10001", "10001", "10001"}},
        {'B', {"11110", "10001", "10001", "11110", "10001", "10001", "11110"}},
        {'C', {"01111", "10000", "10000", "10000", "10000", "10000", "01111"}},
        {'D', {"11110", "10001", "10001", "10001", "10001", "10001", "11110"}},
        {'E', {"11111", "10000", "10000", "11110", "10000", "10000", "11111"}},
        {'F', {"11111", "10000", "10000", "11110", "10000", "10000", "10000"}},
        {'G', {"01111", "10000", "10000", "10111", "10001", "10001", "01111"}},
        {'H', {"10001", "10001", "10001", "11111", "10001", "10001", "10001"}},
        {'I', {"11111", "00100", "00100", "00100", "00100", "00100", "11111"}},
        {'J', {"00111", "00010", "00010", "00010", "00010", "10010", "01100"}},
        {'K', {"10001", "10010", "10100", "11000", "10100", "10010", "10001"}},
        {'L', {"10000", "10000", "10000", "10000", "10000", "10000", "11111"}},
        {'M', {"10001", "11011", "10101", "10101", "10001", "10001", "10001"}},
        {'N', {"10001", "11001", "10101", "10011", "10001", "10001", "10001"}},
        {'O', {"01110", "10001", "10001", "10001", "10001", "10001", "01110"}},
        {'P', {"11110", "10001", "10001", "11110", "10000", "10000", "10000"}},
        {'Q', {"01110", "10001", "10001", "10001", "10101", "10010", "01101"}},
        {'R', {"11110", "10001", "10001", "11110", "10100", "10010", "10001"}},
        {'S', {"01111", "10000", "10000", "01110", "00001", "00001", "11110"}},
        {'T', {"11111", "00100", "00100", "00100", "00100", "00100", "00100"}},
        {'U', {"10001", "10001", "10001", "10001", "10001", "10001", "01110"}},
        {'V', {"10001", "10001", "10001", "10001", "10001", "01010", "00100"}},
        {'W', {"10001", "10001", "10001", "10101", "10101", "11011", "10001"}},
        {'X', {"10001", "10001", "01010", "00100", "01010", "10001", "10001"}},
        {'Y', {"10001", "10001", "01010", "00100", "00100", "00100", "00100"}},
        {'Z', {"11111", "00001", "00010", "00100", "01000", "10000", "11111"}},
        {'0', {"01110", "10001", "10011", "10101", "11001", "10001", "01110"}},
        {'1', {"00100", "01100", "00100", "00100", "00100", "00100", "01110"}},
        {'2', {"01110", "10001", "00001", "00010", "00100", "01000", "11111"}},
        {'3', {"11110", "00001", "00001", "01110", "00001", "00001", "11110"}},
        {'4', {"00010", "00110", "01010", "10010", "11111", "00010", "00010"}},
        {'5', {"11111", "10000", "10000", "11110", "00001", "00001", "11110"}},
        {'6', {"01110", "10000", "10000", "11110", "10001", "10001", "01110"}},
        {'7', {"11111", "00001", "00010", "00100", "01000", "01000", "01000"}},
        {'8', {"01110", "10001", "10001", "01110", "10001", "10001", "01110"}},
        {'9', {"01110", "10001", "10001", "01111", "00001", "00001", "01110"}},
        {'.', {"00000", "00000", "00000", "00000", "00000", "00110", "00110"}},
        {':', {"00000", "00110", "00110", "00000", "00110", "00110", "00000"}},
        {'-', {"00000", "00000", "00000", "11111", "00000", "00000", "00000"}},
        {'/', {"00001", "00010", "00010", "00100", "01000", "01000", "10000"}},
        {'!', {"00100", "00100", "00100", "00100", "00100", "00000", "00100"}},
        {'%', {"11001", "11010", "00000", "00100", "00000", "01011", "10011"}},
        {'?', {"01110", "10001", "00001", "00010", "00100", "00000", "00100"}}
    };
    auto it = font.find(c);
    return it == font.end() ? std::vector<std::string>(7, "00000") : it->second;
}

static int textWidth(const std::string& text, int scale) {
    return static_cast<int>(text.size()) * (6 * scale);
}

static void drawText(SDL_Renderer* renderer, const std::string& text, int x, int y,
                     int scale, SDL_Color color, bool centered = false) {
    if (centered) x -= textWidth(text, scale) / 2;
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    int cursor = x;
    for (char c : text) {
        const auto rows = glyph(c);
        for (int row = 0; row < 7; ++row) {
            for (int col = 0; col < 5; ++col) {
                if (rows[row][col] == '1') {
                    SDL_Rect pixel{cursor + col * scale, y + row * scale, scale, scale};
                    SDL_RenderFillRect(renderer, &pixel);
                }
            }
        }
        cursor += 6 * scale;
    }
}

static void fillCircle(SDL_Renderer* renderer, int cx, int cy, int radius, SDL_Color color) {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    for (int y = -radius; y <= radius; ++y) {
        const int half = static_cast<int>(std::sqrt(std::max(0, radius * radius - y * y)));
        SDL_RenderDrawLine(renderer, cx - half, cy + y, cx + half, cy + y);
    }
}

static void drawRing(SDL_Renderer* renderer, int cx, int cy, int radius, SDL_Color color) {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    for (int i = 0; i < 48; ++i) {
        const float a0 = (2.0f * PI * i) / 48.0f;
        const float a1 = (2.0f * PI * (i + 1)) / 48.0f;
        SDL_RenderDrawLine(renderer, cx + static_cast<int>(std::cos(a0) * radius),
                           cy + static_cast<int>(std::sin(a0) * radius),
                           cx + static_cast<int>(std::cos(a1) * radius),
                           cy + static_cast<int>(std::sin(a1) * radius));
    }
}

class PoolGame {
public:
    bool init() {
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) return false;
        window = SDL_CreateWindow("HERCULEZ POOL CLUB", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                  SCREEN_W, SCREEN_H, SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
        if (!window) return false;
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
        if (!renderer) renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
        if (!renderer) return false;
        SDL_RenderSetLogicalSize(renderer, SCREEN_W, SCREEN_H);
        loadAIProfile();
        resetGame();
        return true;
    }

    void run() {
        bool running = true;
        Uint64 last = SDL_GetPerformanceCounter();
        while (running) {
            const Uint64 now = SDL_GetPerformanceCounter();
            float dt = static_cast<float>(now - last) / static_cast<float>(SDL_GetPerformanceFrequency());
            last = now;
            dt = std::min(dt, 0.033f);
            SDL_Event event;
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_QUIT) running = false;
                else handleEvent(event);
            }
            update(dt);
            render();
        }
    }

    ~PoolGame() {
        saveAIProfile();
        if (renderer) SDL_DestroyRenderer(renderer);
        if (window) SDL_DestroyWindow(window);
        SDL_Quit();
    }

private:
    enum class ScreenState { Menu, Match };

    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    std::vector<Ball> balls;
    ScreenState screen = ScreenState::Menu;
    bool vsAI = false;
    int themeIndex = 0;
    int cueStyle = 0;
    int currentPlayer = 0;
    int playerGroup[2] = {-1, -1}; // 0 = lisas, 1 = listradas
    int score[2] = {0, 0};
    bool ballsMoving = false;
    bool aiming = false;
    bool customizer = false;
    bool gameOver = false;
    bool foulThisTurn = false;
    bool pottedThisTurn = false;
    bool aiThinking = false;
    Vec2 aimMouse{0, 0};
    std::string status = "YOUR TURN - POSITION THE SHOT";
    float statusTimer = 0.0f;
    float aiTimer = 0.0f;
    float aiThinkDuration = 1.4f;
    int aiExperience = 0;
    int aiStyle = 1; // 0 defensive, 1 balanced, 2 aggressive
    int aiTempo = 1; // 0 patient, 1 standard, 2 quick
    std::mt19937 rng{std::random_device{}()};

    const float left = 120.0f;
    const float right = 1080.0f;
    const float top = 190.0f;
    const float bottom = 670.0f;
    const std::array<Vec2, 6> pockets = {{{left, top}, {(left + right) / 2.0f, top}, {right, top},
                                          {left, bottom}, {(left + right) / 2.0f, bottom}, {right, bottom}}};

    void loadAIProfile() {
        std::ifstream profile("herculez_ai.profile");
        if (profile) profile >> aiExperience >> aiStyle >> aiTempo;
        aiExperience = std::max(0, std::min(100, aiExperience));
        aiStyle = std::max(0, std::min(2, aiStyle));
        aiTempo = std::max(0, std::min(2, aiTempo));
    }

    void saveAIProfile() const {
        std::ofstream profile("herculez_ai.profile", std::ios::trunc);
        if (profile) profile << aiExperience << " " << aiStyle << " " << aiTempo << "\n";
    }

    void startMatch(bool againstAI) {
        vsAI = againstAI;
        screen = ScreenState::Match;
        resetGame();
    }

    void resetGame() {
        balls.clear();
        balls.push_back({{350, 430}, {}, 0, false, false});
        int number = 1;
        for (int row = 0; row < 5; ++row) {
            for (int col = 0; col <= row; ++col) {
                const float x = 795.0f + row * 23.0f;
                const float y = 430.0f + (col - row * 0.5f) * 26.0f;
                Ball ball{{x, y}, {}, number, false, number > 8};
                balls.push_back(ball);
                ++number;
            }
        }
        // O 8-ball fica no centro da formação para uma leitura clássica.
        std::swap(balls[8], balls[9]);
        balls[8].number = 8;
        balls[8].striped = false;
        balls[9].number = 9;
        balls[9].striped = true;
        currentPlayer = 0;
        playerGroup[0] = playerGroup[1] = -1;
        score[0] = score[1] = 0;
        ballsMoving = false;
        aiming = false;
        customizer = false;
        gameOver = false;
        foulThisTurn = false;
        pottedThisTurn = false;
        aiThinking = false;
        aiTimer = 0.0f;
        aimMouse = {530, 430};
        status = "YOUR TURN - POSITION THE SHOT";
        statusTimer = 0.0f;
    }

    Vec2 mouseWorld(int x, int y) const {
        int w = SCREEN_W, h = SCREEN_H;
        SDL_GetWindowSize(window, &w, &h);
        return {static_cast<float>(x) * SCREEN_W / std::max(1, w),
                static_cast<float>(y) * SCREEN_H / std::max(1, h)};
    }

    bool inside(float x, float y, float x0, float y0, float w, float h) const {
        return x >= x0 && x <= x0 + w && y >= y0 && y <= y0 + h;
    }

    void handleEvent(const SDL_Event& e) {
        if (screen == ScreenState::Menu) {
            if (e.type == SDL_KEYDOWN && !e.key.repeat) {
                if (e.key.keysym.sym == SDLK_1) startMatch(false);
                if (e.key.keysym.sym == SDLK_2 || e.key.keysym.sym == SDLK_RETURN) startMatch(true);
            }
            if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
                const Vec2 m = mouseWorld(e.button.x, e.button.y);
                if (inside(m.x, m.y, 235, 420, 350, 82)) startMatch(false);
                if (inside(m.x, m.y, 695, 420, 350, 82)) startMatch(true);
            }
            return;
        }
        if (e.type == SDL_KEYDOWN && !e.key.repeat) {
            if (e.key.keysym.sym == SDLK_ESCAPE) {
                if (customizer) customizer = false;
                else if (!ballsMoving && !aiThinking) screen = ScreenState::Menu;
            }
            if (e.key.keysym.sym == SDLK_c) customizer = !customizer;
            if (e.key.keysym.sym == SDLK_n) resetGame();
            if (customizer) {
                if (e.key.keysym.sym == SDLK_1) themeIndex = 0;
                if (e.key.keysym.sym == SDLK_2) themeIndex = 1;
                if (e.key.keysym.sym == SDLK_3) themeIndex = 2;
                if (e.key.keysym.sym == SDLK_q) cueStyle = 0;
                if (e.key.keysym.sym == SDLK_w) cueStyle = 1;
                if (e.key.keysym.sym == SDLK_e) cueStyle = 2;
                if (e.key.keysym.sym == SDLK_a) aiStyle = 0;
                if (e.key.keysym.sym == SDLK_s) aiStyle = 1;
                if (e.key.keysym.sym == SDLK_d) aiStyle = 2;
                if (e.key.keysym.sym == SDLK_z) aiTempo = 0;
                if (e.key.keysym.sym == SDLK_x) aiTempo = 1;
                if (e.key.keysym.sym == SDLK_v) aiTempo = 2;
                if (e.key.keysym.sym == SDLK_MINUS || e.key.keysym.sym == SDLK_KP_MINUS)
                    aiExperience = std::max(0, aiExperience - 5);
                if (e.key.keysym.sym == SDLK_EQUALS || e.key.keysym.sym == SDLK_KP_PLUS)
                    aiExperience = std::min(100, aiExperience + 5);
                saveAIProfile();
            }
        }
        if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
            const Vec2 m = mouseWorld(e.button.x, e.button.y);
            if (customizer) {
                if (inside(m.x, m.y, 230, 265, 105, 58)) themeIndex = 0;
                if (inside(m.x, m.y, 355, 265, 105, 58)) themeIndex = 1;
                if (inside(m.x, m.y, 480, 265, 105, 58)) themeIndex = 2;
                if (inside(m.x, m.y, 230, 415, 105, 58)) cueStyle = 0;
                if (inside(m.x, m.y, 355, 415, 105, 58)) cueStyle = 1;
                if (inside(m.x, m.y, 480, 415, 105, 58)) cueStyle = 2;
                if (inside(m.x, m.y, 650, 285, 105, 55)) aiStyle = 0;
                if (inside(m.x, m.y, 775, 285, 105, 55)) aiStyle = 1;
                if (inside(m.x, m.y, 900, 285, 105, 55)) aiStyle = 2;
                if (inside(m.x, m.y, 650, 410, 52, 48)) aiExperience = std::max(0, aiExperience - 5);
                if (inside(m.x, m.y, 955, 410, 52, 48)) aiExperience = std::min(100, aiExperience + 5);
                if (inside(m.x, m.y, 650, 525, 105, 50)) aiTempo = 0;
                if (inside(m.x, m.y, 775, 525, 105, 50)) aiTempo = 1;
                if (inside(m.x, m.y, 900, 525, 105, 50)) aiTempo = 2;
                if (inside(m.x, m.y, 900, 680, 160, 45)) customizer = false;
                saveAIProfile();
                return;
            }
            if (inside(m.x, m.y, 1115, 32, 145, 52)) {
                customizer = true;
                return;
            }
            if (!ballsMoving && !aiThinking && !gameOver && currentPlayer == 0 && !cuePocketed()) {
                aiming = true;
                aimMouse = m;
            }
        }
        if (e.type == SDL_MOUSEMOTION) aimMouse = mouseWorld(e.motion.x, e.motion.y);
        if (e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_LEFT && aiming) {
            aiming = false;
            if (!ballsMoving && !aiThinking && !gameOver && currentPlayer == 0) shoot(mouseWorld(e.button.x, e.button.y));
        }
    }

    bool cuePocketed() const { return balls.empty() || balls[0].pocketed; }

    void shoot(const Vec2& mouse) {
        // O arraste aponta a tacada: da branca em direção ao cursor.
        const Vec2 shotDirection = mouse - balls[0].pos;
        const float power = clampf(length(shotDirection), 0.0f, 190.0f);
        if (power < 12.0f) return;
        launchShot(normalized(shotDirection), power);
    }

    void launchShot(const Vec2& direction, float power) {
        balls[0].vel = normalized(direction) * (power * 5.0f + 100.0f);
        ballsMoving = true;
        foulThisTurn = false;
        pottedThisTurn = false;
        status = "SHOT IN PROGRESS";
        statusTimer = 0.0f;
    }

    void beginAIThinking() {
        if (!vsAI || currentPlayer != 1 || gameOver) return;
        aiThinking = true;
        aiTimer = 0.0f;
        const std::array<float, 3> minThink = {1.35f, 0.9f, 0.5f};
        const std::array<float, 3> maxThink = {2.35f, 1.8f, 1.05f};
        std::uniform_real_distribution<float> thinkTime(minThink[aiTempo], maxThink[aiTempo]);
        aiThinkDuration = thinkTime(rng);
        status = "AI IS READING THE TABLE";
        statusTimer = 0.0f;
    }

    void executeAIShot() {
        if (!aiThinking || gameOver || ballsMoving || currentPlayer != 1) return;
        aiThinking = false;

        const int group = playerGroup[1];
        int bestBall = -1;
        Vec2 bestPocket{};
        Vec2 bestDirection{1.0f, 0.0f};
        float bestScore = 1000000.0f;
        std::uniform_real_distribution<float> planningNoise(0.0f, (100.0f - aiExperience) * 2.2f);
        const int candidateCount = (group < 0) ? 15 : 15;
        for (int i = 1; i <= candidateCount && i < static_cast<int>(balls.size()); ++i) {
            const Ball& target = balls[i];
            if (target.pocketed || target.number == 8) continue;
            if (group >= 0 && (target.striped ? 1 : 0) != group) continue;
            for (const Vec2& pocket : pockets) {
                const Vec2 targetToPocket = normalized(pocket - target.pos);
                const Vec2 contact = target.pos - targetToPocket * (2.0f * BALL_R + 1.0f);
                const Vec2 cueToContact = contact - balls[0].pos;
                const float shotLength = length(cueToContact);
                if (shotLength < 20.0f) continue;
                const float pocketWeight = aiStyle == 0 ? 0.65f : (aiStyle == 2 ? 0.15f : 0.35f);
                const float complexityPenalty = aiStyle == 0 ? shotLength * 0.22f :
                                                (aiStyle == 2 ? shotLength * 0.03f : shotLength * 0.12f);
                const float routeScore = shotLength + length(target.pos - pocket) * pocketWeight +
                                         complexityPenalty + planningNoise(rng);
                if (routeScore < bestScore) {
                    bestScore = routeScore;
                    bestBall = i;
                    bestPocket = pocket;
                    bestDirection = normalized(cueToContact);
                }
            }
        }

        // Quando o grupo já acabou, a IA passa a procurar a 8-ball.
        if (bestBall < 0) {
            for (int i = 1; i < static_cast<int>(balls.size()); ++i) {
                if (!balls[i].pocketed && balls[i].number == 8) {
                    bestBall = i;
                    bestPocket = pockets[static_cast<std::size_t>(i) % pockets.size()];
                    const Vec2 targetToPocket = normalized(bestPocket - balls[i].pos);
                    bestDirection = normalized(balls[i].pos - targetToPocket * (2.0f * BALL_R + 1.0f) - balls[0].pos);
                    break;
                }
            }
        }

        const float styleSkill = aiStyle == 0 ? 0.035f : (aiStyle == 2 ? -0.035f : 0.0f);
        const float skill = clampf(0.42f + static_cast<float>(aiExperience) * 0.004f + styleSkill, 0.38f, 0.86f);
        const float errorRange = (1.0f - skill) * 0.18f;
        std::uniform_real_distribution<float> error(-errorRange, errorRange);
        const float angle = error(rng);
        const Vec2 noisyDirection{
            bestDirection.x * std::cos(angle) - bestDirection.y * std::sin(angle),
            bestDirection.x * std::sin(angle) + bestDirection.y * std::cos(angle)};
        const float distanceToTarget = bestBall >= 0 ? length(balls[bestBall].pos - balls[0].pos) : 240.0f;
        std::uniform_real_distribution<float> powerJitter(0.88f, 1.06f);
        const float stylePower = aiStyle == 0 ? 0.84f : (aiStyle == 2 ? 1.08f : 0.96f);
        const float power = clampf(distanceToTarget * 0.72f * stylePower * powerJitter(rng), 55.0f, 190.0f);
        launchShot(noisyDirection, power);
        (void)bestPocket;
    }

    void update(float dt) {
        if (statusTimer < 8.0f) statusTimer += dt;
        if (screen == ScreenState::Menu) return;
        if (aiThinking) {
            aiTimer += dt;
            if (aiTimer >= aiThinkDuration) executeAIShot();
            return;
        }
        if (!ballsMoving) return;
        for (auto& ball : balls) {
            if (ball.pocketed) continue;
            ball.pos += ball.vel * dt;
            const float speed = length(ball.vel);
            if (speed > 0.0f) ball.vel = ball.vel * std::pow(0.986f, dt * 60.0f);

            for (const Vec2& pocket : pockets) {
                if (length(ball.pos - pocket) < POCKET_R) {
                    pocketBall(ball);
                    break;
                }
            }
            if (ball.pocketed) continue;
            if (ball.pos.x - BALL_R < left) { ball.pos.x = left + BALL_R; ball.vel.x = std::abs(ball.vel.x) * 0.88f; }
            if (ball.pos.x + BALL_R > right) { ball.pos.x = right - BALL_R; ball.vel.x = -std::abs(ball.vel.x) * 0.88f; }
            if (ball.pos.y - BALL_R < top) { ball.pos.y = top + BALL_R; ball.vel.y = std::abs(ball.vel.y) * 0.88f; }
            if (ball.pos.y + BALL_R > bottom) { ball.pos.y = bottom - BALL_R; ball.vel.y = -std::abs(ball.vel.y) * 0.88f; }
        }
        resolveCollisions();

        bool moving = false;
        for (auto& ball : balls) {
            if (!ball.pocketed && length(ball.vel) > 8.0f) moving = true;
            if (!ball.pocketed && length(ball.vel) <= 8.0f) ball.vel = {};
        }
        if (!moving) {
            ballsMoving = false;
            finishTurn();
        }
    }

    void resolveCollisions() {
        for (std::size_t i = 0; i < balls.size(); ++i) {
            if (balls[i].pocketed) continue;
            for (std::size_t j = i + 1; j < balls.size(); ++j) {
                if (balls[j].pocketed) continue;
                Vec2 delta = balls[j].pos - balls[i].pos;
                float distance = length(delta);
                if (distance < 2.0f * BALL_R && distance > 0.001f) {
                    const Vec2 normal = delta * (1.0f / distance);
                    const float overlap = 2.0f * BALL_R - distance;
                    balls[i].pos += normal * (-overlap * 0.5f);
                    balls[j].pos += normal * (overlap * 0.5f);
                    const float approach = dot(balls[j].vel - balls[i].vel, normal);
                    if (approach < 0.0f) {
                        const Vec2 impulse = normal * approach;
                        balls[i].vel += impulse;
                        balls[j].vel -= impulse;
                    }
                }
            }
        }
    }

    void pocketBall(Ball& ball) {
        ball.pocketed = true;
        ball.vel = {};
        if (ball.number == 0) {
            foulThisTurn = true;
            status = "FOUL - CUE BALL SCRATCHED";
            statusTimer = 0.0f;
        } else if (ball.number == 8) {
            const int remaining = remainingFor(currentPlayer);
            if (remaining == 0) {
                gameOver = true;
                status = currentPlayer == 0 ? "PLAYER 1 WINS!" : "PLAYER 2 WINS!";
            } else {
                gameOver = true;
                status = currentPlayer == 0 ? "PLAYER 2 WINS - ILLEGAL 8" : "PLAYER 1 WINS - ILLEGAL 8";
            }
            statusTimer = 0.0f;
        } else {
            pottedThisTurn = true;
            score[currentPlayer] += 10;
            if (playerGroup[currentPlayer] < 0) {
                playerGroup[currentPlayer] = ball.striped ? 1 : 0;
                playerGroup[1 - currentPlayer] = 1 - playerGroup[currentPlayer];
            }
        }
    }

    int remainingFor(int player) const {
        if (playerGroup[player] < 0) return 7;
        int count = 0;
        for (const auto& ball : balls) {
            if (!ball.pocketed && ball.number >= 1 && ball.number <= 15 && ball.number != 8 &&
                (ball.striped ? 1 : 0) == playerGroup[player]) ++count;
        }
        return count;
    }

    void finishTurn() {
        if (gameOver) return;
        const bool aiWasShooting = vsAI && currentPlayer == 1;
        if (cuePocketed()) {
            balls[0].pocketed = false;
            balls[0].pos = {355, 430};
            balls[0].vel = {};
        }
        if (foulThisTurn || !pottedThisTurn) currentPlayer = 1 - currentPlayer;
        if (foulThisTurn) status = "BALL IN HAND - OTHER PLAYER'S TURN";
        else status = pottedThisTurn ? "NICE! KEEP SHOOTING" : "NO SCORE - OTHER PLAYER'S TURN";
        statusTimer = 0.0f;
        if (aiWasShooting) {
            aiExperience = std::min(100, aiExperience + (pottedThisTurn ? 2 : 1));
            saveAIProfile();
        }
        if (vsAI && currentPlayer == 1) beginAIThinking();
    }

    void render() {
        SDL_SetRenderDrawColor(renderer, 8, 13, 22, 255);
        SDL_RenderClear(renderer);
        drawBackground();
        if (screen == ScreenState::Menu) {
            drawMenu();
            SDL_RenderPresent(renderer);
            return;
        }
        drawHeader();
        drawTable();
        drawSidebar();
        if ((currentPlayer == 0 || !vsAI) && !aiThinking &&
            (aiming || (!ballsMoving && !gameOver && !cuePocketed()))) drawAim();
        for (const auto& ball : balls) if (!ball.pocketed) drawBall(ball);
        if (customizer) drawCustomizer();
        SDL_RenderPresent(renderer);
    }

    void drawMenu() {
        SDL_SetRenderDrawColor(renderer, 14, 21, 33, 238);
        SDL_Rect card{150, 110, 980, 590};
        SDL_RenderFillRect(renderer, &card);
        SDL_SetRenderDrawColor(renderer, THEMES[themeIndex].accent.r, THEMES[themeIndex].accent.g,
                               THEMES[themeIndex].accent.b, 255);
        SDL_Rect topLine{150, 110, 980, 4};
        SDL_RenderFillRect(renderer, &topLine);
        drawText(renderer, "HERCULEZ", 640, 170, 7, rgba(247, 249, 250), true);
        drawText(renderer, "POOL CLUB", 640, 235, 3, THEMES[themeIndex].accent, true);
        drawText(renderer, "8 BALL / CHOOSE YOUR MATCH", 640, 295, 2, rgba(155, 169, 184), true);

        SDL_SetRenderDrawColor(renderer, 27, 44, 61, 255);
        SDL_Rect friendButton{235, 420, 350, 82};
        SDL_Rect aiButton{695, 420, 350, 82};
        SDL_RenderFillRect(renderer, &friendButton);
        SDL_RenderFillRect(renderer, &aiButton);
        SDL_SetRenderDrawColor(renderer, THEMES[themeIndex].accent.r, THEMES[themeIndex].accent.g,
                               THEMES[themeIndex].accent.b, 255);
        SDL_RenderDrawRect(renderer, &friendButton);
        SDL_RenderDrawRect(renderer, &aiButton);
        drawText(renderer, "PLAY WITH FRIEND", 410, 442, 2, rgba(243, 245, 238), true);
        drawText(renderer, "PLAY WITH AI", 870, 442, 2, rgba(243, 245, 238), true);
        drawText(renderer, "1", 410, 475, 1, THEMES[themeIndex].accent, true);
        drawText(renderer, "2 / ENTER", 870, 475, 1, THEMES[themeIndex].accent, true);

        const int level = 1 + aiExperience / 20;
        drawText(renderer, "AI PROFILE", 640, 580, 2, rgba(159, 174, 188), true);
        drawText(renderer, ("LEVEL " + std::to_string(level) + "  /  EXPERIENCE " + std::to_string(aiExperience) + "%"),
                 640, 610, 2, rgba(215, 223, 215), true);
        drawText(renderer, "THE AI TAKES ITS TIME, MAKES MISTAKES AND LEARNS", 640, 655, 1, rgba(128, 145, 160), true);
    }

    void drawBackground() {
        for (int y = 0; y < SCREEN_H; y += 4) {
            const Uint8 r = static_cast<Uint8>(8 + y * 4 / SCREEN_H);
            const Uint8 g = static_cast<Uint8>(13 + y * 7 / SCREEN_H);
            const Uint8 b = static_cast<Uint8>(22 + y * 12 / SCREEN_H);
            SDL_SetRenderDrawColor(renderer, r, g, b, 255);
            SDL_Rect band{0, y, SCREEN_W, 4};
            SDL_RenderFillRect(renderer, &band);
        }
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 8);
        for (int i = 0; i < 9; ++i) SDL_RenderDrawLine(renderer, 0, 125 + i * 70, SCREEN_W, 125 + i * 70);
    }

    void drawHeader() {
        SDL_SetRenderDrawColor(renderer, 14, 21, 33, 245);
        SDL_Rect header{0, 0, SCREEN_W, 110};
        SDL_RenderFillRect(renderer, &header);
        SDL_SetRenderDrawColor(renderer, THEMES[themeIndex].accent.r, THEMES[themeIndex].accent.g,
                               THEMES[themeIndex].accent.b, 255);
        SDL_Rect underline{36, 91, 1080, 2};
        SDL_RenderFillRect(renderer, &underline);
        drawText(renderer, "HERCULEZ", 38, 22, 5, rgba(247, 249, 250));
        drawText(renderer, "POOL CLUB", 41, 57, 2, THEMES[themeIndex].accent);
        drawText(renderer, "8 BALL / LOCAL MATCH", 385, 28, 2, rgba(155, 169, 184));
        drawText(renderer, currentPlayer == 0 ? "PLAYER 1 TO BREAK" : (vsAI ? "AI TO PLAY" : "PLAYER 2 TO BREAK"), 385, 55, 3,
                 THEMES[themeIndex].accent);
        SDL_SetRenderDrawColor(renderer, 24, 34, 49, 255);
        SDL_Rect custom{1115, 32, 145, 52};
        SDL_RenderFillRect(renderer, &custom);
        drawRing(renderer, 1135, 58, 13, THEMES[themeIndex].accent);
        drawText(renderer, "C", 1131, 51, 2, THEMES[themeIndex].accent);
        drawText(renderer, "CUSTOMIZE", 1155, 50, 2, rgba(230, 235, 240));
    }

    void drawTable() {
        const Theme& t = THEMES[themeIndex];
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 90);
        SDL_Rect shadow{84, 164, 1030, 532};
        SDL_RenderFillRect(renderer, &shadow);
        SDL_SetRenderDrawColor(renderer, t.rail.r, t.rail.g, t.rail.b, 255);
        SDL_Rect outer{92, 158, 1016, 544};
        SDL_RenderFillRect(renderer, &outer);
        SDL_SetRenderDrawColor(renderer, 139, 92, 48, 255);
        SDL_Rect trim{101, 167, 998, 526};
        SDL_RenderDrawRect(renderer, &trim);
        SDL_SetRenderDrawColor(renderer, t.felt.r, t.felt.g, t.felt.b, 255);
        SDL_Rect felt{static_cast<int>(left), static_cast<int>(top), static_cast<int>(right - left), static_cast<int>(bottom - top)};
        SDL_RenderFillRect(renderer, &felt);
        SDL_SetRenderDrawColor(renderer, t.feltLight.r, t.feltLight.g, t.feltLight.b, 35);
        for (int y = static_cast<int>(top + 10); y < bottom; y += 38) SDL_RenderDrawLine(renderer, left, y, right, y);
        SDL_SetRenderDrawColor(renderer, t.feltLight.r, t.feltLight.g, t.feltLight.b, 150);
        SDL_RenderDrawRect(renderer, &felt);
        for (const auto& pocket : pockets) {
            fillCircle(renderer, static_cast<int>(pocket.x + 3), static_cast<int>(pocket.y + 4), 25, rgba(0, 0, 0, 85));
            fillCircle(renderer, static_cast<int>(pocket.x), static_cast<int>(pocket.y), 22, rgba(4, 7, 10));
            drawRing(renderer, static_cast<int>(pocket.x), static_cast<int>(pocket.y), 23, rgba(209, 157, 73));
        }
        SDL_SetRenderDrawColor(renderer, 223, 221, 184, 100);
        SDL_RenderDrawLine(renderer, 390, top + 2, 390, bottom - 2);
        fillCircle(renderer, 390, 430, 3, rgba(232, 222, 174));
        fillCircle(renderer, 600, 430, 3, rgba(232, 222, 174));
        fillCircle(renderer, 1010, 430, 3, rgba(232, 222, 174));
        drawText(renderer, "BREAK LINE", 310, 682, 1, rgba(204, 204, 180, 130));
    }

    void drawSidebar() {
        SDL_SetRenderDrawColor(renderer, 16, 25, 38, 245);
        SDL_Rect panel{1135, 120, 135, 560};
        SDL_RenderFillRect(renderer, &panel);
        drawText(renderer, "SCOREBOARD", 1144, 139, 2, rgba(159, 174, 188));
        drawText(renderer, "P1", 1147, 180, 2, currentPlayer == 0 ? THEMES[themeIndex].accent : rgba(185, 195, 205));
        drawText(renderer, std::to_string(score[0]), 1224, 176, 3, rgba(245, 247, 249));
        drawText(renderer, playerGroup[0] < 0 ? "-" : (playerGroup[0] == 0 ? "SOLIDS" : "STRIPES"), 1147, 205, 1, rgba(128, 145, 160));
        SDL_SetRenderDrawColor(renderer, 43, 58, 74, 255);
        SDL_RenderDrawLine(renderer, 1145, 233, 1260, 233);
        drawText(renderer, vsAI ? "AI" : "P2", 1147, 253, 2, currentPlayer == 1 ? THEMES[themeIndex].accent : rgba(185, 195, 205));
        drawText(renderer, std::to_string(score[1]), 1224, 249, 3, rgba(245, 247, 249));
        drawText(renderer, playerGroup[1] < 0 ? "-" : (playerGroup[1] == 0 ? "SOLIDS" : "STRIPES"), 1147, 278, 1, rgba(128, 145, 160));
        drawText(renderer, "STATUS", 1147, 330, 2, rgba(159, 174, 188));
        drawText(renderer, gameOver ? "MATCH" : "LIVE", 1147, 355, 2, gameOver ? rgba(224, 91, 82) : rgba(93, 215, 157));
        drawText(renderer, "CONTROLS", 1147, 423, 2, rgba(159, 174, 188));
        drawText(renderer, "DRAG / SHOOT", 1147, 450, 1, rgba(198, 207, 215));
        drawText(renderer, "C  CUSTOMIZE", 1147, 474, 1, rgba(198, 207, 215));
        drawText(renderer, "N  NEW MATCH", 1147, 498, 1, rgba(198, 207, 215));
        drawText(renderer, "ESC MENU", 1147, 522, 1, rgba(198, 207, 215));
        drawText(renderer, themeIndex == 0 ? "EMERALD FELT" : (themeIndex == 1 ? "MIDNIGHT FELT" : "ROYAL FELT"),
                 1147, 614, 1, THEMES[themeIndex].accent);
    }

    void drawAim() {
        if (cuePocketed()) return;
        const Vec2 shotDirection = aimMouse - balls[0].pos;
        const float power = clampf(length(shotDirection), 0.0f, 190.0f);
        const Vec2 direction = normalized(shotDirection);
        if (length(shotDirection) < 0.1f) return;
        const std::array<SDL_Color, 3> cueColors = {rgba(211, 170, 90), rgba(205, 208, 213), rgba(190, 92, 71)};
        const SDL_Color cueColor = cueColors[cueStyle];
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

        // Taco e mao estilizados: a mao acompanha o recuo do taco durante a mira.
        const float cueLength = 78.0f + power * 0.32f;
        const Vec2 cueTip = balls[0].pos - direction * 17.0f;
        const Vec2 cueButt = balls[0].pos - direction * cueLength;
        const Vec2 hand = balls[0].pos - direction * (cueLength + 6.0f);
        SDL_SetRenderDrawColor(renderer, cueColor.r, cueColor.g, cueColor.b, 245);
        SDL_RenderDrawLine(renderer, static_cast<int>(cueTip.x), static_cast<int>(cueTip.y),
                           static_cast<int>(cueButt.x), static_cast<int>(cueButt.y));
        SDL_SetRenderDrawColor(renderer, 244, 238, 220, 255);
        SDL_RenderDrawLine(renderer, static_cast<int>(cueTip.x), static_cast<int>(cueTip.y),
                           static_cast<int>(cueTip.x - direction.x * 8.0f),
                           static_cast<int>(cueTip.y - direction.y * 8.0f));
        fillCircle(renderer, static_cast<int>(hand.x), static_cast<int>(hand.y), 9, rgba(212, 145, 103, 235));
        const Vec2 perpendicular{-direction.y, direction.x};
        SDL_SetRenderDrawColor(renderer, 230, 172, 130, 240);
        for (int finger = -1; finger <= 1; ++finger) {
            const Vec2 fingerStart = hand + perpendicular * static_cast<float>(finger * 3) - direction * 4.0f;
            const Vec2 fingerEnd = fingerStart - direction * 12.0f;
            SDL_RenderDrawLine(renderer, static_cast<int>(fingerStart.x), static_cast<int>(fingerStart.y),
                               static_cast<int>(fingerEnd.x), static_cast<int>(fingerEnd.y));
        }

        SDL_SetRenderDrawColor(renderer, 240, 241, 211, 125);
        SDL_RenderDrawLine(renderer, static_cast<int>(balls[0].pos.x), static_cast<int>(balls[0].pos.y),
                           static_cast<int>(balls[0].pos.x + direction.x * 265),
                           static_cast<int>(balls[0].pos.y + direction.y * 265));
        SDL_SetRenderDrawColor(renderer, cueColor.r, cueColor.g, cueColor.b, 230);
        SDL_RenderDrawLine(renderer, static_cast<int>(balls[0].pos.x - direction.x * 18),
                           static_cast<int>(balls[0].pos.y - direction.y * 18),
                           static_cast<int>(balls[0].pos.x - direction.x * (18 + power * 0.55f)),
                           static_cast<int>(balls[0].pos.y - direction.y * (18 + power * 0.55f)));
        SDL_SetRenderDrawColor(renderer, 247, 238, 183, 220);
        SDL_RenderDrawLine(renderer, static_cast<int>(balls[0].pos.x - direction.x * 18),
                           static_cast<int>(balls[0].pos.y - direction.y * 18),
                           static_cast<int>(balls[0].pos.x - direction.x * 22),
                           static_cast<int>(balls[0].pos.y - direction.y * 22));
        drawRing(renderer, static_cast<int>(balls[0].pos.x + direction.x * 265),
                 static_cast<int>(balls[0].pos.y + direction.y * 265), 4, rgba(247, 238, 183, 190));

        // Previsao simples da primeira colisao: mostra a linha que a bola atingida
        // provavelmente seguira, como nos jogos de 8-ball.
        int hitIndex = -1;
        float hitDistance = 1000000.0f;
        for (int i = 1; i < static_cast<int>(balls.size()); ++i) {
            if (balls[i].pocketed) continue;
            const Vec2 toBall = balls[i].pos - balls[0].pos;
            const float along = dot(toBall, direction);
            if (along <= 0.0f) continue;
            const float side = std::abs(toBall.x * direction.y - toBall.y * direction.x);
            if (side > BALL_R * 2.0f) continue;
            const float reach = std::sqrt(std::max(0.0f, 4.0f * BALL_R * BALL_R - side * side));
            const float collisionDistance = along - reach;
            if (collisionDistance > 0.0f && collisionDistance < hitDistance) {
                hitDistance = collisionDistance;
                hitIndex = i;
            }
        }
        if (hitIndex >= 0) {
            const Vec2 contact = balls[0].pos + direction * hitDistance;
            const Vec2 targetDirection = normalized(balls[hitIndex].pos - contact);
            const Vec2 predictedEnd = balls[hitIndex].pos + targetDirection * 140.0f;
            SDL_SetRenderDrawColor(renderer, 111, 224, 186, 205);
            SDL_RenderDrawLine(renderer, static_cast<int>(balls[hitIndex].pos.x),
                               static_cast<int>(balls[hitIndex].pos.y), static_cast<int>(predictedEnd.x),
                               static_cast<int>(predictedEnd.y));
            fillCircle(renderer, static_cast<int>(balls[hitIndex].pos.x + targetDirection.x * 70.0f),
                       static_cast<int>(balls[hitIndex].pos.y + targetDirection.y * 70.0f), 8,
                       rgba(111, 224, 186, 45));
            drawRing(renderer, static_cast<int>(balls[hitIndex].pos.x), static_cast<int>(balls[hitIndex].pos.y),
                     17, rgba(111, 224, 186, 200));
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
            drawText(renderer, "BALL PATH", static_cast<int>(predictedEnd.x - 25),
                     static_cast<int>(predictedEnd.y - 14), 1, rgba(155, 240, 204));
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        }
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
        drawText(renderer, aiming ? "POWER" : "AIM", 115, 718, 2, rgba(214, 222, 204));
        SDL_SetRenderDrawColor(renderer, 35, 49, 56, 255);
        SDL_Rect powerBar{185, 718, 180, 10};
        SDL_RenderFillRect(renderer, &powerBar);
        SDL_SetRenderDrawColor(renderer, THEMES[themeIndex].accent.r, THEMES[themeIndex].accent.g, THEMES[themeIndex].accent.b, 255);
        SDL_Rect powerFill{185, 718, static_cast<int>(180 * power / 190.0f), 10};
        SDL_RenderFillRect(renderer, &powerFill);
        drawText(renderer, status, 600, 724, 2, gameOver ? rgba(241, 118, 106) : rgba(215, 223, 215), true);
    }

    void drawBall(const Ball& ball) {
        const int x = static_cast<int>(ball.pos.x);
        const int y = static_cast<int>(ball.pos.y);
        fillCircle(renderer, x + 3, y + 4, 14, rgba(0, 0, 0, 85));
        const SDL_Color base = ball.number == 0 ? rgba(236, 237, 223) : BALL_COLORS[(ball.number - 1) % 7];
        fillCircle(renderer, x, y, 13, base);
        if (ball.striped) {
            SDL_SetRenderDrawColor(renderer, 242, 240, 226, 255);
            SDL_Rect stripe{x - 13, y - 4, 26, 8};
            SDL_RenderFillRect(renderer, &stripe);
        }
        fillCircle(renderer, x - 4, y - 5, 3, rgba(255, 255, 255, 145));
        drawRing(renderer, x, y, 13, rgba(255, 255, 255, 85));
        if (ball.number != 0) {
            fillCircle(renderer, x, y, 5, rgba(248, 248, 240));
            drawText(renderer, std::to_string(ball.number), x, y - 3, 1, rgba(23, 27, 31), true);
        }
    }

    void drawCustomizer() {
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 3, 6, 12, 190);
        SDL_Rect veil{0, 0, SCREEN_W, SCREEN_H};
        SDL_RenderFillRect(renderer, &veil);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
        SDL_SetRenderDrawColor(renderer, 18, 29, 45, 255);
        SDL_Rect card{180, 105, 920, 650};
        SDL_RenderFillRect(renderer, &card);
        SDL_SetRenderDrawColor(renderer, THEMES[themeIndex].accent.r, THEMES[themeIndex].accent.g, THEMES[themeIndex].accent.b, 255);
        SDL_Rect topLine{180, 105, 920, 4};
        SDL_RenderFillRect(renderer, &topLine);
        drawText(renderer, "CUSTOMIZE YOUR CLUB", 640, 135, 3, rgba(246, 246, 237), true);
        drawText(renderer, "TABLE FINISH", 230, 220, 2, rgba(157, 174, 186));
        for (int i = 0; i < 3; ++i) {
            const int x = 230 + i * 125;
            SDL_SetRenderDrawColor(renderer, THEMES[i].felt.r, THEMES[i].felt.g, THEMES[i].felt.b, 255);
            SDL_Rect swatch{x, 265, 105, 58};
            SDL_RenderFillRect(renderer, &swatch);
            if (themeIndex == i) {
                SDL_SetRenderDrawColor(renderer, THEMES[i].accent.r, THEMES[i].accent.g, THEMES[i].accent.b, 255);
                SDL_RenderDrawRect(renderer, &swatch);
                SDL_Rect inner{x + 3, 268, 99, 52};
                SDL_RenderDrawRect(renderer, &inner);
            }
            drawText(renderer, THEMES[i].name, x + 52, 284, 1, rgba(239, 241, 232), true);
            drawText(renderer, std::to_string(i + 1), x + 52, 335, 1, rgba(157, 174, 186), true);
        }
        drawText(renderer, "CUE SKIN", 230, 370, 2, rgba(157, 174, 186));
        const std::array<SDL_Color, 3> cueColors = {rgba(211, 170, 90), rgba(205, 208, 213), rgba(190, 92, 71)};
        const std::array<const char*, 3> cueNames = {"CLASSIC", "CHROME", "CARMINE"};
        for (int i = 0; i < 3; ++i) {
            const int x = 230 + i * 125;
            SDL_SetRenderDrawColor(renderer, 32, 46, 60, 255);
            SDL_Rect swatch{x, 415, 105, 58};
            SDL_RenderFillRect(renderer, &swatch);
            SDL_SetRenderDrawColor(renderer, cueColors[i].r, cueColors[i].g, cueColors[i].b, 255);
            SDL_RenderDrawLine(renderer, x + 16, 460, x + 88, 428);
            SDL_RenderDrawLine(renderer, x + 18, 462, x + 90, 430);
            if (cueStyle == i) SDL_RenderDrawRect(renderer, &swatch);
            drawText(renderer, cueNames[i], x + 52, 485, 1, rgba(239, 241, 232), true);
            drawText(renderer, std::string(1, static_cast<char>('Q' + i)), x + 52, 505, 1, rgba(157, 174, 186), true);
        }

        // Perfil da IA: todos os controles abaixo alteram a tomada de decisao real.
        drawText(renderer, "AI PROFILE", 650, 220, 2, THEMES[themeIndex].accent);
        drawText(renderer, "STYLE", 650, 255, 2, rgba(157, 174, 186));
        const std::array<const char*, 3> styleNames = {"SAFE", "BALANCED", "BOLD"};
        for (int i = 0; i < 3; ++i) {
            const int x = 650 + i * 125;
            SDL_SetRenderDrawColor(renderer, 32, 46, 60, 255);
            SDL_Rect styleBox{x, 285, 105, 55};
            SDL_RenderFillRect(renderer, &styleBox);
            if (aiStyle == i) {
                SDL_SetRenderDrawColor(renderer, THEMES[themeIndex].accent.r, THEMES[themeIndex].accent.g,
                                       THEMES[themeIndex].accent.b, 255);
                SDL_RenderDrawRect(renderer, &styleBox);
            }
            drawText(renderer, styleNames[i], x + 52, 304, 1, rgba(239, 241, 232), true);
            drawText(renderer, std::string(1, static_cast<char>('A' + i)), x + 52, 325, 1, rgba(157, 174, 186), true);
        }

        drawText(renderer, "EXPERIENCE / ACCURACY", 650, 375, 2, rgba(157, 174, 186));
        SDL_SetRenderDrawColor(renderer, 32, 46, 60, 255);
        SDL_Rect experienceBar{715, 425, 230, 12};
        SDL_RenderFillRect(renderer, &experienceBar);
        SDL_SetRenderDrawColor(renderer, THEMES[themeIndex].accent.r, THEMES[themeIndex].accent.g,
                               THEMES[themeIndex].accent.b, 255);
        SDL_Rect experienceFill{715, 425, static_cast<int>(230 * aiExperience / 100.0f), 12};
        SDL_RenderFillRect(renderer, &experienceFill);
        SDL_SetRenderDrawColor(renderer, 32, 46, 60, 255);
        SDL_Rect minus{650, 410, 52, 48};
        SDL_Rect plus{955, 410, 52, 48};
        SDL_RenderFillRect(renderer, &minus);
        SDL_RenderFillRect(renderer, &plus);
        drawText(renderer, "-", 676, 425, 2, rgba(239, 241, 232), true);
        drawText(renderer, "+", 981, 424, 2, rgba(239, 241, 232), true);
        drawText(renderer, std::to_string(aiExperience) + "%", 830, 450, 2, rgba(239, 241, 232), true);
        drawText(renderer, "MORE EXPERIENCE = BETTER AIM AND ROUTES", 650, 475, 1, rgba(128, 195, 168));

        drawText(renderer, "THINKING PACE", 650, 505, 2, rgba(157, 174, 186));
        const std::array<const char*, 3> paceNames = {"PATIENT", "STANDARD", "QUICK"};
        const std::array<const char*, 3> paceKeys = {"Z", "X", "V"};
        for (int i = 0; i < 3; ++i) {
            const int x = 650 + i * 125;
            SDL_SetRenderDrawColor(renderer, 32, 46, 60, 255);
            SDL_Rect paceBox{x, 525, 105, 50};
            SDL_RenderFillRect(renderer, &paceBox);
            if (aiTempo == i) {
                SDL_SetRenderDrawColor(renderer, THEMES[themeIndex].accent.r, THEMES[themeIndex].accent.g,
                                       THEMES[themeIndex].accent.b, 255);
                SDL_RenderDrawRect(renderer, &paceBox);
            }
            drawText(renderer, paceNames[i], x + 52, 542, 1, rgba(239, 241, 232), true);
            drawText(renderer, paceKeys[i], x + 52, 560, 1, rgba(157, 174, 186), true);
        }
        drawText(renderer, "SAFE: SHORT ROUTES   BOLD: RISKY ROUTES", 650, 610, 1, rgba(128, 145, 160));

        SDL_SetRenderDrawColor(renderer, 32, 48, 64, 255);
        SDL_Rect close{900, 680, 160, 45};
        SDL_RenderFillRect(renderer, &close);
        drawText(renderer, "DONE / ESC", 980, 696, 2, rgba(224, 232, 235), true);
    }
};

int main(int argc, char* argv[]) {
    PoolGame game;
    if (!game.init()) {
        SDL_Log("Nao foi possivel iniciar o jogo: %s", SDL_GetError());
        return EXIT_FAILURE;
    }
    game.run();
    return EXIT_SUCCESS;
}
