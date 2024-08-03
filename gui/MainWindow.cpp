#include "MainWindow.hpp"
#include <QScreen>
#include <QThread>

namespace Tetris::gui {

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent) {
    initWindow();
    initWidgets();
    connectWidgets();
}

MainWindow::~MainWindow() = default;

void MainWindow::initWindow() {
    setFixedSize(m_windowWidth, m_windowHeight);
    setWindowTitle("Tetris");
    move(screen()->geometry().center() - frameGeometry().center());
}

void MainWindow::initWidgets() {
    m_pieceRandomizer = core::TetrominoFactory::UniformPieceRandomizer;
    m_timer = std::make_unique<QTimer>(this);

    QFont labelFont("Courier", 12, QFont::Bold);
    m_labelNext = createLabel("Next", labelFont);
    m_labelLines = createLabel("Lines\n0", labelFont);
    m_labelLevel = createLabel("Level\n0", labelFont);
    m_labelScore = createLabel("Score\n0", labelFont);

    m_labelRandomizer = new QLabel("Randomizer", this);
    m_labelLanguage = new QLabel("Language", this);

    m_comboLanguage = createComboBox({"english", "français"});
    m_comboRandomizer = createComboBox({"uniform randomizer", "7-bag randomizer"});

    m_buttonStart = new QPushButton("start", this);
    m_buttonPause = new QPushButton("pause", this);
    m_buttonAbout = new QPushButton("about", this);

    m_messageBox = createMessageBox();

    m_renderGame = new RendererGame();
    m_renderPreview = new RendererPreview();

    alignLabels();

    m_comboLanguage->setFixedWidth(m_comboBoxWidth);
    m_comboRandomizer->setFixedWidth(m_comboBoxWidth);
    setComboBoxFocusPolicy();

    m_layoutLanguage = createHBoxLayout({m_labelLanguage, m_comboLanguage});
    m_layoutRandomizer = createHBoxLayout({m_labelRandomizer, m_comboRandomizer});
    m_layoutButtons = createHBoxLayout({m_buttonStart, m_buttonPause, m_buttonAbout});
    
    m_layoutInformations = new QVBoxLayout();
    m_layoutInformations->addLayout(m_layoutLanguage);
    m_layoutInformations->addLayout(m_layoutRandomizer);
    m_layoutInformations->addWidget(m_labelNext);
    m_layoutInformations->addWidget(m_renderPreview);
    m_layoutInformations->addWidget(m_labelLines);
    m_layoutInformations->addWidget(m_labelLevel);
    m_layoutInformations->addWidget(m_labelScore);
    m_layoutInformations->addLayout(m_layoutButtons);

    m_renderGame->setSizePolicy(createSizePolicy());

    m_layoutMain = new QHBoxLayout();
    m_layoutMain->addWidget(m_renderGame);
    m_layoutMain->addLayout(m_layoutInformations);

    QWidget* mainWidget = new QWidget();
    mainWidget->setLayout(m_layoutMain);
    setFocusPolicy(Qt::TabFocus);
    setCentralWidget(mainWidget);
}

void MainWindow::connectWidgets() {
    connect(m_comboRandomizer, &QComboBox::currentTextChanged, this, &MainWindow::changePiecePandomizer);
    connect(m_buttonStart, &QPushButton::clicked, this, &MainWindow::initGameArea);
    connect(m_buttonPause, &QPushButton::clicked, this, &MainWindow::pauseGame);
    connect(m_buttonAbout, &QPushButton::clicked, m_messageBox, &QMessageBox::exec);
    connect(m_timer.get(), &QTimer::timeout, this, &MainWindow::updateGameArea);
}

void MainWindow::initGameArea() {
    if (m_buttonStart->text() == "resume") {
        m_timer->start();
        m_buttonStart->setText("restart");
    } else {
        resetGame();
    }
}

void MainWindow::updateGameArea() {
    if (!m_board.canMoveCurrentPieceDown()) {
        processPieceLanding();
    } else {
        moveCurrentPieceDown();
    }
    m_renderGame->update();
}

void MainWindow::blinkLines(int lineStart, int lineStop) {
    QPainterPath blinkArea;
    blinkArea.addRect(m_renderGame->getMarginLeft(), 
                      m_renderGame->getMarginTop() + m_renderGame->getCellSize() * lineStart, 
                      m_renderGame->getCellSize() * core::Board::m_width, 
                      m_renderGame->getCellSize() * (lineStop - lineStart));

    for (int i = 0; i < 3; ++i) {
        m_renderGame->setExtraShapes({blinkArea});
        m_renderGame->setExtraColor(Qt::black);
        m_renderGame->repaint();
        QThread::msleep(50);

        m_renderGame->setExtraColor(QColor(0, 0, 0, 0));
        m_renderGame->repaint();
        QThread::msleep(50);
    }

    m_renderGame->setExtraShapes({});
}

void MainWindow::addScore(int completedLines) {
    static const std::array<int, 5> scoreMultipliers = {0, 40, 100, 300, 1200};
    if (completedLines > 0 && completedLines <= 4) {
        m_score += scoreMultipliers[completedLines] * (m_level + 1);
    } else {
        throw std::runtime_error("Can't complete more than 4 lines at once.");
    }
}

void MainWindow::keyReleaseEvent(QKeyEvent* e) {
    if (e->key() == Qt::Key_Left && m_board.canMoveCurrentPieceLeft()) {
        m_board.getCurrentPiece()->moveLeft();
    } else if (e->key() == Qt::Key_Right && m_board.canMoveCurrentPieceRight()) {
        m_board.getCurrentPiece()->moveRight();
    } else if (e->key() == Qt::Key_Up && m_board.canRotateCurrentPiece()) {
        m_board.getCurrentPiece()->rotate();
    } else if (e->key() == Qt::Key_Down && m_board.canMoveCurrentPieceDown()) {
        m_board.getCurrentPiece()->moveDown();
    }
    m_renderGame->update();
}

void MainWindow::pauseGame() {
    m_timer->stop();
    m_buttonStart->setText("resume");
}

void MainWindow::changePiecePandomizer() {
    if (m_comboRandomizer->currentText().contains("uniform")) {
        m_pieceRandomizer = core::TetrominoFactory::UniformPieceRandomizer;
    } else if (m_comboRandomizer->currentText().contains("7-bag")) {
        m_pieceRandomizer = core::TetrominoFactory::BagPieceRandomizer;
    } else {
        throw std::runtime_error("Unknown piece randomizer");
    }
}

QLabel* MainWindow::createLabel(const QString& text, const QFont& font) {
    QLabel* label = new QLabel(text, this);
    label->setFont(font);
    label->setAlignment(Qt::AlignCenter);
    return label;
}

QComboBox* MainWindow::createComboBox(const QStringList& items) {
    QComboBox* comboBox = new QComboBox(this);
    comboBox->addItems(items);
    return comboBox;
}

QMessageBox* MainWindow::createMessageBox() {
    auto* msgBox = new QMessageBox(this);
    msgBox->setText("This application is written in C++14 with Qt6 and OpenGL libraries.\n"
                    "Have a look at the <a href=\"https://github.com/Unchanted/Arcade28-CPPTetris\">source code</a>.");
    msgBox->setTextFormat(Qt::RichText);
    msgBox->setTextInteractionFlags(Qt::TextBrowserInteraction);
    msgBox->setIcon(QMessageBox::Information);
    return msgBox;
}

QHBoxLayout* MainWindow::createHBoxLayout(const std::initializer_list<QWidget*>& widgets) {
    auto* layout = new QHBoxLayout();
    for (auto* widget : widgets) {
        layout->addWidget(widget);
    }
    return layout;
}

QSizePolicy MainWindow::createSizePolicy() {
    QSizePolicy spLeft(QSizePolicy::Preferred, QSizePolicy::Preferred);
    spLeft.setHorizontalStretch(2);
    return spLeft;
}

void MainWindow::alignLabels() {
    m_labelNext->setAlignment(Qt::AlignCenter | Qt::AlignBottom);
    m_labelLines->setAlignment(Qt::AlignCenter);
    m_labelLevel->setAlignment(Qt::AlignCenter);
    m_labelScore->setAlignment(Qt::AlignCenter);

    m_labelRandomizer->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_labelLanguage->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
}

void MainWindow::setComboBoxFocusPolicy() {
    m_comboRandomizer->setFocusPolicy(Qt::NoFocus);
    m_comboLanguage->setFocusPolicy(Qt::NoFocus);
}

void MainWindow::resetGame() {
    m_board.clear();
    m_board.setCurrentPiece(m_pieceRandomizer());
    m_board.setNextPiece(m_pieceRandomizer());
    m_renderGame->setBoard(&m_board);
    m_renderGame->setGameOver(false);

    m_renderPreview->setTetromino(m_board.getNextPiece());
    m_renderPreview->update();
    m_buttonStart->setText("restart");

    m_lines = 0;
    m_level = 0;
    m_score = 0;

    m_labelLines->setText("Lines\n0");
    m_labelLevel->setText("Level```cpp
#include "MainWindow.hpp"
#include <QScreen>
#include <QThread>

Tetris::gui::MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    initWindow();
    initWidgets();
    connectWidgets();
}

Tetris::gui::MainWindow::~MainWindow()
{
}

void Tetris::gui::MainWindow::initWindow() {
    setFixedSize(m_windowWidth, m_windowHeight);
    setWindowTitle("Tetris");
    move(screen()->geometry().center() - frameGeometry().center());
}

void Tetris::gui::MainWindow::initWidgets() {
    // Initialize private members
    m_pieceRandomizer = Tetris::core::TetrominoFactory::UniformPieceRandomizer;
    m_timer = std::make_unique<QTimer>(this);

    // Initialize game labels
    QFont labelFont("Courier", 12, QFont::Bold);
    initLabel(m_labelNext, "Next", labelFont);
    initLabel(m_labelLines, "Lines\n0", labelFont);
    initLabel(m_labelLevel, "Level\n0", labelFont);
    initLabel(m_labelScore, "Score\n0", labelFont);

    // Initialize option labels
    m_labelRandomizer = new QLabel("Randomizer", this);
    m_labelLanguage = new QLabel("Language", this);

    // Initialize option combo boxes
    initComboBox(m_comboLanguage, {"english"});
    initComboBox(m_comboRandomizer, {"uniform randomizer", "7-bag randomizer"});

    // Initialize buttons
    m_buttonStart = new QPushButton("start", this);
    m_buttonPause = new QPushButton("pause", this);
    m_buttonAbout = new QPushButton("about", this);

    // Initialize message box
    m_messageBox = new QMessageBox(this);
    m_messageBox->setText("This application is written in C++14 with Qt6 and OpenGL libraries.\n"
                          "Have a look at the <a href=\"https://github.com/Unchanted/Arcade28-CPPTetris\">source code</a>.");
    m_messageBox->setTextFormat(Qt::RichText);
    m_messageBox->setTextInteractionFlags(Qt::TextBrowserInteraction);
    m_messageBox->setIcon(QMessageBox::Information);

    // Initialize rendering widgets
    m_renderGame = new Tetris::gui::RendererGame();
    m_renderPreview = new Tetris::gui::RendererPreview();

    // Set alignments and focus policies
    setLabelAlignment(m_labelNext, Qt::AlignCenter | Qt::AlignBottom);
    setLabelAlignment(m_labelLines, Qt::AlignCenter);
    setLabelAlignment(m_labelLevel, Qt::AlignCenter);
    setLabelAlignment(m_labelScore, Qt::AlignCenter);
    setLabelAlignment(m_labelRandomizer, Qt::AlignLeft | Qt::AlignVCenter);
    setLabelAlignment(m_labelLanguage, Qt::AlignLeft | Qt::AlignVCenter);
    m_comboLanguage->setFixedWidth(m_comboBoxWidth);
    m_comboRandomizer->setFixedWidth(m_comboBoxWidth);
    m_comboLanguage->setFocusPolicy(Qt::NoFocus);
    m_comboRandomizer->setFocusPolicy(Qt::NoFocus);

    // Layouts
    QVBoxLayout* mainLayout = new QVBoxLayout();
    mainLayout->addLayout(createHBoxLayout(m_labelLanguage, m_comboLanguage));
    mainLayout->addLayout(createHBoxLayout(m_labelRandomizer, m_comboRandomizer));
    mainLayout->addWidget(m_labelNext);
    mainLayout->addWidget(m_renderPreview);
    mainLayout->addWidget(m_labelLines);
    mainLayout->addWidget(m_labelLevel);
    mainLayout->addWidget(m_labelScore);
    mainLayout->addLayout(createHBoxLayout({m_buttonStart, m_buttonPause, m_buttonAbout}));

    // Set size policies and main widget
    m_renderGame->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    m_renderGame->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    m_renderGame->setHorizontalStretch(2);
    QHBoxLayout* layoutMain = new QHBoxLayout();
    layoutMain->addWidget(m_renderGame);
    layoutMain->addLayout(mainLayout);
    QWidget* mainWidget = new QWidget();
    mainWidget->setLayout(layoutMain);
    setFocusPolicy(Qt::TabFocus);
    setCentralWidget(mainWidget);
}

void Tetris::gui::MainWindow::connectWidgets() {
    connect(m_comboRandomizer, &QComboBox::currentTextChanged, this, &MainWindow::changePieceRandomizer);
    connect(m_buttonStart, &QPushButton::clicked, this, &MainWindow::initGameArea);
    connect(m_buttonPause, &QPushButton::clicked, this, &MainWindow::pauseGame);
    connect(m_buttonAbout, &QPushButton::clicked, m_messageBox, &QMessageBox::exec);
    connect(m_timer.get(), &QTimer::timeout, this, &MainWindow::updateGameArea);
}

void Tetris::gui::MainWindow::initGameArea() {
    if (m_buttonStart->text() == "resume") {
        m_timer->start();
        m_buttonStart->setText("restart");
    } else {
        resetGame();
    }
}

void Tetris::gui::MainWindow::updateGameArea() {
    if (!m_board.canMoveCurrentPieceDown()) {
        handlePieceDrop();
    } else {
        m_board.getCurrentPiece()->setY(m_board.getCurrentPiece()->getY() + 1);
    }
    m_renderGame->update();
}

void Tetris::gui::MainWindow::blinkLines(int lineStart, int lineStop) {
    auto blink = [this](const QColor& color) {
        m_renderGame->setExtraColor(color);
        m_renderGame->repaint();
        QThread::msleep(50);
    };

    QPainterPath blinkArea;
    blinkArea.addRect(QRect(m_renderGame->getMarginLeft(),
                            m_renderGame->getMarginTop() + m_renderGame->getCellSize() * lineStart,
                            m_renderGame->getCellSize() * Tetris::core::Board::m_width,
                            m_renderGame->getCellSize() * (lineStop - lineStart)));
    m_renderGame->setExtraShapes({blinkArea});

    blink(Qt::black);
    blink(QColor(0, 0, 0, 0));
    blink(Qt::black);
    blink(QColor(0, 0, 0, 0));
    blink(Qt::black);

    m_renderGame->setExtraShapes({});
    m_renderGame->setExtraColor(QColor(0, 0, 0, 0));
}

void Tetris::gui::MainWindow::addScore(int completedLines) {
    static const int scoreValues[] = {0, 40, 100, 300, 1200};
    if (completedLines < 1 || completedLines > 4)
        throw std::runtime_error("Can't complete more than 4 lines at once.");
    m_score += scoreValues[completedLines] * (m_level + 1);
}

void Tetris::gui::MainWindow::keyReleaseEvent(QKeyEvent* e) {
    switch (e->key()) {
        case Qt::Key_Left:
            movePiece(m_board.canMoveCurrentPieceLeft(), -1, 0);
            break;
        case Qt::Key_Right:
            movePiece(m_board.canMoveCurrentPieceRight(), 1, 0);
            break;
        case Qt::Key_Up:
            rotatePiece(m_board.canRotateCurrentPiece());
            break;
        case Qt::Key_Down:
            movePiece(m_board.canMoveCurrentPieceDown(), 0, 1);
            break;
    }
}

void Tetris::gui::MainWindow::pauseGame() {
    m_timer->stop();
    m_buttonStart->setText("resume");
}

void Tetris::gui::MainWindow::changePieceRandomizer() {
    if (m_comboRandomizer->currentText().contains("uniform")) {
        m_pieceRandomizer = Tetris::core::TetrominoFactory::UniformPieceRandomizer;
    } else if (m_comboRandomizer->currentText().contains("7-bag")) {
        m_pieceRandomizer = Tetris::core::TetrominoFactory::BagPieceRandomizer;
    } else {
        throw std::runtime_error("Unknown piece randomizer");
    }
}

void Tetris::gui::MainWindow::initLabel(QLabel*& label, const QString& text, const QFont& font) {
    label = new QLabel(text, this);
    label->setFont(font);
}

void Tetris::gui::MainWindow::initComboBox(QComboBox*& comboBox, const QStringList& items) {
    comboBox = new QComboBox(this);
    comboBox->addItems(items);
}

QHBoxLayout* Tetris::gui::MainWindow::createHBoxLayout(QWidget* widget1, QWidget* widget2) {
    QHBoxLayout* layout = new QHBoxLayout();
    layout->addWidget(widget1);
    layout->addWidget(widget2);
    return layout;
}

QHBoxLayout* Tetris::gui::MainWindow::createHBoxLayout(const QList<QWidget*>& widgets) {
    QHBoxLayout* layout = new QHBoxLayout();
    for (auto widget : widgets) {
        layout->addWidget(widget);
    }
    return layout;
}

void Tetris::gui::MainWindow::setLabelAlignment(QLabel* label, Qt::Alignment alignment) {
    label->setAlignment(alignment);
}

void Tetris::gui::MainWindow::resetGame() {
    m_board.clear();
    m_board.setYour `MainWindow.cpp` file for your Qt Tetris game looks quite well-organized! Here are a few potential improvements and tips:

### General Structure
1. **Consistency in Naming Conventions:**
   - Ensure consistency in naming conventions. For example, if using `m_` for member variables, apply it throughout.
   
### Code Improvements
2. **Function Decomposition:**
   - Some functions, like `initWidgets()`, can be broken down further for readability and maintainability. For example, separating the initialization of game labels, option labels, combo boxes, buttons, and rendering widgets into different functions.

3. **Use of Smart Pointers:**
   - Good use of `std::unique_ptr` for `m_timer`. Consider using smart pointers for other dynamically allocated widgets where ownership is clear.

### Example: Refactoring `initWidgets()`

#### Before Refactoring:
```cpp
void Tetris::gui::MainWindow::initWidgets() {
    // Initialize private members
    m_pieceRandomizer = Tetris::core::TetrominoFactory::UniformPieceRandomizer;
    m_timer = std::make_unique<QTimer>(this);

    // Initialize game labels
    QFont labelFont("Courier", 12, QFont::Bold);
    initLabel(m_labelNext, "Next", labelFont);
    initLabel(m_labelLines, "Lines\n0", labelFont);
    initLabel(m_labelLevel, "Level\n0", labelFont);
    initLabel(m_labelScore, "Score\n0", labelFont);

    // Other initializations...
}
