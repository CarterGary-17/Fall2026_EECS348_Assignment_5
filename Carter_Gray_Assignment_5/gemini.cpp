/*
Name: EECS 348 Assignment - Object-Oriented Sudoku Solver
Description: An object-oriented C++ program that solves 9x9 Sudoku puzzles using
    recursive depth-first search (DFS) with backtracking. Reads five puzzle files,
    validates their formatting and initial board state, and prints either all valid
    solutions or "No solution found".
Input: Five text files (puzzle1.txt, puzzle2.txt, puzzle3.txt, puzzle4.txt, puzzle5.txt) containing
    9x9 space-separated grids of digits ('1'-'9') and underscores ('_') for empty cells.
Output: Console output displaying the filename, the initial puzzle grid, all discovered solutions
    (plus total solution count), or an appropriate error / "No solution found" message.
Collaborators: Gemini
Author: Aaron Hails
Creation: 10/15/2026
Revision: 10/15/2026
Revision History: Initial creation.
    Modifications to improve correctness, execution speed, space efficiency, and maintainability.
*/

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstdint>

/*
 * Enumeration representing the outcome of loading a puzzle file.
 * Distinguishes between missing files and improperly formatted puzzle data.
 * Authorship: Gemini
 */
enum class LoadStatus {
    SUCCESS,
    FILE_OPEN_ERROR,
    FORMAT_ERROR
};

/*
 * SudokuBoard class definition.
 * Stores the 9x9 board using uint16_t values (0 for empty '_', 1-9 for digits)
 * and maintains lookup tables tracking which digits are present in each row,
 * column, and 3x3 block for O(1) candidate checking.
 * Authorship: Gemini
 */
class SudokuBoard {
private:
    static const int SIZE = 9;
    static const int BLOCK_SIZE = 3;
    uint16_t grid[SIZE][SIZE]; // 0 represents '_', 1-9 represent digits

    // Lookup tables for O(1) constraint checking (1-indexed for digits 1-9)
    bool rowPresent[SIZE][SIZE + 1];
    bool colPresent[SIZE][SIZE + 1];
    bool blockPresent[SIZE][SIZE + 1];

    // Maps (row, col) coordinates to one of the nine 3x3 sub-grids (0-8)
    int getBlockIndex(int row, int col) const {
        return (row / BLOCK_SIZE) * BLOCK_SIZE + (col / BLOCK_SIZE);
    }

public:
    static const int MAX_CANDIDATES = 9;

    SudokuBoard() {
        clear();
    }

    /*
     * Resets all board cells to 0 (empty) and clears the presence lookup tables.
     * Authorship: Gemini
     */
    void clear() {
        for (int r = 0; r < SIZE; ++r) {
            for (int c = 0; c < SIZE; ++c) {
                grid[r][c] = 0;
            }
            for (int d = 0; d <= SIZE; ++d) {
                rowPresent[r][d] = false;
                colPresent[r][d] = false;
                blockPresent[r][d] = false;
            }
        }
    }

    /*
     * Reads a 9x9 Sudoku puzzle from a text file, differentiating between
     * file-open failures and bad puzzle formatting (invalid chars or token count).
     * Authorship: Gemini
     */
    LoadStatus loadFromFile(const std::string& filename) {
        clear();
        std::ifstream file(filename);
        if (!file.is_open()) {
            return LoadStatus::FILE_OPEN_ERROR;
        }

        for (int r = 0; r < SIZE; ++r) {
            for (int c = 0; c < SIZE; ++c) {
                char token = '\0';
                if (!(file >> token)) {
                    return LoadStatus::FORMAT_ERROR; // Fewer than 81 tokens
                }

                if (token == '_') {
                    grid[r][c] = 0;
                } else if (token >= '1' && token <= '9') {
                    uint16_t digit = static_cast<uint16_t>(token - '0');
                    grid[r][c] = digit;
                    rowPresent[r][digit] = true;
                    colPresent[c][digit] = true;
                    blockPresent[getBlockIndex(r, c)][digit] = true;
                } else {
                    return LoadStatus::FORMAT_ERROR; // Invalid character
                }
            }
        }

        // Verify no extra non-whitespace tokens remain after reading 81 cells
        char extraToken = '\0';
        if (file >> extraToken) {
            return LoadStatus::FORMAT_ERROR;
        }

        return LoadStatus::SUCCESS;
    }

    /*
     * Verifies that the initial pre-filled puzzle does not contain duplicate clues
     * in any row, column, or 3x3 block. Uses local tracking arrays so the method
     * remains strictly const.
     * Authorship: Gemini
     */
    bool isInitialBoardValid() const {
        bool seenInRow[SIZE][SIZE + 1] = {false};
        bool seenInCol[SIZE][SIZE + 1] = {false};
        bool seenInBlock[SIZE][SIZE + 1] = {false};

        for (int r = 0; r < SIZE; ++r) {
            for (int c = 0; c < SIZE; ++c) {
                uint16_t val = grid[r][c];
                if (val != 0) {
                    int blockIdx = getBlockIndex(r, c);
                    if (seenInRow[r][val] || seenInCol[c][val] || seenInBlock[blockIdx][val]) {
                        return false;
                    }
                    seenInRow[r][val] = true;
                    seenInCol[c][val] = true;
                    seenInBlock[blockIdx][val] = true;
                }
            }
        }
        return true;
    }

    /*
     * Finds the next empty cell (value 0) starting from (startRow, startCol)
     * to avoid rescanning from (0, 0) on every recursive call.
     * Authorship: Gemini
     */
    bool findEmptyCell(int startRow, int startCol, int& outRow, int& outCol) const {
        for (int r = startRow; r < SIZE; ++r) {
            int firstCol = (r == startRow) ? startCol : 0;
            for (int c = firstCol; c < SIZE; ++c) {
                if (grid[r][c] == 0) {
                    outRow = r;
                    outCol = c;
                    return true;
                }
            }
        }
        return false;
    }

    /*
     * Populates a fixed-size stack array with valid candidate digits for (row, col)
     * using O(1) lookups into rowPresent, colPresent, and blockPresent.
     * Authorship: Gemini
     */
    int getCandidates(int row, int col, uint16_t candidates[MAX_CANDIDATES]) const {
        int blockIdx = getBlockIndex(row, col);
        int count = 0;
        for (uint16_t digit = 1; digit <= SIZE; ++digit) {
            if (!rowPresent[row][digit] && !colPresent[col][digit] && !blockPresent[blockIdx][digit]) {
                candidates[count] = digit;
                ++count;
            }
        }
        return count;
    }

    /*
     * Places a digit (1-9) or clears a cell (0) at (row, col) and keeps the
     * row, column, and block presence lookup tables synchronized.
     * Authorship: Gemini
     */
    void setCell(int row, int col, uint16_t val) {
        uint16_t oldVal = grid[row][col];
        int blockIdx = getBlockIndex(row, col);

        // Remove the previous digit from the lookup tables if the cell wasn't empty
        if (oldVal != 0) {
            rowPresent[row][oldVal] = false;
            colPresent[col][oldVal] = false;
            blockPresent[blockIdx][oldVal] = false;
        }

        grid[row][col] = val;

        // Register the new digit in the lookup tables if non-empty
        if (val != 0) {
            rowPresent[row][val] = true;
            colPresent[col][val] = true;
            blockPresent[blockIdx][val] = true;
        }
    }

    /*
     * Prints the 9x9 Sudoku grid to the console, outputting '_' for 0.
     * Authorship: Gemini
     */
    void print() const {
        for (int r = 0; r < SIZE; ++r) {
            for (int c = 0; c < SIZE; ++c) {
                if (grid[r][c] == 0) {
                    std::cout << '_';
                } else {
                    std::cout << grid[r][c];
                }
                std::cout << (c < SIZE - 1 ? " " : "");
            }
            std::cout << "\n";
        }
    }
};

/*
 * SudokuSolver class encapsulating the recursive depth-first search (DFS)
 * with backtracking algorithm to find all solutions to a SudokuBoard.
 * Authorship: Gemini
 */
class SudokuSolver {
private:
    SudokuBoard board;
    int solutionsCount;

    /*
     * Recursive DFS method with backtracking. Finds the next empty cell starting
     * from (startRow, startCol), gathers candidates into a stack-allocated array,
     * and exhaustively tests each candidate.
     * Authorship: Gemini
     */
    bool solveRecursive(int startRow, int startCol) {
        int row = 0;
        int col = 0;

        // Base case: no empty cells remain, meaning a complete solution was found
        if (!board.findEmptyCell(startRow, startCol, row, col)) {
            ++solutionsCount;
            std::cout << "\nSolution " << solutionsCount << ":\n";
            board.print();
            return true;
        }

        uint16_t candidates[SudokuBoard::MAX_CANDIDATES];
        int candidateCount = board.getCandidates(row, col, candidates);

        if (candidateCount == 0) {
            return false; // Dead end; trigger backtracking
        }

        bool foundSolutionInBranch = false;
        int nextRow = (col == 8) ? (row + 1) : row;
        int nextCol = (col == 8) ? 0 : (col + 1);

        for (int i = 0; i < candidateCount; ++i) {
            board.setCell(row, col, candidates[i]);

            if (solveRecursive(nextRow, nextCol)) {
                foundSolutionInBranch = true;
            }

            board.setCell(row, col, 0); // Backtrack
        }

        return foundSolutionInBranch;
    }

public:
    SudokuSolver() : solutionsCount(0) {}

    /*
     * Loads a single puzzle file, reports file or format errors, prints the
     * initial puzzle, validates the initial clues, and runs the recursive solver.
     * Authorship: Gemini
     */
    void processPuzzleFile(const std::string& filename) {
        solutionsCount = 0;
        std::cout << "====================================\n";
        std::cout << filename << ":\n";

        LoadStatus status = board.loadFromFile(filename);
        if (status == LoadStatus::FILE_OPEN_ERROR) {
            std::cout << "Error: Unable to open file " << filename << "\n\n";
            return;
        } else if (status == LoadStatus::FORMAT_ERROR) {
            std::cout << "Error: Bad file formatting in " << filename << "\n\n";
            return;
        }

        board.print();

        if (!board.isInitialBoardValid() || !solveRecursive(0, 0)) {
            std::cout << "\nNo solution found\n";
        } else {
            std::cout << "\nSolutions found: " << solutionsCount << "\n";
        }
        std::cout << "\n";
    }
};

/*
 * SudokuApplication class that manages the list of five puzzle files
 * and runs the SudokuSolver on each file sequentially.
 * Authorship: Gemini
 */
class SudokuApplication {
private:
    std::vector<std::string> puzzleFiles;
    SudokuSolver solver;

public:
    SudokuApplication() {
        puzzleFiles = {
            "puzzle1.txt",
            "puzzle2.txt",
            "puzzle3.txt",
            "puzzle4.txt",
            "puzzle5.txt"
        };
    }

    void run() {
        for (const std::string& filename : puzzleFiles) {
            solver.processPuzzleFile(filename);
        }
    }
};

/*
 * Program entry point.
 * Authorship: Gemini
 */
int main() {
    SudokuApplication app;
    app.run();
    return 0;
}