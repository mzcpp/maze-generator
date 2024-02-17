#include "Game.hpp"
#include "Maze.hpp"
#include "Constants.hpp"

#include <SDL2/SDL.h>

#include <cstdint>
#include <iostream>
#include <cmath>
#include <cassert>
#include <stack>
#include <vector>
#include <deque>
#include <set>
#include <random>
#include <ctime>
#include <string>
#include <mutex>

Maze::Maze(Game* game) : 
    game_(game), 
    test_loops_(5'000), 
    shift_pressed_(false), 
	left_mouse_button_pressed_(false), 
	cell_size_(128), 
	cells_width_(constants::screen_width / cell_size_), 
	cells_height_(constants::screen_height / cell_size_), 
	custom_maze_current_cell_(nullptr), 
	start_cell_(nullptr), 
	end_cell_(nullptr), 
	shortest_path_found_(false)
{
    board_.resize(cells_width_ * cells_height_);
	ResetBoard();

	std::srand(std::time(0));
	std::rand();

	mouse_position_ = { 0, 0 };
}

Maze::~Maze()
{

}

void Maze::HandleEvent(SDL_Event* e)
{
    SDL_GetMouseState(&mouse_position_.x, &mouse_position_.y);

    mouse_position_.x = std::clamp(mouse_position_.x, 0, constants::screen_width - cell_size_);
    mouse_position_.y = std::clamp(mouse_position_.y, 0, constants::screen_height - cell_size_);

    const std::size_t index = (mouse_position_.y / cell_size_) * cells_width_ + (mouse_position_.x / cell_size_);

    if (e->type == SDL_MOUSEMOTION)
    {
        if (shift_pressed_ && left_mouse_button_pressed_)
        {
            const int neighbor_index = GetNeighborIndex(*custom_maze_current_cell_, board_[index]);

            if (neighbor_index != -1 && custom_maze_current_cell_ != nullptr && &board_[index] != custom_maze_current_cell_)
            {
                const std::size_t maze_cell_index = GetCellIndex(*custom_maze_current_cell_);
                const std::vector<Cell*> neighbors = GetNeighborCells(maze_cell_index);

                SetConnections(custom_maze_current_cell_, neighbors, neighbor_index);
                
                if (DetectCycleDepthFirstSearch(&board_[index]))
                {
                    SetConnections(custom_maze_current_cell_, neighbors, neighbor_index, true);
                }
                
                custom_maze_current_cell_ = &board_[index];
            }

        }
    }

    if (e->type == SDL_MOUSEBUTTONDOWN)
    {
        if (e->button.button == SDL_BUTTON_LEFT)
        {
            left_mouse_button_pressed_ = true;

            if (shift_pressed_)
            {
                custom_maze_current_cell_ = &board_[index];
            }

            if (end_cell_ != &board_[index] && !shift_pressed_)
            {
                if (start_cell_ != &board_[index])
                {
                    start_cell_ = &board_[index];
                    shortest_path_found_ = FindShortestPathBetweenStartEnd();
                }
                else
                {
                    start_cell_ = nullptr;
                    shortest_path_found_ = false;
                }
            }
        }
        if (e->button.button == SDL_BUTTON_RIGHT)
        {
            if (start_cell_ != &board_[index] && !shift_pressed_)
            {
                if (end_cell_ != &board_[index])
                {
                    end_cell_ = &board_[index];
                    shortest_path_found_ = FindShortestPathBetweenStartEnd();
                }
                else
                {
                    end_cell_ = nullptr;
                    shortest_path_found_ = false;
                }
            }
        }
    }
    else if (e->type == SDL_MOUSEBUTTONUP)
    {
        if (e->button.button == SDL_BUTTON_LEFT)
        {
            left_mouse_button_pressed_ = false;
            custom_maze_current_cell_ = nullptr;
        }
    }

    if (e->type == SDL_KEYDOWN)
    {
        std::string new_title = "";

        if (e->key.keysym.sym == SDLK_1)
        {
            GenerateMazeRecursiveBacktracker();
            new_title = std::string(constants::game_title) + " - Recursive backtracker algorithm.";
            SDL_SetWindowTitle(game_->window_, new_title.c_str());
        }
        else if (e->key.keysym.sym == SDLK_2)
        {
            GenerateMazeHuntAndKill();
            new_title = std::string(constants::game_title) + " - Hunt and kill algorithm.";
            SDL_SetWindowTitle(game_->window_, new_title.c_str());
        }
        else if (e->key.keysym.sym == SDLK_3)
        {
            GenerateMazeWilsons();
            new_title = std::string(constants::game_title) + " - Wilson's algorithm.";
            SDL_SetWindowTitle(game_->window_, new_title.c_str());
        }
        else if (e->key.keysym.sym == SDLK_4)
        {
            GenerateMazeRandomizedKruskal();
            new_title = std::string(constants::game_title) + " - Randomized Kruskal's algorithm.";
            SDL_SetWindowTitle(game_->window_, new_title.c_str());
        }
        else if (e->key.keysym.sym == SDLK_5)
        {
            GenerateMazePrimSimplified();
            new_title = std::string(constants::game_title) + " - Prim's simplified algorithm.";
            SDL_SetWindowTitle(game_->window_, new_title.c_str());
        }
        else if (e->key.keysym.sym == SDLK_UP)
        {
            SetCellSize(cell_size_ / 2);
        }
        else if (e->key.keysym.sym == SDLK_DOWN)
        {
            SetCellSize(cell_size_ * 2);
        }
        else if (e->key.keysym.sym == SDLK_a)
        {		
            FindLongestPathInMaze();
        }
        else if (e->key.keysym.sym == SDLK_r)
        {
            ResetBoard();
        }

        if (e->key.keysym.sym == SDLK_LSHIFT)
        {
            shift_pressed_ = true;

            if (left_mouse_button_pressed_)
            {
                custom_maze_current_cell_ = &board_[index];
            }
        }
    }
    else if (e->type == SDL_KEYUP)
    {
        if (e->key.keysym.sym == SDLK_LSHIFT)
        {
            shift_pressed_ = false;
            custom_maze_current_cell_ = nullptr;
        }
    }
}

void Maze::Tick()
{

}

void Maze::Render()
{
    RenderCells();
}

int Maze::GetRandomNeighborIndex(const std::vector<Cell*>& neighbors, bool unvisited)
{
	assert(neighbors.size() == 4);

	const int valid_neighbors_count = std::count_if(neighbors.begin(), neighbors.end(), [unvisited](Cell* neighbor)
		{
			return neighbor != nullptr && (!unvisited || !neighbor->visited_);
		});

	if (valid_neighbors_count == 0)
	{
		return -1;
	}

	int random_neighbour_index = std::rand() % 4;

	while (neighbors[random_neighbour_index] == nullptr || (unvisited && neighbors[random_neighbour_index]->visited_))
	{
		random_neighbour_index = (random_neighbour_index + 1) % 4;
	}

	return random_neighbour_index;
}

void Maze::SetConnections(Cell* current_cell, const std::vector<Cell*>& neighbors, std::size_t neighbor_index, bool unset)
{
	assert(current_cell != nullptr);

	switch (neighbor_index)
		{
		case 0:
			current_cell->left_edge_.destination_cell_ = unset ? nullptr : neighbors[neighbor_index];
			neighbors[neighbor_index]->right_edge_.destination_cell_ = unset ? nullptr : current_cell;
			break;

		case 1:
			current_cell->right_edge_.destination_cell_ = unset ? nullptr : neighbors[neighbor_index];
			neighbors[neighbor_index]->left_edge_.destination_cell_ = unset ? nullptr : current_cell;
			break;
		
		case 2:
			current_cell->top_edge_.destination_cell_ = unset ? nullptr : neighbors[neighbor_index];
			neighbors[neighbor_index]->bottom_edge_.destination_cell_ = unset ? nullptr : current_cell;
			break;
		
		case 3:
			current_cell->bottom_edge_.destination_cell_ = unset ? nullptr : neighbors[neighbor_index];
			neighbors[neighbor_index]->top_edge_.destination_cell_ = unset ? nullptr : current_cell;
			break;

		default:
			assert(false);
		}
}

std::size_t Maze::GetCellIndex(const Cell& cell)
{
	return (cell.rect_.y / cell_size_) * cells_width_ + (cell.rect_.x / cell_size_);
}

void Maze::GenerateMazeRecursiveBacktracker()
{
	ResetBoard();

	std::stack<Cell*> cell_stack;
	cell_stack.push(&board_[std::rand() % board_.size()]);
	cell_stack.top()->visited_ = true;

	while (!cell_stack.empty())
	{
		Cell* const stack_top = cell_stack.top();
		const std::vector<Cell*> neighbors = GetNeighborCells(GetCellIndex(*stack_top));
		const int random_neighbour_index = GetRandomNeighborIndex(neighbors, true);

		if (random_neighbour_index == -1)
		{
			cell_stack.pop();
			continue;
		}

		SetConnections(stack_top, neighbors, random_neighbour_index);
		cell_stack.push(neighbors[random_neighbour_index]);
		cell_stack.top()->visited_ = true;
	}
}

void Maze::GenerateMazeHuntAndKill()
{
	ResetBoard();

	Cell* current_cell = &board_[std::rand() % board_.size()];
	current_cell->visited_ = true;
	
	while (true)
	{
		std::vector<Cell*> neighbors = GetNeighborCells(GetCellIndex(*current_cell));
		const int random_neighbour_index = GetRandomNeighborIndex(neighbors, true);

		if (random_neighbour_index == -1)
		{
			bool found_new_cell = false;
			bool found_visited_neighbor = false;

			for (Cell& cell : board_)
			{
				if (cell.visited_)
				{
					continue;
				}

				neighbors = GetNeighborCells(GetCellIndex(cell));
				
				for (std::size_t index = 0; index < neighbors.size(); ++index)
				{
					if (neighbors[index] != nullptr && neighbors[index]->visited_)
					{
						SetConnections(&cell, neighbors, index);
						found_visited_neighbor = true;
						break;
					}
				}

				if (found_visited_neighbor)
				{
					current_cell = &cell;
					current_cell->visited_ = true;
					found_new_cell = true;
					break;
				}
			}

			if (!found_new_cell)
			{
				return;
			}
		}
		else
		{
			SetConnections(current_cell, neighbors, random_neighbour_index);
			current_cell = neighbors[random_neighbour_index];
			current_cell->visited_ = true;
		}
	}
}

void Maze::GenerateMazeWilsons()
{
	ResetBoard();

	Cell* target_cell = &board_[std::rand() % board_.size()];
	target_cell->visited_ = true;
	std::size_t unvisited_cells = board_.size() - 1;
	std::vector<Cell*> carving_path;

	while (unvisited_cells != 0)
	{
		if (carving_path.empty())
		{
			std::size_t random_index = std::rand() % board_.size();

			while (board_[random_index].visited_)
			{
				random_index = std::rand() % board_.size();
			}

			board_[random_index].seen_ = true;
			carving_path.push_back(&board_[random_index]);
		}

		std::vector<Cell*> neighbors = GetNeighborCells(GetCellIndex(*carving_path.back()));
		int random_neighbour_index = GetRandomNeighborIndex(neighbors, false);

		if (neighbors[random_neighbour_index]->visited_)
		{
			carving_path.push_back(neighbors[random_neighbour_index]);
			carving_path.back()->seen_ = true;

			for (std::size_t i = 0; i < carving_path.size() - 1; ++i)
			{
				carving_path[i]->visited_ = true;
				carving_path[i]->seen_ = false;
				SetConnections(carving_path[i], GetNeighborCells(GetCellIndex(*carving_path[i])), GetNeighborIndex(*carving_path[i], *carving_path[i + 1]));
				--unvisited_cells;
			}

			carving_path.clear();
		}
		else
		{
			if (neighbors[random_neighbour_index]->seen_)
			{
				Cell* cycle_begin = neighbors[random_neighbour_index];

				while (carving_path.back() != cycle_begin)
				{
					carving_path.back()->seen_ = false;
					carving_path.pop_back();
				}
			}
			else
			{
				carving_path.push_back(neighbors[random_neighbour_index]);
				carving_path.back()->seen_ = true;
			}
		}
	}
}

void Maze::GenerateMazeRandomizedKruskal()
{
	ResetBoard();

	std::vector<std::set<Cell* const>> set_vector;

	for (Cell& cell : board_)
	{
		set_vector.push_back( { &cell } );
	}

	std::size_t non_empty_sets_remaining = board_.size();

	while (non_empty_sets_remaining != 1)
	{
		const std::size_t random_index = std::rand() % board_.size();
		Cell* random_cell = &board_[random_index];
		std::vector<Cell*> neighbors = GetNeighborCells(random_index);
		const int random_neighbour_index = GetRandomNeighborIndex(neighbors, false);
		Cell* random_neighbor = neighbors[random_neighbour_index];

		const auto cell_set_it = std::find_if(set_vector.begin(), set_vector.end(), [random_cell](const std::set<Cell* const>& set)
		{
			return set.find(random_cell) != set.end();
		});

		/* Neighboring cells are not in the same set. */
		if (cell_set_it->find(random_neighbor) == cell_set_it->end())
		{
			const auto neighbor_set_it = std::find_if(set_vector.begin(), set_vector.end(), [random_neighbor](const std::set<Cell* const>& set)
			{
				return set.find(random_neighbor) != set.end();
			});

			cell_set_it->merge(*neighbor_set_it);
			SetConnections(random_cell, neighbors, random_neighbour_index);
			--non_empty_sets_remaining;
		}
	}
}
	
void Maze::GenerateMazePrimSimplified()
{
	ResetBoard();

	std::vector<Cell*> visited_cells = { &board_[std::rand() % board_.size()] };		
	visited_cells.front()->visited_ = true;

	while (visited_cells.size() != board_.size())
	{
		Cell* random_visited_cell = visited_cells[std::rand() % visited_cells.size()];
		std::vector<Cell*> neighbors = GetNeighborCells(GetCellIndex(*random_visited_cell));
		int random_unvisited_neighbor_index = GetRandomNeighborIndex(neighbors, true);
		
		while (random_unvisited_neighbor_index == -1)
		{
			random_visited_cell = visited_cells[std::rand() % visited_cells.size()];
			neighbors = GetNeighborCells(GetCellIndex(*random_visited_cell));
			random_unvisited_neighbor_index = GetRandomNeighborIndex(neighbors, true);
		}
		
		SetConnections(random_visited_cell, neighbors, random_unvisited_neighbor_index);
		visited_cells.push_back(neighbors[random_unvisited_neighbor_index]);
		visited_cells.back()->visited_ = true;
	}
}

void Maze::BreadthFirstSearch(Cell* start_cell)
{
	for (Cell& cell : board_)
	{
		cell.visited_ = false;
	}

	bfs_cells_predecessors_.clear();
	bfs_cells_predecessors_.resize(board_.size());
	bfs_cells_distances_.clear();
	bfs_cells_distances_.resize(board_.size());
	std::fill(bfs_cells_distances_.begin(), bfs_cells_distances_.end(), 0);

	const std::size_t random_index = std::rand() % board_.size();
	std::deque<Cell*> bfs_queue = { (start_cell == nullptr) ? &board_[random_index] : start_cell };
	
	while (!bfs_queue.empty())
	{
		Cell* current_cell = bfs_queue.front();
		current_cell->visited_ = true;
		bfs_queue.pop_front();

		for (Cell* cell : GetConnectedNeighborCells(*current_cell))
		{
			if (cell == nullptr || cell->visited_)
			{
				continue;
			}

			std::size_t cell_index = GetCellIndex(*cell);
			bfs_cells_predecessors_[cell_index] = current_cell;
			bfs_cells_distances_[cell_index] = bfs_cells_distances_[GetCellIndex(*current_cell)] + 1;
			bfs_queue.push_back(cell);
		}
	}
}

bool Maze::DetectCycleDepthFirstSearch(Cell* start_cell)
{
	for (Cell& board_cell : board_)
	{
		board_cell.visited_ = false;
	}

	std::vector<Cell*> dfs_cells_parents_(board_.size());
	std::fill(dfs_cells_parents_.begin(), dfs_cells_parents_.end(), nullptr);

	std::vector<bool> dfs_cells_discovering_(board_.size());
	std::fill(dfs_cells_discovering_.begin(), dfs_cells_discovering_.end(), false);

	std::stack<Cell*> cell_stack;
	cell_stack.push((start_cell == nullptr) ? &board_[std::rand() % board_.size()] : start_cell);
	dfs_cells_parents_[GetCellIndex(*cell_stack.top())] = nullptr;
	dfs_cells_discovering_[GetCellIndex(*cell_stack.top())] = true;
	Cell* current_cell = nullptr;

	while (!cell_stack.empty())
	{
		current_cell = cell_stack.top();
		current_cell->visited_ = true;
		cell_stack.pop();

		for (Cell* cell : GetConnectedNeighborCells(*current_cell))
		{
			if (cell == nullptr)
			{
				continue;
			}

			const std::size_t cell_index = GetCellIndex(*cell);

			if (dfs_cells_parents_[cell_index] == nullptr && !cell->visited_)
			{
				dfs_cells_parents_[cell_index] = current_cell;
			}

			if (cell->visited_)
			{
				if (dfs_cells_parents_[GetCellIndex(*current_cell)] != cell)
				{
					return true;
				}
				else
				{
					continue;
				}
			}
			else if (!dfs_cells_discovering_[cell_index])
			{
				cell_stack.push(cell);
				dfs_cells_discovering_[cell_index] = true;
			}
		}
	}

	return false;
}

bool Maze::FindShortestPathBetweenStartEnd()
{
	if (start_cell_ == nullptr || end_cell_ == nullptr)
	{
		return false;
	}

	for (Cell& cell : board_)
	{
		cell.visited_ = false;
	}

	bfs_cells_predecessors_.clear();
	bfs_cells_predecessors_.resize(board_.size());
	bfs_cells_distances_.clear();
	bfs_cells_distances_.resize(board_.size());
	std::deque<Cell*> bfs_queue = { start_cell_ };

	while (!bfs_queue.empty())
	{
		Cell* current_cell = bfs_queue.front();
		current_cell->visited_ = true;
		bfs_queue.pop_front();

		for (Cell* cell : GetConnectedNeighborCells(*current_cell))
		{
			if (cell == nullptr || cell->visited_)
			{
				continue;
			}

			bfs_cells_predecessors_[GetCellIndex(*cell)] = current_cell;

			if (cell == end_cell_)
			{
				return true;
			}
			
			bfs_queue.push_back(cell);
		}
	}

	return false;
}
	
bool Maze::FindLongestPathInMaze()
{
	start_cell_ = nullptr;
	end_cell_ = nullptr;

	BreadthFirstSearch(nullptr);
	const std::size_t start_index = std::max_element(bfs_cells_distances_.begin(), bfs_cells_distances_.end()) - bfs_cells_distances_.begin();
	BreadthFirstSearch(&board_[start_index]);
	const std::size_t end_index = std::max_element(bfs_cells_distances_.begin(), bfs_cells_distances_.end()) - bfs_cells_distances_.begin();

	if (start_index == end_index)
	{
		return false;
	}
	
	start_cell_ = &board_[start_index];
	end_cell_ = &board_[end_index];
	shortest_path_found_ = FindShortestPathBetweenStartEnd();
	
	return true;
}

void Maze::ResetBoard()
{
    if (game_ != nullptr)
    {
	    SDL_SetWindowTitle(game_->window_, constants::game_title);
    }

	for (int y = 0; y < cells_height_; ++y)
	{
		for (int x = 0; x < cells_width_; ++x)
		{
			const int index = y * cells_width_ + x;

			board_[index].rect_.x = x * cell_size_;
			board_[index].rect_.y = y * cell_size_;
			board_[index].rect_.w = cell_size_;
			board_[index].rect_.h = board_[index].rect_.w;

			board_[index].left_edge_.destination_cell_ = nullptr;
			board_[index].left_edge_.weight_ = 0;
			board_[index].right_edge_.destination_cell_ = nullptr;
			board_[index].right_edge_.weight_ = 0;
			board_[index].top_edge_.destination_cell_ = nullptr;
			board_[index].top_edge_.weight_ = 0;
			board_[index].bottom_edge_.destination_cell_ = nullptr;
			board_[index].bottom_edge_.weight_ = 0;

			board_[index].visited_ = false;
			board_[index].seen_ = false;
			board_[index].weight_ = 0;
		}
	}
	
	bfs_cells_predecessors_.clear();
	std::fill(bfs_cells_distances_.begin(), bfs_cells_distances_.end(), 0);
	start_cell_ = nullptr;
	end_cell_ = nullptr;
	shortest_path_found_ = false;
}

void Maze::GenerateEdgesWeights()
{

}

void Maze::SetCellSize(std::size_t new_size)
{
	constexpr std::size_t max_size = 128;
	constexpr std::size_t min_size = 32;
	
	if (constants::screen_width % new_size != 0 || constants::screen_height % new_size != 0 || new_size > max_size || new_size < min_size)
	{
		return;
	}

	cell_size_ = new_size;
	cells_width_ = constants::screen_width / cell_size_;
	cells_height_ = constants::screen_height / cell_size_;

	board_.resize(cells_width_ * cells_height_);
	ResetBoard();
}

void Maze::RenderCells()
{
	SDL_SetRenderDrawColor(game_->renderer_, 0xff, 0xff, 0xff, 0xff);
	
	constexpr int padding = 1;

	for (const Cell& cell : board_)
	{
		if (&cell == start_cell_)
		{
			SDL_SetRenderDrawColor(game_->renderer_, 0x00, 0xff, 0x00, 0xff);
		}
		else if (&cell == end_cell_)
		{
			SDL_SetRenderDrawColor(game_->renderer_, 0xff, 0x00, 0x00, 0xff);
		}
		
		SDL_RenderFillRect(game_->renderer_, &cell.rect_);
		SDL_SetRenderDrawColor(game_->renderer_, 0x00, 0x00, 0x00, 0xff);

		SDL_Rect wall;

		if (cell.left_edge_.destination_cell_ == nullptr)
		{
			wall = cell.rect_;
			wall.w = padding;
			SDL_RenderFillRect(game_->renderer_, &wall);
		}

		if (cell.right_edge_.destination_cell_ == nullptr)
		{
			wall = cell.rect_;
			wall.x = cell.rect_.x + cell.rect_.w - padding;
			SDL_RenderFillRect(game_->renderer_, &wall);
		}

		if (cell.top_edge_.destination_cell_ == nullptr)
		{
			wall = cell.rect_;
			wall.h = padding;
			SDL_RenderFillRect(game_->renderer_, &wall);
		}

		if (cell.bottom_edge_.destination_cell_ == nullptr)
		{
			wall = cell.rect_;
			wall.y = cell.rect_.y + cell.rect_.h - padding;
			SDL_RenderFillRect(game_->renderer_, &wall);
		}

		SDL_SetRenderDrawColor(game_->renderer_, 0xff, 0xff, 0xff, 0xff);
	}

	SDL_SetRenderDrawColor(game_->renderer_, 0xff, 0x00, 0x00, 0xff);

	if (shortest_path_found_)
	{
		SDL_SetRenderDrawColor(game_->renderer_, 0x00, 0x00, 0xff, 0xff);
		const double offset = (cell_size_ / 2);

		Cell* current = end_cell_;

		while (current != start_cell_)
		{
			const std::size_t current_index = GetCellIndex(*current);

			if (bfs_cells_predecessors_[current_index] == nullptr)
			{
				break;
			}

			SDL_RenderDrawLine(game_->renderer_, board_[current_index].rect_.x + offset, board_[current_index].rect_.y + offset, bfs_cells_predecessors_[current_index]->rect_.x + offset, bfs_cells_predecessors_[current_index]->rect_.y + offset);
			current = bfs_cells_predecessors_[current_index];
		}
	}

	// for (Cell& cell : board_)
	// {
	// 	const double offset = (cell_size_ / 2);
		
	// 	if (cell.left_edge_.destination_cell_ != nullptr)
	// 	{
	// 		SDL_RenderDrawLine(game_->renderer_, cell.rect_.x + offset, cell.rect_.y + offset, cell.left_edge_.destination_cell_->rect_.x + offset, cell.left_edge_.destination_cell_->rect_.y + offset);
	// 	}

	// 	if (cell.right_edge_.destination_cell_ != nullptr)
	// 	{
	// 		SDL_RenderDrawLine(game_->renderer_, cell.rect_.x + offset, cell.rect_.y + offset, cell.right_edge_.destination_cell_->rect_.x + offset, cell.right_edge_.destination_cell_->rect_.y + offset);
	// 	}

	// 	if (cell.top_edge_.destination_cell_ != nullptr)
	// 	{
	// 		SDL_RenderDrawLine(game_->renderer_, cell.rect_.x + offset, cell.rect_.y + offset, cell.top_edge_.destination_cell_->rect_.x + offset, cell.top_edge_.destination_cell_->rect_.y + offset);
	// 	}

	// 	if (cell.bottom_edge_.destination_cell_ != nullptr)
	// 	{
	// 		SDL_RenderDrawLine(game_->renderer_, cell.rect_.x + offset, cell.rect_.y + offset, cell.bottom_edge_.destination_cell_->rect_.x + offset, cell.bottom_edge_.destination_cell_->rect_.y + offset);
	// 	}
	// }
}

void Maze::PrintDistancesAndPredecessors()
{
	for (std::size_t i = 0; i < bfs_cells_distances_.size(); ++i)
	{
		std::cout << bfs_cells_distances_[i] << "\t";

		if ((i + 1) % cells_width_ == 0)
		{
			std::cout << std::endl;
		}
	}

	std::cout << std::endl;

	for (std::size_t i = 0; i < bfs_cells_predecessors_.size(); ++i)
	{
		if (bfs_cells_predecessors_[i])
		{
			std::cout << GetCellIndex(*bfs_cells_predecessors_[i]) << "\t";
		}
		else
		{
			std::cout << "X" << "\t";
		}

		if ((i + 1) % cells_width_ == 0)
		{
			std::cout << std::endl;
		}
	}
}

std::vector<Cell*> Maze::GetConnectedNeighborCells(const Cell& current_cell)
{
	return { current_cell.left_edge_.destination_cell_, current_cell.right_edge_.destination_cell_, current_cell.top_edge_.destination_cell_, current_cell.bottom_edge_.destination_cell_ };
}

int Maze::GetNeighborIndex(const Cell& current_cell, const Cell& neighbor_cell)
{
	std::size_t current_index = GetCellIndex(current_cell);
	std::size_t neighbor_index = GetCellIndex(neighbor_cell);

	if (neighbor_index + 1 == current_index)
	{
		return 0;
	}
	else if (neighbor_index - 1 == current_index)
	{
		return 1;
	}
	else if (neighbor_index + cells_width_ == current_index)
	{
		return 2;
	}
	else if (neighbor_index - cells_width_ == current_index)
	{
		return 3;
	}

	return -1;
}

std::vector<Cell*> Maze::GetNeighborCells(std::size_t cell_index)
{
	return { GetLeftNeighbor(cell_index), GetRightNeighbor(cell_index), GetTopNeighbor(cell_index), GetBottomNeighbor(cell_index) };
}

Cell* Maze::GetLeftNeighbor(std::size_t cell_index)
{
	if ((board_[cell_index].rect_.x - cell_size_) >= 0)
	{
		return &board_[cell_index - 1];
	}

	return nullptr;
}
	
Cell* Maze::GetRightNeighbor(std::size_t cell_index)
{
	if ((board_[cell_index].rect_.x + cell_size_) < constants::screen_width)
	{
		return &board_[cell_index + 1];
	}
	
	return nullptr;
}

Cell* Maze::GetTopNeighbor(std::size_t cell_index)
{
	if ((board_[cell_index].rect_.y - cell_size_) >= 0)
	{
		return &board_[cell_index - cells_width_];
	}
	
	return nullptr;
}

Cell* Maze::GetBottomNeighbor(std::size_t cell_index)
{
	if ((board_[cell_index].rect_.y + cell_size_) < constants::screen_height)
	{
		return &board_[cell_index + cells_width_];
	}

	return nullptr;
}

void Maze::TestRecursiveBacktracker()
{
    for (std::size_t i = 0; i < test_loops_; ++i)
    {
        GenerateMazeRecursiveBacktracker();
                        
        if (DetectCycleDepthFirstSearch(nullptr))
        {
            printf("%s\n" ,"Cycle detected in recursive backtracker algorithm!");
        }
    }
}
    
void Maze::TestHuntAndKill()
{
    for (std::size_t i = 0; i < test_loops_; ++i)
    {
        GenerateMazeHuntAndKill();
                        
        if (DetectCycleDepthFirstSearch(nullptr))
        {
            printf("%s\n" ,"Cycle detected in hunt and kill algorithm!");
        }
    }
}

void Maze::TestWilsons()
{
    for (std::size_t i = 0; i < test_loops_; ++i)
    {
        GenerateMazeWilsons();
                        
        if (DetectCycleDepthFirstSearch(nullptr))
        {
            printf("%s\n" ,"Cycle detected in Wilson's algorithm!");
        }
    }
}

void Maze::TestRandomizedKruskals()
{
    for (std::size_t i = 0; i < test_loops_; ++i)
    {
        GenerateMazeRandomizedKruskal();
                        
        if (DetectCycleDepthFirstSearch(nullptr))
        {
            printf("%s\n" ,"Cycle detected in randomized Kruskal's algorithm!");
        }
    }
}

void Maze::TestPrimSimplified()
{
    for (std::size_t i = 0; i < test_loops_; ++i)
    {
        GenerateMazePrimSimplified();
                        
        if (DetectCycleDepthFirstSearch(nullptr))
        {
            printf("%s\n" ,"Cycle detected in Prim's simplified algorithm!");
        }
    }
}