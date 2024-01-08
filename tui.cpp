#include "minesweeper.hpp"
#include <algorithm>
#include <array>
#include <cctype>
#include <iostream>

static constexpr std::array<std::string, 9> NUMCOLOURS = {
  "",
  "\033[94m",
  "\033[32m",
  "\033[91m",
  "\033[34m",
  "\033[31m",
  "\033[36m",
  "\033[35m",
  "\033[37m"
};

template<typename T>
static T wrap(T value, const T& min, const T& max) {
  if (max < min) {
    throw std::invalid_argument("max must be greater than min");
  }

  while (value < min) {
    value += (max - min);
  }
  return value % (max - min);
}

int main(int argc, char** argv) {
  if (argc < 4) {
    std::cout << "USAGE: command [width] [height] [mine count]" << std::endl;
    return 1;
  }
  while (true) {
    minesweeper::game game = minesweeper::game(atoi(argv[1]), atoi(argv[2]), atoi(argv[3]));

    const auto& grid = game.getGrid();

    int selectedRow = 0, selectedCol = 0;
    while (!game.isAllExceptMinesRevealed()) {
      // Print the game state
      system("clear");

      std::cout << game.flagCount() << '/' << game.mineCount() << '\n';

      for (size_t row = 0; row < game.height(); ++row) {
        for (size_t col = 0; col < game.width(); ++col) {
          const auto& t = grid.at(row).at(col);

          if (row == selectedRow && col == selectedCol) {
            std::cout << "\033[100m";
          }

          if (!t.isRevealed() && !t.isFlagged()) {
            std::cout << "■";
          } else {
            if (t.isFlagged()) {
            } else if (t.isMine()) {
              std::cout << "\033[41m";
            } else {
              std::cout << NUMCOLOURS[t.adjacentMineCount()];
            }
            std::cout << (char) t;
          }

          std::cout << "\033[0m";
        }
        std::cout << '\n';
      }

      if (game.isMineRevealed()) {
        break;
      }

      std::cout.flush();
      std::cout << "Input: ";
      std::cout.flush();

      // Get user input
      std::string input;
      std::cin >> input;

      // Set input to lowercase
      // https://stackoverflow.com/a/313990
      std::transform(input.begin(), input.end(), input.begin(), [](unsigned char c){ return std::tolower(c); });

      // Process input
      for (const char& c : input) {
        switch (c) {
          case '>':
            selectedCol = wrap(selectedCol + 1, 0, (int)game.width());
            break;
          case '<':
            selectedCol = wrap(selectedCol - 1, 0, (int)game.width());
            break;
          case 'v':
            selectedRow = wrap(selectedRow + 1, 0, (int)game.height());
            break;
          case '^':
            selectedRow = wrap(selectedRow - 1, 0, (int)game.height());
            break;
          case 'r':
            game.reveal(selectedRow, selectedCol);
            break;
          case 'f':
            game.flag(selectedRow, selectedCol);
            break;
          case 'q':
            return 0;
          case 's':
            goto newGame;
        }
      }
    }

    // Print final game state
    system("clear");

    if (game.isMineRevealed()) {
      std::cout << "You lose!" << std::endl;
    } else if (game.isAllExceptMinesRevealed()) {
      std::cout << "You win!" << std::endl;
    }

    for (size_t row = 0; row < game.height(); ++row) {
      for (size_t col = 0; col < game.width(); ++col) {
        const auto& t = grid.at(row).at(col);

        if (t.isFlagged()) {
          if (t.isMine()) {
            std::cout << "\033[42m";
          } else {
            std::cout << "\033[41m";
          }
        } else if (t.isMine()) {
          if (t.isRevealed()) {
            std::cout << "\033[41m";
          }
        } else {
          std::cout << NUMCOLOURS[t.adjacentMineCount()];
        }

        if (t.isRevealed() || t.isMine() || t.isFlagged()) {
          std::cout << (char) t;
        } else {
          std::cout << "\033[0m■";
        }

        std::cout << "\033[0m";
      }

      std::cout << '\n';
    }
    std::cout.flush();

    return 0;

newGame:
    continue;
  }
}

// vim: ts=2:sw=2:expandtab
