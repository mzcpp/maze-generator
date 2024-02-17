#include "Game.hpp"
#include "Maze.hpp"
#include "Constants.hpp"

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include <iostream>
#include <thread>

Game::Game() : 
	initialized_(false), 
	running_(false), 
	ticks_(0), 
	maze_(nullptr), 
	window_(nullptr), 
	renderer_(nullptr)
{
	initialized_ = Initialize();

	maze_ = std::make_unique<Maze>(this);
}

Game::~Game()
{
	Finalize();
}

bool Game::Initialize()
{
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		printf("SDL could not be initialized! SDL Error: %s\n", SDL_GetError());
		return false;
	}

	if (!SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0"))
	{
		printf("%s\n", "Warning: Texture filtering is not enabled!");
	}

	window_ = SDL_CreateWindow(constants::game_title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, constants::screen_width, constants::screen_height, SDL_WINDOW_SHOWN);

	if (window_ == nullptr)
	{
		printf("Window could not be created! SDL Error: %s\n", SDL_GetError());
		return false;
	}

	renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_ACCELERATED);

	if (renderer_ == nullptr)
	{
		printf("Renderer could not be created! SDL Error: %s\n", SDL_GetError());
		return false;
	}

	constexpr int img_flags = IMG_INIT_PNG;

	SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND);

	if (!(IMG_Init(img_flags) & img_flags))
	{
		printf("SDL_image could not be initialized! SDL_image Error: %s\n", IMG_GetError());
		return false;
	}

	return true;
}

void Game::Finalize()
{
	SDL_DestroyWindow(window_);
	window_ = nullptr;
	
	SDL_DestroyRenderer(renderer_);
	renderer_ = nullptr;

	SDL_Quit();
	IMG_Quit();
}

void Game::Run()
{
	if (!initialized_)
	{
		return;
	}

	running_ = true;

	constexpr double ms = 1.0 / 60.0;
	std::uint64_t last_time = SDL_GetPerformanceCounter();
	long double delta = 0.0;

	double timer = SDL_GetTicks();

	int frames = 0;
	int ticks = 0;

	while (running_)
	{
		const std::uint64_t now = SDL_GetPerformanceCounter();
		const long double elapsed = static_cast<long double>(now - last_time) / static_cast<long double>(SDL_GetPerformanceFrequency());

		last_time = now;
		delta += elapsed;

		HandleEvents();

		while (delta >= ms)
		{
			Tick();
			delta -= ms;
			++ticks;
		}

		//printf("%Lf\n", delta / ms);
		Render();
		++frames;

		if (SDL_GetTicks() - timer > 1000.0)
		{
			timer += 1000.0;
			//printf("Frames: %d, Ticks: %d\n", frames, ticks);
			frames = 0;
			ticks = 0;
		}
	}
}

void Game::HandleEvents()
{
	SDL_Event e;

	while (SDL_PollEvent(&e) != 0)
	{
		if (e.type == SDL_QUIT)
		{
			running_ = false;
			return;
		}

		if (e.type == SDL_KEYDOWN)
		{
			if (e.key.keysym.sym == SDLK_t)
			{
				maze_->ResetBoard();
				Timer timer;
				std::cout << "Parallel testing has started!" << '\n';

				std::thread t1(&Maze::TestRecursiveBacktracker, Maze());
				std::thread t2(&Maze::TestHuntAndKill, Maze());
				std::thread t3(&Maze::TestWilsons, Maze());
				std::thread t4(&Maze::TestRandomizedKruskals, Maze());
				std::thread t5(&Maze::TestPrimSimplified, Maze());

				t1.join();
				t2.join();
				t3.join();
				t4.join();
				t5.join();

				std::cout << "Parallel testing has ended!" << '\n';
				std::cout << "It took " << timer.elapsed() << " seconds\n";
			}
			if (e.key.keysym.sym == SDLK_y)
			{
				maze_->ResetBoard();
				Timer timer;
				std::cout << "Sequential testing has started!" << '\n';

				std::unique_ptr<Maze> maze_ptr = std::make_unique<Maze>(this);

				maze_ptr->TestRecursiveBacktracker();
				maze_ptr->TestHuntAndKill();
				maze_ptr->TestWilsons();
				maze_ptr->TestRandomizedKruskals();
				maze_ptr->TestPrimSimplified();

				std::cout << "Sequential testing has ended!" << '\n';
				std::cout << "It took " << timer.elapsed() << " seconds\n";
			}
		}

		maze_->HandleEvent(&e);
	}
}

void Game::Tick()
{
	++ticks_;

	maze_->Tick();
}

void Game::Render()
{
	SDL_RenderSetViewport(renderer_, NULL);
	SDL_SetRenderDrawColor(renderer_, 0x00, 0x00, 0x00, 0xff);
	SDL_RenderClear(renderer_);

	maze_->Render();

	SDL_RenderPresent(renderer_);
}