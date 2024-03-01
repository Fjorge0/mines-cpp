#include "minesweeper.hpp"
#include <chrono>
#include <format>
#include <unordered_map>
#include <QtCore/QTimer>
#include <QtGui/QIcon>
#include <QtGui/QMouseEvent>
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
            if (parentWindow->gameState == NONE) {
                if (parentWindow->game.getGrid().at(row).at(col).isRevealed()) {
                    for (int rowOffset = -1; rowOffset <= 1; ++rowOffset) {
                        for (int colOffset = -1; colOffset <= 1; ++colOffset) {
                            if (rowOffset == 0 && colOffset == 0) {
                                continue;
                            }

                            try {
                                Tile* t = parentWindow->grid.at(row + rowOffset).at(col + colOffset);
                                if (!t->isFlat()) {
                                    t->setDown(true);
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
            parentWindow->timeLabel->setText(QString::fromStdString(std::format("{0:%H}:{0:%M}:{0:%S}", castDuration)));
        }

    private:
        MinesweeperWindow* parentWindow;
        std::chrono::time_point<std::chrono::high_resolution_clock> startTime;
        std::chrono::duration<unsigned long int, std::nano> lastDuration;
        bool paused = false;
    };

    enum GameState {
        NONE = 0,
        WON = 1,
        LOST = 2
    };

public:
    //9x9: 10
    //16x16: 40
    //30x16: 99
    MinesweeperWindow() : QMainWindow(), game(30, 16, 99) {
        gridLayout->setSpacing(1);
        flagLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        restartButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        timeLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

        // Create a central widget
        QWidget *centralWidget = new QWidget();
        this->setCentralWidget(centralWidget);

        // Create the top bar
        QHBoxLayout *topBoxLayout = new QHBoxLayout();

        topBoxLayout->addWidget(flagLabel, 1, Qt::AlignLeft);
        topBoxLayout->addWidget(restartButton, 0, Qt::AlignCenter);
        topBoxLayout->addWidget(settingsButton, 0, Qt::AlignCenter);
        topBoxLayout->addWidget(timeLabel, 1, Qt::AlignRight);

        mainLayout->addItem(topBoxLayout);

        // Create the grid for the board
        resizeGrid();

        gridLayout->setContentsMargins(0, 0, 0, 0);
        gridFrame->setLayout(gridLayout);

        mainLayout->addWidget(gridFrame);

        // Show the window
        centralWidget->setLayout(mainLayout);
    }

    virtual ~MinesweeperWindow() {}

    // TODO: Make space global
    virtual void keyPressEvent(QKeyEvent* event) override {
        if (event->key() == Qt::Key_Space) {
            this->restartGame();
        }

        QMainWindow::keyPressEvent(event);
    }

protected:
    void restartGame(unsigned int width, unsigned int height, unsigned long int mineCount) {
        game.initialise(width, height, mineCount);
        this->resizeGrid();
        this->repaint();
    }

    void restartGame() {
        this->restartGame(game.width(), game.height(), game.mineCount());
    }

    void playPauseGame() {
        timer.playPause();

        if (!timer.isPaused()) {
            gridFrame->setGraphicsEffect(nullptr);
        } else {
            QGraphicsBlurEffect* blur = new QGraphicsBlurEffect();
            blur->setBlurHints(QGraphicsBlurEffect::PerformanceHint);
            blur->setBlurRadius(5);

            gridFrame->setGraphicsEffect(blur);
        }

        this->updateGrid();
    }

    void resizeGrid() {
        static QSizePolicy tilePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
        tilePolicy.setHeightForWidth(true);

        QLayoutItem* item = nullptr;
        while ((item = gridLayout->takeAt(0)) != nullptr) {
            gridLayout->removeWidget(item->widget());
            gridLayout->removeItem(item);
            delete item->widget();
            delete item;
        }
        grid.clear();

        for (size_t row = 0; row < game.height(); ++row) {
            for (size_t col = 0; col < game.width(); ++col) {
                Tile* tile = new Tile("", this, row, col);
                tile->setSizePolicy(tilePolicy);

                gridLayout->addWidget(tile, row, col);
                tile->show();

                grid[row][col] = tile;
            }
        }

        this->updateGrid();

        this->timer.stop();
        this->timeLabel->setText("00:00");
    }

    void updateGrid() {
        if (this->game.isAllExceptMinesRevealed()) {
            restartButton->setIcon(QIcon::fromTheme("face-cool"));
            gameState = WON;
        } else if (this->game.isMineRevealed()) {
            restartButton->setIcon(QIcon::fromTheme("face-sad"));
            gameState = LOST;
        } else {
            restartButton->setIcon(QIcon::fromTheme("face-smile"));
            gameState = NONE;
        }

        const auto& board = game.getGrid();
        for (size_t row = 0; row < game.height(); ++row) {
            for (size_t col = 0; col < game.width(); ++col) {
                const auto& t = board.at(row).at(col);
                Tile* tile = grid.at(row).at(col);

                // Default empty
                tile->setIcon(QIcon());
                tile->setText("");

                if (timer.isPaused()) {
                    tile->setDisabled(true);

                    QGraphicsBlurEffect* blur = new QGraphicsBlurEffect();
                    blur->setBlurHints(QGraphicsBlurEffect::PerformanceHint);
                    blur->setBlurRadius(20);

                    tile->setGraphicsEffect(blur);
                } else {
                    tile->setDisabled(false);
                    tile->setGraphicsEffect(nullptr);
                }

                tile->setFlat(false);
                tile->setProperty("class", "");

                if (t.isMine()) {
                    tile->setProperty("class", "mine");
                }

                if (t.isFlagged()) {
                    // Flagged
                    tile->setProperty("class", tile->property("class").toString() + " flagged");

                    if (gameState != NONE) {
                        if (t.isMine()) {
                            tile->setIcon(QIcon::fromTheme("flag-green"));
                        } else {
                            tile->setIcon(QIcon::fromTheme("flag-red"));
                        }
                    } else {
                        tile->setIcon(QIcon::fromTheme("flag"));
                    }
                    tile->setFlat(false);
                }

                if (t.isRevealed()) {
                    // Revealed class
                    tile->setProperty("class", tile->property("class").toString() + " revealed");

                    tile->setFlat(true);

                    if (t.isMine()) {
                        tile->setIcon(QIcon::fromTheme("edit-bomb"));
                        tile->setFlat(false);
                        tile->setCheckable(true);
                    } else if (t.adjacentMineCount() != 0) {
                        tile->setText(QString::fromStdString(std::string(1, (char) board.at(row).at(col))));
                    }
                } else if (gameState != NONE) {
                    // When game is over
                    tile->setProperty("class", tile->property("class").toString() + " over");

                    // Mine
                    if (t.isMine() && !t.isFlagged()) {
                        tile->setIcon(QIcon::fromTheme("edit-bomb"));
                        tile->setFlat(false);
                        tile->setDisabled(true);
                    }
                }
            }
        }

        if (!timer.isActive()) {
            timer.start();
        } else if (gameState != NONE) {
            timer.stop();
        }
        this->flagLabel->setText(QString::fromStdString(std::to_string(game.flagCount()) + '/' + std::to_string(game.mineCount())));
    }

protected:
    minesweeper::game game;
    std::unordered_map<size_t, std::unordered_map<size_t, Tile*>> grid;

    QVBoxLayout *mainLayout = new QVBoxLayout();

    QFrame* gridFrame = new QFrame();
    QGridLayout* gridLayout = new QGridLayout();
    QLabel* flagLabel = new QLabel();
    RestartButton* restartButton = new RestartButton("", this);
    SettingsButton* settingsButton = new SettingsButton(QIcon::fromTheme("configure"), "", this);

    GameTimer timer = GameTimer(this);
    QLabel* timeLabel = new QLabel();

    GameState gameState = NONE;
};

int main(int argc, char* argv[]) {
    QApplication::setDesktopSettingsAware(true);
    QApplication app(argc, argv);

    // Create and title the window
    MinesweeperWindow window;
    window.setWindowTitle("Minesweeper");

    // Borrow the icon from gnome-mines if possible
    window.setWindowIcon(QIcon::fromTheme("gnome-mines", QIcon::fromTheme("edit-bomb")));

    window.show();

    // Execute the application
    return app.exec();
}

// vim: ts=2:sw=2:expandtab
