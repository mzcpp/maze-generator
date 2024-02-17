#ifndef MAZE_HPP
#define MAZE_HPP

#include <SDL2/SDL.h>

#include <vector>
#include <mutex>

struct Cell;
class Game;

struct CellEdge
{
	Cell* destination_cell_;
	int weight_;
};

struct Cell
{
	SDL_Rect rect_;

	CellEdge left_edge_;
	CellEdge right_edge_;
	CellEdge top_edge_;
	CellEdge bottom_edge_;

	bool visited_;
	bool seen_;
	int weight_;
};

class Maze
{
private:
    Game* game_;
    std::size_t test_loops_;
    bool shift_pressed_;
    bool left_mouse_button_pressed_;

    int cell_size_;
    int cells_width_;
    int cells_height_;
    
    std::vector<Cell> board_;

    std::vector<Cell*> bfs_cells_predecessors_;
    std::vector<int> bfs_cells_distances_;
    Cell* custom_maze_current_cell_;
    Cell* start_cell_;
    Cell* end_cell_;
    bool shortest_path_found_;
	SDL_Point mouse_position_;

public:
    Maze(Game* game = nullptr);

    ~Maze();

    void HandleEvent(SDL_Event* e);

    void Tick();

    void Render();

    int GetRandomNeighborIndex(const std::vector<Cell*>& neighbors, bool unvisited);

    void SetConnections(Cell* current_cell, const std::vector<Cell*>& neighbors, std::size_t neighbor_index, bool unset = false);

    std::size_t GetCellIndex(const Cell& cell);

    void GenerateMazeRecursiveBacktracker();
    
    void GenerateMazeHuntAndKill();

    void GenerateMazeWilsons();
    
    void GenerateMazeRandomizedKruskal();
    
    void GenerateMazePrimSimplified();

    void BreadthFirstSearch(Cell* start_cell);

    bool DetectCycleDepthFirstSearch(Cell* start_cell);

    bool FindShortestPathBetweenStartEnd();
    
    bool FindLongestPathInMaze();

    void ResetBoard();

    void GenerateEdgesWeights();

    void SetCellSize(std::size_t size);

    void RenderGrid();
		
    void RenderCells();

    int GetNeighborIndex(const Cell& current_cell, const Cell& neighbor_cell);

    void PrintDistancesAndPredecessors();

    std::vector<Cell*> GetConnectedNeighborCells(const Cell& current_cell);

    std::vector<Cell*> GetNeighborCells(std::size_t cell_index);

    Cell* GetLeftNeighbor(std::size_t cell_index);
    
    Cell* GetRightNeighbor(std::size_t cell_index);
    
    Cell* GetTopNeighbor(std::size_t cell_index);
    
    Cell* GetBottomNeighbor(std::size_t cell_index);
    
    void TestRecursiveBacktracker();
    
    void TestHuntAndKill();

    void TestWilsons();

    void TestRandomizedKruskals();

    void TestPrimSimplified();
};

#endif
