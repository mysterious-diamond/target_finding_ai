#include <algorithm>
#include <array>
#include <cmath>
#include <format>
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
#define WIN_SCORE 10000
#define MAX_SPEED 5.0f

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
    std::uniform_real_distribution<float> distrib_x(
        targetSize / 2, SCREEN_WIDTH - targetSize / 2);
    std::uniform_real_distribution<float> distrib_y(
        targetSize / 2, SCREEN_HEIGHT - targetSize / 2);

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
    this->pos = {this->distrib_x(this->gen) - this->targetSize / 2,
                 this->distrib_y(this->gen) - this->targetSize / 2};
}

Vector2 Target::getPos() { return this->pos; }

class Player {
   public:
    Player(float x, float y, float playerSize);
    void move(float dx, float dy);
    void draw();
    float getScore(Vector2 targetPos, float targetSize);
    Vector2 getDistance(Vector2 targetPos, float targetSize);

   private:
    Vector2 curPos;
    Vector2 lastPos;
    float playerSize;
    double timeSinceLastTarget;
};

Player::Player(float x, float y, float playerSize) {
    this->curPos = {x, y};
    this->lastPos = this->curPos;
    this->playerSize = playerSize;
    this->timeSinceLastTarget = 0.0;
}

void Player::move(float dx, float dy) {
    this->lastPos = curPos;
    float newX = this->curPos.x + this->playerSize / 2 + dx;
    float newY = this->curPos.y + this->playerSize / 2 + dy;
    this->curPos = {newX - this->playerSize / 2, newY - this->playerSize / 2};
}

void Player::draw() {
    DrawRectangle(
        std::clamp(this->curPos.x + this->playerSize / 2, this->playerSize / 2,
                   (float)SCREEN_WIDTH - this->playerSize / 2) -
            this->playerSize / 2,
        std::clamp(this->curPos.y + this->playerSize / 2, this->playerSize / 2,
                   (float)SCREEN_HEIGHT - this->playerSize / 2) -
            this->playerSize / 2,
        this->playerSize, this->playerSize, RED);
}

float Player::getScore(Vector2 targetPos, float targetSize) {
    if (GetTime() - timeSinceLastTarget > 3.0) {
        this->timeSinceLastTarget = GetTime();
        return -100.0f;
    }
    float currentDistance =
        std::sqrt(std::pow((this->curPos.x + this->playerSize / 2) -
                               (targetPos.x + targetSize / 2),
                           2) +
                  std::pow((this->curPos.y + this->playerSize / 2) -
                               (targetPos.y + targetSize / 2),
                           2));
    if (currentDistance <= this->playerSize / 2 + targetSize / 2) {
        this->timeSinceLastTarget = GetTime();
        return WIN_SCORE;
    }

    float lastDistance =
        std::sqrt(std::pow((this->lastPos.x + this->playerSize / 2) -
                               (targetPos.x + targetSize / 2),
                           2) +
                  std::pow((this->lastPos.y + this->playerSize / 2) -
                               (targetPos.y + targetSize / 2),
                           2));
    float difference = lastDistance - currentDistance;
    return difference * 20 - 1.0f;
}

Vector2 Player::getDistance(Vector2 targetPos, float targetSize) {
    float dx = (this->curPos.x + this->playerSize / 2) -
               (targetPos.x + targetSize / 2);

    float dy = (this->curPos.y + this->playerSize / 2) -
               (targetPos.y + targetSize / 2);

    return {dx, dy};
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

std::array<float, 2> handle_ai_step(HANDLE pipe, Vector2 dist) {
    DWORD bytes_written;
    float data[2] = {dist.x / (float)SCREEN_WIDTH,
                     dist.y / (float)SCREEN_HEIGHT};
    WriteFile(pipe, data, sizeof(float) * 2, &bytes_written, NULL);

    float result[2] = {};
    DWORD bytes_read;
    bool _ = ReadFile(pipe, result, sizeof(float) * 2, &bytes_read, NULL);

    return {result[0], result[1]};
}

int main() {
    bool isManual = false;
    int wins = 0;
    std::cout << "Are you training AI or are you manually playing? (0 for "
                 "training, 1 for manual)\n";
    std::cin >> isManual;
    Player player =
        Player((float)SCREEN_WIDTH / 2, (float)SCREEN_HEIGHT / 2, PLAYERSIZE);

    Target target = Target(TARGET_SIZE);
    HANDLE pipe;
    if (!isManual) pipe = establish_pipe_connection();

    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Path Finding AI");
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(GRAY);
        DrawText(std::format("You have {} wins!", wins).c_str(),
                 SCREEN_WIDTH / 2 -
                     MeasureText(std::format("You have {} wins!", wins).c_str(),
                                 40) /
                         2,
                 50, 40, WHITE);

        player.draw();
        target.draw();

        if (IsKeyDown(KEY_W) && isManual) {
            player.move(0, -200 * GetFrameTime());
        }
        if (IsKeyDown(KEY_S) && isManual) {
            player.move(0, 200 * GetFrameTime());
        }
        if (IsKeyDown(KEY_A) && isManual) {
            player.move(-200 * GetFrameTime(), 0);
        }
        if (IsKeyDown(KEY_D) && isManual) {
            player.move(200 * GetFrameTime(), 0);
        }

        if (!isManual) {
            Vector2 dist = player.getDistance(target.getPos(), TARGET_SIZE);
            std::array<float, 2> result = handle_ai_step(pipe, dist);
            player.move(result[0] * MAX_SPEED, result[1] * MAX_SPEED);
        }
        float score = player.getScore(target.getPos(), TARGET_SIZE);

        if (!isManual) {
            DWORD bytes_written;
            WriteFile(pipe, &score, sizeof(float), &bytes_written, NULL);
        }

        if (score == WIN_SCORE) {
            wins++;
            target.place();
        } else if (score == -100.0f) {
            target.place();
        }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
