#ifndef GAME_HPP
#define GAME_HPP

#include "Maze.hpp"

#include <SDL2/SDL.h>

#include <chrono>
#include <memory>

class Timer
{
private:
    using clock = std::chrono::high_resolution_clock;
    using second = std::chrono::duration<double, std::ratio<1>>;

    std::chrono::time_point<clock> start;
public:
    Timer() : start(clock::now()) {}

    void reset()
	{
        start = clock::now();
    }

    double elapsed() const
	{
        return std::chrono::duration_cast<second>(clock::now() - start).count();
    }
};

class Game
{
private:
	bool initialized_;
	bool running_;
	int ticks_;

	std::unique_ptr<Maze> maze_;

public:
	SDL_Window* window_;
	SDL_Renderer* renderer_;

	Game();

	~Game();

	bool Initialize();

	void Finalize();

	void Run();

	void HandleEvents();
	
	void Tick();

	void Render();
};

#endif