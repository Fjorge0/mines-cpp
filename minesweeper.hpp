#ifndef MINESWEEPER
#define MINESWEEPER

#include <chrono>
#include <deque>
#include <vector>
#include <unordered_set>
#include <random>

static inline std::pair<size_t, size_t> intToCoords(size_t width, size_t height, unsigned long int num) {
  return std::pair<size_t, size_t>{num / width, num % height};
}

static inline unsigned long int coordsToInt(size_t width, size_t height, const std::pair<size_t, size_t>& coords) {
  return width * coords.first + coords.second;
}

namespace minesweeper {
  class game {
    public:
      class tile {
        public:
          bool isRevealed() const {
            return this->revealed;
          }

          bool isFlagged() const {
            return this->flagged;
          }

          bool isMine() const {
            return this->mined;
          }

          unsigned short int adjacentMineCount() const {
            return this->adjacentMines;
          }

          unsigned short int adjacentFlagCount() const {
            return this->adjacentFlags;
          }

          // Return a character representing the tile
          explicit operator char() const {
            if (this->flagged) {
              return 'F';
            }

            if (this->mined) {
              return '*';
            }

            if (this->adjacentMines == 0) {
              return ' ';
            }

            return '0' + this->adjacentMines;
          }

        private:
          tile() {}

          bool flag() {
            if (!this->revealed) {
              this->flagged = !this->flagged;
              return true;
            } else {
              return false;
            }
          }

          bool reveal() {
            if (!this->flagged && !this->revealed) {
              this->revealed = true;
              return true;
            } else {
              return false;
            }
          }

          friend minesweeper::game;

        private:
          bool revealed = false;
          bool flagged = false;
          bool mined = false;
          unsigned short int adjacentMines = 0;
          unsigned short int adjacentFlags = 0;
      };

      tile& tileAt(size_t row, size_t col) {
        return this->grid.at(row).at(col);
      }

      const tile& tileAt(size_t row, size_t col) const {
        return this->grid.at(row).at(col);
      }

      tile& tileAt(const std::pair<size_t, size_t>& coords) {
        return this->tileAt(coords.first, coords.second);
      }

      const tile& tileAt(const std::pair<size_t, size_t>& coords) const {
        return this->tileAt(coords.first, coords.second);
      }

    public:
      game(unsigned int width, unsigned int height, unsigned long int mineCount) {
        // Initialise the random number generator
        this->rng.seed(std::chrono::high_resolution_clock::now().time_since_epoch().count());
        this->rng.discard(5);

        this->initialise(width, height, mineCount);
      }

      void initialise(unsigned int width, unsigned int height, unsigned long int mineCount) {
        if (width == 0 || height == 0) {
          throw std::invalid_argument("Invalid width or height of game board.");
        }
        if (mineCount > width * height) {
          throw std::out_of_range("Requested mine count exceeds size of board.");
        }

        // Reset game state
        this->mines.clear();
        this->firstReveal = true;
        {
          std::vector<tile> row(width, tile());
          this->grid = std::vector<std::vector<tile>>(height, row);
        }

        // Create distribution to generate mines
        std::uniform_int_distribution<unsigned long int> distribution(0, (unsigned long int)(width * height) - 1);

        // Generate mines
        while (this->mines.size() < mineCount) {
          unsigned long int minePos = distribution(this->rng);

          if (this->mines.insert(minePos).second) {
            auto [row, col] = intToCoords(width, height, minePos);

            // Set the position as mined
            try {
              this->tileAt(row, col).mined = true;
            } catch (const std::out_of_range&) {
              this->mines.erase(minePos);
              continue;
            }

            // Increase the count of adjacent mines in adjacent tiles
            for (int rowOffset = -1; rowOffset <= 1; ++rowOffset) {
              for (int colOffset = -1; colOffset <= 1; ++colOffset) {
                if (rowOffset == 0 && colOffset == 0) {
                  continue;
                }

                try {
                  ++(this->tileAt(row + rowOffset, col + colOffset).adjacentMines);
                } catch (const std::out_of_range&) {
                  continue;
                }
              }
            }
          }
        }
      }

      void reveal(unsigned long int initialPosition) {
        std::unordered_set<unsigned long int> passedTiles;

        // Continue to reveal tiles until there are no more to reveal
        std::deque<unsigned long int> queuedTiles;
        queuedTiles.push_front(initialPosition);
        while (queuedTiles.size() > 0) {
          unsigned long int position = queuedTiles.front();
          queuedTiles.pop_front();

          if (passedTiles.insert(position).second) {
            try {
              auto [row, col] = intToCoords(this->width(), this->height(), position);
              tile& t = this->tileAt(row, col);

              bool wasHidden = t.reveal();

              if (wasHidden) {
                // Regenerate the game if the first reveal is on a mine
                if (t.mined && this->firstReveal) {
                  this->initialise(this->width(), this->height(), this->mines.size());
                  return this->reveal(initialPosition);
                }

                this->firstReveal = false;
              }

              // Propogate the revealing to the surrounding tiles
              if (
                  // Do not propogate if the tile is a mine or a flag
                  (!t.mined && !t.flagged) &&
                  (
                   // Propogate if the tile is blank
                   (t.adjacentMines == 0) ||
                   // Propogate if the number of flags match the number of mines and it is the initial tile and revealed
                   (position == initialPosition && t.adjacentFlags == t.adjacentMines && !wasHidden)
                  )
                 ){
                for (int rowOffset = -1; rowOffset <= 1; ++rowOffset) {
                  for (int colOffset = -1; colOffset <= 1; ++colOffset) {
                    if (rowOffset == 0 && colOffset == 0) {
                      continue;
                    }

                    try {
                      // Check if tile is valid
                      this->tileAt(row + rowOffset, col + colOffset);

                      // Queue adjacent tile to be revealed
                      queuedTiles.push_back(coordsToInt(this->width(), this->height(), {row + rowOffset, col + colOffset}));
                    } catch (const std::out_of_range&) {
                      continue;
                    }
                  }
                }
              }
            } catch (const std::out_of_range& e) {
              if (position == initialPosition) {
                throw e;
              }

              continue;
            }
          }
        }
      }

      void flag(unsigned long int position) {
        auto [row, col] = intToCoords(this->width(), this->height(), position);

        tile& t = this->tileAt(row, col);
        if (t.flag()) {
          // Adjust the adjacent flag count on the grid
          for (int rowOffset = -1; rowOffset <= 1; ++rowOffset) {
            for (int colOffset = -1; colOffset <= 1; ++colOffset) {
              if (rowOffset == 0 && colOffset == 0) {
                continue;
              }

              try {
                this->tileAt(row + rowOffset, col + colOffset).adjacentFlags += (t.flagged ? 1 : -1);
              } catch (const std::out_of_range&) {
                continue;
              }
            }
          }

          // Update the flags set
          if (t.flagged) {
            this->flags.insert(position);
          } else {
            this->flags.erase(position);
          }
        }
      }

      auto reveal(unsigned int row, unsigned int col) {
        return this->reveal(coordsToInt(this->width(), this->height(), {row, col}));
      }

      auto flag(unsigned int row, unsigned int col) {
        return this->flag(coordsToInt(this->width(), this->height(), {row, col}));
      }

    private:
      std::default_random_engine rng;
      std::vector<std::vector<tile>> grid;
      std::unordered_set<unsigned long int> mines, flags;
      bool firstReveal = true;

    public:
      inline size_t width() const {
        return this->grid.at(0).size();
      }

      inline size_t height() const {
        return this->grid.size();
      }

      const std::vector<std::vector<tile>>& getGrid() const {
        return this->grid;
      }

      size_t mineCount() const {
        return this->mines.size();
      }

      size_t flagCount() const {
        return this->flags.size();
      }

      bool isAllExceptMinesRevealed() const {
        for (const auto& row : this->grid) {
          for (const tile& t : row) {
            if (!t.revealed && !t.mined) {
              return false;
            }
          }
        }

        return !this->isMineRevealed();
      }

      bool isMineRevealed() const {
        for (const auto& position : mines) {
          auto [row, col] = intToCoords(this->width(), this->height(), position);

          if (this->tileAt(row, col).revealed) {
            return true;
          }
        }

        return false;
      }
  };
};

#endif
// vim: ts=2:sw=2:expandtab
