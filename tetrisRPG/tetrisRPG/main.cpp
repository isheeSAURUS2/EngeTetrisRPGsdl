#include <SDL.h> 
#include <iostream>
#include <vector>
#include <ctime>
#include <cstdlib>
#include <algorithm>

const int BLOCK_SIZE = 20;
const int GRID_WIDTH = 10;
const int GRID_HEIGHT = 25;
const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;
const int PLAYFIELD_LEFT = 200;
const int PLAYFIELD_TOP = 100;

SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;

int linesCleared = 0;

struct Point {
    int x, y;
};

struct Tetrimino {
    std::vector<std::vector<Point>> rotations;
    int rotationIndex = 0;
    Point position;
    SDL_Color color;

    std::vector<Point> getCurrentShape() const {
        std::vector<Point> shape = rotations[rotationIndex];
        for (auto& p : shape) {
            p.x += position.x;
            p.y += position.y;
        }
        return shape;
    }

    void rotate() {
        rotationIndex = (rotationIndex + 1) % rotations.size();
    }

    void unrotate() {
        rotationIndex = (rotationIndex - 1 + rotations.size()) % rotations.size();
    }
};

std::vector<std::vector<Point>> shapes[7] = {
    // I
    {
        {{0,1}, {1,1}, {2,1}, {3,1}},
        {{2,0}, {2,1}, {2,2}, {2,3}}
    },
    // O
    {
        {{1,0}, {2,0}, {1,1}, {2,1}}
    },
    // T
    {
        {{1,0}, {0,1}, {1,1}, {2,1}},
        {{1,0}, {1,1}, {2,1}, {1,2}},
        {{0,1}, {1,1}, {2,1}, {1,2}},
        {{1,0}, {0,1}, {1,1}, {1,2}}
    },
    // S
    {
        {{1,0}, {2,0}, {0,1}, {1,1}},
        {{1,0}, {1,1}, {2,1}, {2,2}}
    },
    // Z
    {
        {{0,0}, {1,0}, {1,1}, {2,1}},
        {{2,0}, {1,1}, {2,1}, {1,2}}
    },
    // J
    {
        {{0,0}, {0,1}, {1,1}, {2,1}},
        {{1,0}, {2,0}, {1,1}, {1,2}},
        {{0,1}, {1,1}, {2,1}, {2,2}},
        {{1,0}, {1,1}, {0,2}, {1,2}}
    },
    // L
    {
        {{2,0}, {0,1}, {1,1}, {2,1}},
        {{1,0}, {1,1}, {1,2}, {2,2}},
        {{0,1}, {1,1}, {2,1}, {0,2}},
        {{0,0}, {1,0}, {1,1}, {1,2}}
    }
};

SDL_Color tetriminoColors[7] = {
    {0, 255, 255, 255},   // I
    {255, 255, 0, 255},   // O
    {128, 0, 128, 255},   // T
    {0, 255, 0, 255},     // S
    {255, 0, 0, 255},     // Z
    {0, 0, 255, 255},     // J
    {255, 165, 0, 255}    // L
};

std::vector<std::vector<SDL_Color>> grid(GRID_HEIGHT, std::vector<SDL_Color>(GRID_WIDTH, { 0, 0, 0, 0 }));
Tetrimino current;

std::vector<int> baseShapesBag = { 0, 1, 2, 3, 4, 5, 6, 5, 6 };
std::vector<int> shapesBag;

Tetrimino createRandomTetrimino() {
    if (shapesBag.empty()) {
        shapesBag = baseShapesBag;
        std::random_shuffle(shapesBag.begin(), shapesBag.end());
    }

    int shapeIndex = rand() % shapesBag.size();
    int shapeId = shapesBag[shapeIndex];
    shapesBag.erase(shapesBag.begin() + shapeIndex);

    Tetrimino t;
    t.rotations = shapes[shapeId];
    t.color = tetriminoColors[shapeId];
    t.position = { 3, 0 };
    return t;
}

bool isValidPosition(const Tetrimino& t) {
    for (const auto& block : t.getCurrentShape()) {
        if (block.x < 0 || block.x >= GRID_WIDTH || block.y >= GRID_HEIGHT) return false;
        if (block.y >= 0 && grid[block.y][block.x].a != 0) return false;
    }
    return true;
}

void lockPiece(const Tetrimino& t) {
    for (const auto& block : t.getCurrentShape()) {
        if (block.y >= 0)
            grid[block.y][block.x] = t.color;
    }
}

void clearLines() {
    int linesThisTurn = 0;
    for (int y = GRID_HEIGHT - 1; y >= 0; --y) {
        bool full = true;
        for (int x = 0; x < GRID_WIDTH; ++x) {
            if (grid[y][x].a == 0) {
                full = false;
                break;
            }
        }
        if (full) {
            ++linesThisTurn;
            for (int row = y; row > 0; --row) {
                grid[row] = grid[row - 1];
            }
            grid[0] = std::vector<SDL_Color>(GRID_WIDTH, { 0, 0, 0, 0 });
            ++y;
        }
    }
    linesCleared += linesThisTurn;
    if (linesThisTurn > 0) {
        std::cout << "Lines cleared this turn: " << linesThisTurn << "\n";
        std::cout << "Total lines cleared: " << linesCleared << "\n";
    }
}

void drawBlock(int x, int y, SDL_Color color) {
    SDL_Rect r = {
        PLAYFIELD_LEFT + x * BLOCK_SIZE,
        PLAYFIELD_TOP + y * BLOCK_SIZE,
        BLOCK_SIZE,
        BLOCK_SIZE
    };
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderFillRect(renderer, &r);
}
void drawGhostBlock(int x, int y, SDL_Color color) {
	SDL_Rect r = {
		PLAYFIELD_LEFT + x * BLOCK_SIZE,
		PLAYFIELD_TOP + y * BLOCK_SIZE,
		BLOCK_SIZE,
		BLOCK_SIZE
	};
	SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, 64); // Semi-transparent
	SDL_RenderFillRect(renderer, &r);
}

void render() {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    for (int y = 0; y < GRID_HEIGHT; ++y) {
        for (int x = 0; x < GRID_WIDTH; ++x) {
            if (grid[y][x].a != 0) {
                drawBlock(x, y, grid[y][x]);
            }
        }
    }
    Tetrimino ghost = current;
    while (true) {
        Tetrimino trial = ghost;
        ++trial.position.y;
        if (isValidPosition(trial)) {
            ghost = trial;
        }
        else {
            break;
        }
    }

    for(const auto& p : ghost.getCurrentShape()) {
		drawGhostBlock(p.x, p.y, ghost.color);
	}
    for (const auto& p : current.getCurrentShape()) {
        drawBlock(p.x, p.y, current.color);
    }
    

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    for (int x = 0; x < GRID_WIDTH; ++x) {
        SDL_Rect floorRect = {
            PLAYFIELD_LEFT + x * BLOCK_SIZE,
            PLAYFIELD_TOP + GRID_HEIGHT * BLOCK_SIZE,
            BLOCK_SIZE,
            BLOCK_SIZE
        };
        SDL_RenderFillRect(renderer, &floorRect);
    }

    for (int y = 0; y < GRID_HEIGHT; ++y) {
        SDL_Rect leftWall = {
            PLAYFIELD_LEFT - BLOCK_SIZE,
            PLAYFIELD_TOP + y * BLOCK_SIZE,
            BLOCK_SIZE,
            BLOCK_SIZE
        };
        SDL_Rect rightWall = {
            PLAYFIELD_LEFT + GRID_WIDTH * BLOCK_SIZE,
            PLAYFIELD_TOP + y * BLOCK_SIZE,
            BLOCK_SIZE,
            BLOCK_SIZE
        };
        SDL_RenderFillRect(renderer, &leftWall);
        SDL_RenderFillRect(renderer, &rightWall);
    }

    SDL_RenderPresent(renderer);
}

int main(int argc, char* argv[]) {
    srand(static_cast<unsigned>(time(nullptr)));
    SDL_Init(SDL_INIT_VIDEO);
    window = SDL_CreateWindow("Tetris", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, 0);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    current = createRandomTetrimino();
    Uint32 lastTick = SDL_GetTicks();
    bool running = true;

    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = false;
            if (e.type == SDL_KEYDOWN) {
                Tetrimino trial = current;
                switch (e.key.keysym.sym) {
                case SDLK_LEFT:
                    --trial.position.x;
                    if (isValidPosition(trial)) current = trial;
                    break;
                case SDLK_RIGHT:
                    ++trial.position.x;
                    if (isValidPosition(trial)) current = trial;
                    break;
                case SDLK_DOWN:
                    ++trial.position.y;
                    if (isValidPosition(trial)) current = trial;
                    break;
                case SDLK_UP:
                    trial.rotate();
                    if (isValidPosition(trial)) current = trial;
                    break;
                }
                switch (e.key.keysym.sym) {
                case SDLK_a:
                    --trial.position.x;
                    if (isValidPosition(trial)) current = trial;
                    break;
                case SDLK_d:
                    ++trial.position.x;
                    if (isValidPosition(trial)) current = trial;
                    break;
                case SDLK_s:
                    ++trial.position.y;
                    if (isValidPosition(trial)) current = trial;
                    break;
                case SDLK_w:
                    trial.rotate();
                    if (isValidPosition(trial)) current = trial;
                    break;
                }
            }
        }

        if (SDL_GetTicks() - lastTick > 500) {
            lastTick = SDL_GetTicks();
            Tetrimino trial = current;
            ++trial.position.y;
            if (isValidPosition(trial)) {
                current = trial;
            }
            else {
                lockPiece(current);
                clearLines();
                current = createRandomTetrimino();
                if (!isValidPosition(current)) {
                    std::cout << "Game Over! Total lines cleared: " << linesCleared << "\n";
                    break;
                }
            }
        }

        render();
        SDL_Delay(16);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}