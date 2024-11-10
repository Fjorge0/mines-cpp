#include "../include/mines.hpp"
#include <chrono>
#include <format>
#include <QtCore/QTimer>
#include <QtGui/QIcon>
#include <QtGui/QMouseEvent>
#include <QShortcut>
#include <QStyle>
#include <QtWidgets/QApplication>
#include <QtWidgets/QFrame>
#include <QtWidgets/QGraphicsBlurEffect>
#include <QtWidgets/QGraphicsColorizeEffect>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QWidget>
#include <QtWidgets/QStackedLayout>

class MinesweeperWindow : public QMainWindow {
public:
    class Tile : public QPushButton {
    public:
        Tile(const QIcon& icon, const QString& str, MinesweeperWindow* parentWindow, size_t row, size_t col)
            : QPushButton(icon, str), row(row), col(col), parentWindow(parentWindow) {
            this->setAutoFillBackground(true);
        }

        Tile(const QString& str, MinesweeperWindow* parentWindow, size_t row, size_t col)
            : Tile(QIcon(), str, parentWindow, row, col) {}

        virtual ~Tile() {}

        virtual bool hasHeightForWidth() const override {
            return true;
        }
        virtual int heightForWidth(int w) const override {
            return w;
        }

        virtual void mousePressEvent(QMouseEvent* event) override {
            if (parentWindow->gameState == GameState::NONE) {
                if (parentWindow->game.getGrid().at(row).at(col).isRevealed()) {
                    for (int rowOffset = -1; rowOffset <= 1; ++rowOffset) {
                        for (int colOffset = -1; colOffset <= 1; ++colOffset) {
                            if (rowOffset == 0 && colOffset == 0) {
                                continue;
                            }

                            try {
                                Tile& t = *parentWindow->grid.at(row + rowOffset).at(col + colOffset);
                                if (!t.isFlat()) {
                                    t.setDown(true);
                                }
                            } catch (const std::out_of_range&) {
                                continue;
                            }
                        }
                    }
                }

                if (event->button() == Qt::LeftButton || event->button() == Qt::MiddleButton) {
                    parentWindow->game.reveal(row, col);
                } else if (event->button() == Qt::RightButton) {
                    parentWindow->game.flag(row, col);
                }

                parentWindow->updateGrid();
                parentWindow->repaint();
            }

            QPushButton::mousePressEvent(event);
        }

        virtual void mouseReleaseEvent(QMouseEvent* event) override {
            for (int rowOffset = -1; rowOffset <= 1; ++rowOffset) {
                for (int colOffset = -1; colOffset <= 1; ++colOffset) {
                    if (rowOffset == 0 && colOffset == 0) {
                        continue;
                    }

                    try {
                        parentWindow->grid.at(row + rowOffset).at(col + colOffset)->setDown(false);
                    } catch (const std::out_of_range&) {
                        continue;
                    }
                }
            }

            QPushButton::mouseReleaseEvent(event);

            this->setChecked(true);
        }


    private:
        const size_t row = 0, col = 0;
        MinesweeperWindow* parentWindow;
    };

    class RestartButton : public QPushButton {
    public:
        RestartButton(const QIcon& icon, const QString& str, MinesweeperWindow* parentWindow)
            : QPushButton(icon, str), parentWindow(parentWindow) {}

        RestartButton(const QString& str, MinesweeperWindow* parentWindow)
            : RestartButton(QIcon(), str, parentWindow) {}

        virtual ~RestartButton() {}

        virtual void mousePressEvent(QMouseEvent* event) override {
            parentWindow->restartGame();
            QPushButton::mousePressEvent(event);
        }

    private:
        MinesweeperWindow* parentWindow;
    };

    class SettingsButton : public QPushButton {
    public:
        SettingsButton(const QIcon& icon, const QString& str, MinesweeperWindow* parentWindow)
            : QPushButton(icon, str), parentWindow(parentWindow) {}

        SettingsButton(const QString& str, MinesweeperWindow* parentWindow)
            : SettingsButton(QIcon(), str, parentWindow) {}

        virtual ~SettingsButton() {}

        virtual void mousePressEvent(QMouseEvent* event) override {
            parentWindow->playPauseGame();
            QPushButton::mousePressEvent(event);
        }

    private:
        MinesweeperWindow* parentWindow;
    };

    class GameTimer : public QTimer {
    public:
        GameTimer(MinesweeperWindow* parentWindow)
            : QTimer(), parentWindow(parentWindow) {
            this->setTimerType(Qt::PreciseTimer);
            this->setInterval(50);
        }

        virtual ~GameTimer() {}

        void playPause() {
            paused = !paused;
        }

        bool isPaused() const {
            return paused;
        }

        void start() {
            startTime = std::chrono::high_resolution_clock::now();
            lastDuration = std::chrono::milliseconds(0);

            QTimer::start();
        }

        void stop() {
            paused = false;
            setDuration();
            QTimer::stop();
        }

        virtual void timerEvent(QTimerEvent*) override {
            setDuration();
        }

    private:
        void setDuration() {
            std::chrono::duration duration = std::chrono::high_resolution_clock::now() - startTime;
            if (paused) {
                startTime += duration - lastDuration;
            } else {
                lastDuration = duration;
            }

            const std::chrono::seconds castDuration = std::chrono::duration_cast<std::chrono::seconds>(duration);
            parentWindow->timeLabel.setText(QString::fromStdString(std::format("{0:%H}:{0:%M}:{0:%S}", castDuration)));
        }

    private:
        MinesweeperWindow* parentWindow;
        std::chrono::time_point<std::chrono::high_resolution_clock> startTime;
        std::chrono::duration<unsigned long int, std::nano> lastDuration;
        bool paused = false;
    };

    enum struct GameState : uint8_t {
        NONE = 0,
        WON = 1,
        LOST = 2
    };

public:
    //9x9: 10
    //16x16: 40
    //30x16: 99
    MinesweeperWindow() : QMainWindow(), game(30, 16, 99) {
        gridLayout.setSpacing(1);
        flagLabel.setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
        restartButton.setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
        timeLabel.setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
        gridFrame.setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
        pausedIcon.setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

        // Create a central widget
        this->setCentralWidget(&centralWidget);

        // Create the top bar
        topBoxLayout.addWidget(&flagLabel, 1, Qt::AlignLeft);
        topBoxLayout.addWidget(&restartButton, 0, Qt::AlignCenter);
        topBoxLayout.addWidget(&settingsButton, 0, Qt::AlignCenter);
        topBoxLayout.addWidget(&timeLabel, 1, Qt::AlignRight);

        mainLayout.addItem(&topBoxLayout);

        // Create indicator that game is paused
        pausedIcon.setIcon(QIcon::fromTheme(("media-playback-pause")));
        pausedIcon.setProperty("class", "pauseIcon");

        // Create a stacked layout for the board
        boardFrame.setLayout(&stackLayout);

        stackLayout.addWidget(&pausedIcon);

        // Create the grid for the board
        gridLayout.setContentsMargins(0, 0, 0, 0);
        gridFrame.setLayout(&gridLayout);

        stackLayout.addWidget(&gridFrame);
        stackLayout.setCurrentWidget(&gridFrame);

        resizeGrid();

        // Add board to window
        mainLayout.addWidget(&boardFrame);

        // Shortcut to restart game
        QObject::connect(&shortcut, &QShortcut::activated, this, static_cast<void (MinesweeperWindow::*)(void)>(&MinesweeperWindow::restartGame));

        // Show the window
        centralWidget.setLayout(&mainLayout);
    }

    virtual ~MinesweeperWindow() {}

protected:
    void restartGame(unsigned int width, unsigned int height, unsigned long int mineCount) {
        game.initialise(width, height, mineCount);
        this->resizeGrid();
        this->repaint();
    }

    void restartGame(void) {
        this->restartGame(game.width(), game.height(), game.mineCount());
    }

    void playPauseGame() {
        timer.playPause();
        this->updateGrid();
    }

    void resizeGrid() {
        static QSizePolicy tilePolicy(QSizePolicy::Preferred, QSizePolicy::Ignored);
        tilePolicy.setWidthForHeight(true);

        // Delete tiles from grid
        QLayoutItem* item = nullptr;
        while ((item = gridLayout.takeAt(0)) != nullptr) {
            gridLayout.removeWidget(item->widget());
            gridLayout.removeItem(item);
            delete item->widget();
            delete item;
        }

        // Add new tiles to grid
        grid.clear();
        grid.resize(game.height());
        for (size_t row = 0; row < game.height(); ++row) {
            grid.at(row).reserve(game.width());

            for (size_t col = 0; col < game.width(); ++col) {
                Tile* tile = new Tile("", this, row, col);
                tile->setSizePolicy(tilePolicy);
                tile->setProperty("class", "tile");
                tile->ensurePolished();

                gridLayout.addWidget(tile, row, col);
                tile->show();

                grid[row].push_back(tile);
            }
        }

        this->timer.stop();
        this->timeLabel.setText("00:00:00");

        this->updateGrid();
    }

    void updateGrid() {
        // Set restart button face
        if (this->game.isAllExceptMinesRevealed()) {
            restartButton.setIcon(QIcon::fromTheme("face-cool"));
            gameState = GameState::WON;
        } else if (this->game.isMineRevealed()) {
            restartButton.setIcon(QIcon::fromTheme("face-sad"));
            gameState = GameState::LOST;
        } else {
            restartButton.setIcon(QIcon::fromTheme("face-smile"));
            gameState = GameState::NONE;
        }

        const auto& board = game.getGrid();

        // Apply blur if game is paused
        if (timer.isPaused()) {
            QGraphicsBlurEffect* blur = new QGraphicsBlurEffect();
            blur->setBlurHints(QGraphicsBlurEffect::PerformanceHint);
            blur->setBlurRadius(20);

            gridFrame.setGraphicsEffect(blur);

            stackLayout.setCurrentWidget(&pausedIcon);
            stackLayout.setStackingMode(QStackedLayout::StackAll);
        } else {
            gridFrame.setGraphicsEffect(nullptr);

            stackLayout.setCurrentWidget(&gridFrame);
            stackLayout.setStackingMode(QStackedLayout::StackOne);
        }

        bool revealed = false;
        for (size_t row = 0; row < game.height(); ++row) {
            for (size_t col = 0; col < game.width(); ++col) {
                const auto& t = board.at(row).at(col);
                Tile& tile = *grid.at(row).at(col);

                // Default empty
                tile.setIcon(QIcon());
                tile.setText("");

                if (timer.isPaused()) {
                    tile.setDisabled(true);
                } else {
                    tile.setDisabled(false);
                }

                tile.setFlat(false);
                tile.setProperty("type", "");

                if (t.isMine()) {
                    tile.setProperty("type", "mine");
                } else {
                    tile.setProperty("type", t.adjacentMineCount());
                }

                if (t.isFlagged()) {
                    // Flagged
                    tile.setProperty("flagged", "true");

                    if (gameState != GameState::NONE) {
                        if (t.isMine()) {
                            tile.setIcon(QIcon::fromTheme("flag-green"));
                        } else {
                            tile.setIcon(QIcon::fromTheme("flag-red"));
                        }
                    } else {
                        tile.setIcon(QIcon::fromTheme("flag"));
                    }
                    tile.setFlat(false);
                } else {
                    tile.setProperty("flagged", "false");
                }

                if (t.isRevealed()) {
                    revealed = true;

                    // Revealed class
                    tile.setProperty("revealed", "true");

                    tile.setFlat(true);

                    if (t.isMine()) {
                        tile.setIcon(QIcon::fromTheme("edit-bomb"));
                        tile.setFlat(false);
                        tile.setCheckable(true);
                    } else if (t.adjacentMineCount() != 0) {
                        tile.setText(QString::fromStdString(std::string(1, (char) board.at(row).at(col))));
                    }
                } else if (gameState != GameState::NONE) {
                    // When game is over
                    tile.setProperty("gameOver", "true");

                    // Mine
                    if (t.isMine() && !t.isFlagged()) {
                        tile.setIcon(QIcon::fromTheme("edit-bomb"));
                        tile.setFlat(false);
                        tile.setDisabled(true);
                    }
                }

                // Update styles
                tile.style()->unpolish(&tile);
                tile.style()->polish(&tile);
            }
        }

        if (revealed && !timer.isActive()) {
            timer.start();
        } else if (gameState != GameState::NONE) {
            timer.stop();
        }
        this->flagLabel.setText(QString::fromStdString(std::to_string(game.flagCount()) + '/' + std::to_string(game.mineCount())));
    }

protected:
    minesweeper::game game;
    QVBoxLayout& mainLayout = *new QVBoxLayout();

    // Minesweeper grid
    QWidget& centralWidget = *new QWidget();

    QFrame& boardFrame = *new QFrame();
    QStackedLayout& stackLayout = *new QStackedLayout();
    QPushButton& pausedIcon = *new QPushButton();

    QFrame& gridFrame = *new QFrame();
    QGridLayout& gridLayout = *new QGridLayout();

    QLabel& flagLabel = *new QLabel();

    // Game controls
    QHBoxLayout& topBoxLayout = *new QHBoxLayout();

    RestartButton& restartButton = *new RestartButton("", this);
    QShortcut& shortcut = *new QShortcut(QKeySequence(Qt::Key_Space), this);

    SettingsButton& settingsButton = *new SettingsButton(QIcon::fromTheme("configure"), "", this);

    // Game misc
    GameTimer& timer = *new GameTimer(this);
    QLabel& timeLabel = *new QLabel();

    GameState gameState = GameState::NONE;
    std::vector<std::vector<Tile*>> grid;
};

int main(int argc, char* argv[]) {
    QApplication::setDesktopSettingsAware(true);
    QApplication app(argc, argv);

    // Create and title the window
    MinesweeperWindow window;
    window.setWindowTitle("Minesweeper");

    // Borrow the icon from gnome-mines if possible
    window.setWindowIcon(QIcon::fromTheme("gnome-mines", QIcon::fromTheme("edit-bomb")));

    // Load an application style
    QFile styleFile(":/style.qss");
    styleFile.open(QFile::ReadOnly);

    // Apply the loaded stylesheet
    QString style(styleFile.readAll());
    app.setStyleSheet(style);

    window.show();

    // Execute the application
    return app.exec();
}

// vim: ts=2:sw=2:expandtab
