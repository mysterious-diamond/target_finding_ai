#include <cmath>
#include <iostream>
#include <random>

#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#define NOGDI
#define NOUSER

#include <windows.h>

#include "raylib.h"

#define SCREEN_HEIGHT 800
#define SCREEN_WIDTH 1000
#define PLAYERSIZE 30
#define TARGET_SIZE 30
#define WIN_SCORE 1000

class Target {
   public:
    Target(float targetSize);
    void draw();
    void place();
    Vector2 getPos();

   private:
    float targetSize;
    Vector2 pos;
    std::mt19937 gen;
    std::uniform_real_distribution<float> distrib_x;
    std::uniform_real_distribution<float> distrib_y;
};

Target::Target(float targetSize) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> distrib_x(0, SCREEN_WIDTH);
    std::uniform_real_distribution<float> distrib_y(0, SCREEN_HEIGHT);

    this->pos = {distrib_x(gen), distrib_y(gen)};
    this->targetSize = targetSize;
    this->gen = gen;
    this->distrib_x = distrib_x;
    this->distrib_y = distrib_y;
}

void Target::draw() {
    DrawRectangle(this->pos.x, this->pos.y, this->targetSize, this->targetSize,
                  GREEN);
}

void Target::place() {
    this->pos = {this->distrib_x(this->gen), this->distrib_y(this->gen)};
}

Vector2 Target::getPos() { return this->pos; }

class Player {
   public:
    Player(float x, float y, float playerSize);
    void move(float dx, float dy);
    void draw();
    float getScore(Vector2 targetPos, float targetSize);

   private:
    Vector2 curPos;
    Vector2 lastPos;
    float playerSize;
};

Player::Player(float x, float y, float playerSize) {
    this->curPos = {x, y};
    this->lastPos = this->curPos;
    this->playerSize = playerSize;
}

void Player::move(float dx, float dy) {
    this->lastPos = curPos;
    this->curPos = {this->curPos.x + dx, this->curPos.y + dy};
}

void Player::draw() {
    DrawRectangle(this->curPos.x, this->curPos.y, this->playerSize,
                  this->playerSize, RED);
}

float Player::getScore(Vector2 targetPos, float targetSize) {
    float currentDistance =
        std::sqrt(std::pow((this->curPos.x + this->playerSize / 2) -
                               (targetPos.x + targetSize / 2),
                           2) +
                  std::pow((this->curPos.y + this->playerSize / 2) -
                               (targetPos.y + targetSize / 2),
                           2));
    if (currentDistance <= this->playerSize) return WIN_SCORE;

    float lastDistance =
        std::sqrt(std::pow((this->lastPos.x + this->playerSize / 2) -
                               (targetPos.x + targetSize / 2),
                           2) +
                  std::pow((this->lastPos.y + this->playerSize / 2) -
                               (targetPos.y + targetSize / 2),
                           2));
    float difference = lastDistance - currentDistance;
    return difference * 10;
}

HANDLE establish_pipe_connection() {
    std::cout << "Connecting pipe...\n";
    HANDLE pipe = CreateFile(R"(\\.\pipe\target-ai)", GENERIC_ALL,
                             FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                             OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (pipe == INVALID_HANDLE_VALUE) {
        std::cout << "Failed to connect to pipe\n";
        system("pause");
        exit(1);
    }

    std::cout << "Pipe connected succesfully.\n";
    return pipe;
}

int main() {
    Player player =
        Player((float)SCREEN_WIDTH / 2, (float)SCREEN_HEIGHT / 2, PLAYERSIZE);

    Target target = Target(TARGET_SIZE);
    HANDLE pipe = establish_pipe_connection();

    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Path Finding AI");
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(GRAY);
        player.draw();
        target.draw();

        if (IsKeyDown(KEY_W)) {
            player.move(0, -200 * GetFrameTime());
        }
        if (IsKeyDown(KEY_S)) {
            player.move(0, 200 * GetFrameTime());
        }
        if (IsKeyDown(KEY_A)) {
            player.move(-200 * GetFrameTime(), 0);
        }
        if (IsKeyDown(KEY_D)) {
            player.move(200 * GetFrameTime(), 0);
        }

        float score = player.getScore(target.getPos(), TARGET_SIZE);
        if (score == WIN_SCORE) target.place();

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
