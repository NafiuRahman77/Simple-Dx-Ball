#include <SFML/Graphics.hpp>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <chrono>

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;
const int PADDLE_WIDTH = 200;
const int PADDLE_HEIGHT = 20;
const int BALL_RADIUS = 10;
const int BRICK_WIDTH = 60;
const int BRICK_HEIGHT = 20;
const int BRICKS_PER_ROW = 1;
const int BRICK_ROWS = 1;
const int MAX_LIVES = 3;
const float BONUS_FALL_SPEED = 0.1f;
const float BONUS_DURATION = 200.0f;
const int PADDLE_ENLARGED_WIDTH = 300;
const int PADDLE_SHRUNKEN_WIDTH = 100;

int score = 0;

enum class GameState
{
    HomeScreen,
    Playing,
    Playing2,
    GameOver,
    YouWin
};

enum class BonusType
{
    None,
    EnlargePaddle,
    ShrinkPaddle,
    Fireball
};

class Paddle
{
public:
    Paddle(float startX, float startY)
    {
        shape.setSize(sf::Vector2f(PADDLE_WIDTH, PADDLE_HEIGHT));
        shape.setFillColor(sf::Color::Green);
        shape.setPosition(startX, startY);
    }

    void move(float dx)
    {
        shape.move(dx, 0);
        if (shape.getPosition().x < 0)
        {
            shape.setPosition(0, shape.getPosition().y);
        }
        else if (shape.getPosition().x + shape.getSize().x > WINDOW_WIDTH)
        {
            shape.setPosition(WINDOW_WIDTH - shape.getSize().x, shape.getPosition().y);
        }
    }

    void enlarge()
    {
        shape.setSize(sf::Vector2f(PADDLE_ENLARGED_WIDTH, PADDLE_HEIGHT));
    }

    void shrink()
    {
        shape.setSize(sf::Vector2f(PADDLE_SHRUNKEN_WIDTH, PADDLE_HEIGHT));
    }

    void resetSize()
    {
        shape.setSize(sf::Vector2f(PADDLE_WIDTH, PADDLE_HEIGHT));
    }

    sf::RectangleShape getShape() const
    {
        return shape;
    }

    sf::FloatRect getBounds() const
    {
        return shape.getGlobalBounds();
    }

private:
    sf::RectangleShape shape;
};

class Ball
{
public:
    Ball(float startX, float startY)
    {
        shape.setRadius(BALL_RADIUS);
        shape.setFillColor(sf::Color::Red);
        shape.setPosition(startX, startY);
        velocity.x = 0.05f;
        velocity.y = -0.3f;
        fireballActive = false;
    }

    void update()
    {
        shape.move(velocity);
        if (shape.getPosition().x < 0 || shape.getPosition().x + BALL_RADIUS * 2 > WINDOW_WIDTH)
        {
            velocity.x = -velocity.x;
        }
        if (shape.getPosition().y < 0)
        {
            velocity.y = -velocity.y;
        }
    }

    void bounce()
    {
        velocity.y = -velocity.y;
    }

    sf::CircleShape getShape() const
    {
        return shape;
    }

    sf::FloatRect getBounds() const
    {
        return shape.getGlobalBounds();
    }

    void setPosition(float x, float y)
    {
        shape.setPosition(x, y);
    }

    sf::Vector2f getVelocity() const
    {
        return velocity;
    }

    void activateFireball()
    {
        fireballActive = true;
        shape.setFillColor(sf::Color::Yellow);
    }

    void deactivateFireball()
    {
        fireballActive = false;
        shape.setFillColor(sf::Color::Red);
    }

    bool isFireballActive() const
    {
        return fireballActive;
    }

private:
    sf::CircleShape shape;
    sf::Vector2f velocity;
    bool fireballActive;
};

class Brick
{
public:
    Brick(float startX, float startY, BonusType bonusType) : bonusType(bonusType)
    {
        shape.setSize(sf::Vector2f(BRICK_WIDTH, BRICK_HEIGHT));
        shape.setFillColor(sf::Color::Blue);
        shape.setPosition(startX, startY);
    }

    sf::RectangleShape getShape() const
    {
        return shape;
    }

    sf::FloatRect getBounds() const
    {
        return shape.getGlobalBounds();
    }

    BonusType getBonusType() const
    {
        return bonusType;
    }

private:
    sf::RectangleShape shape;
    BonusType bonusType;
};

class Bonus
{
public:
    Bonus(float startX, float startY, BonusType type) : type(type)
    {
        shape.setSize(sf::Vector2f(BRICK_WIDTH / 2, BRICK_HEIGHT / 2));
        shape.setFillColor(getColorForBonusType(type));
        shape.setPosition(startX, startY);
    }

    void update()
    {
        shape.move(0, BONUS_FALL_SPEED);
    }

    sf::RectangleShape getShape() const
    {
        return shape;
    }

    sf::FloatRect getBounds() const
    {
        return shape.getGlobalBounds();
    }

    BonusType getType() const
    {
        return type;
    }

private:
    sf::RectangleShape shape;
    BonusType type;

    sf::Color getColorForBonusType(BonusType type)
    {
        switch (type)
        {
        case BonusType::EnlargePaddle:
            return sf::Color::Yellow;
        case BonusType::ShrinkPaddle:
            return sf::Color::Magenta;
        case BonusType::Fireball:
            return sf::Color::Cyan;
        default:
            return sf::Color::White;
        }
    }
};

class Score
{
public:
    std::string name;
    int score;
};

std::vector<Score> loadScores()
{
    std::vector<Score> high_scores;
    std::ifstream file("high_scores.txt");
    if (file.is_open())
    {
        std::string line;
        while (std::getline(file, line))
        {
            std::istringstream ss(line);
            Score score;
            ss >> score.name >> score.score;
            high_scores.push_back(score);
        }
        file.close();
    }
    return high_scores;
}

bool isHighScore(int score, std::vector<Score> high_scores)
{
    if (high_scores.size() < 5)
    {
        return true;
    }
    for (auto s : high_scores)
    {
        if (score > s.score)
        {
            return true;
        }
    }
    return false;
}

void saveScore(std::string name, int score, std::vector<Score> high_scores)
{
    high_scores.push_back({name, score});
    if (high_scores.size() > 5)
    {
        high_scores.erase(std::min_element(high_scores.begin(), high_scores.end(), [](Score a, Score b)
                                           { return a.score < b.score; }));
    }
    std::sort(high_scores.begin(), high_scores.end(), [](Score a, Score b)
              { return a.score > b.score; });
    std::ofstream file("high_scores.txt");
    for (auto s : high_scores)
    {
        file << s.name << " " << s.score << std::endl;
    }
    file.close();
}

bool isMouseOverText(const sf::Text &text, const sf::RenderWindow &window)
{
    sf::Vector2i mousePos = sf::Mouse::getPosition(window);
    sf::FloatRect textBounds = text.getGlobalBounds();
    return textBounds.contains(static_cast<sf::Vector2f>(mousePos));
}

void updateTextColor(sf::Text &text, const sf::RenderWindow &window)
{
    if (isMouseOverText(text, window))
    {
        text.setFillColor(sf::Color::Yellow);
    }
    else
    {
        text.setFillColor(sf::Color::White);
    }
}

void refillBricks(std::vector<Brick> &bricks, std::vector<Bonus> &bonuses)
{
    bricks.clear();
    bonuses.clear();
    for (int i = 0; i < BRICK_ROWS; ++i)
    {
        for (int j = 0; j < BRICKS_PER_ROW; ++j)
        {
            BonusType bonusType = BonusType::None;
            if (std::rand() % 5 == 0)
            {
                int bonusRand = std::rand() % 3;
                if (bonusRand == 0)
                {
                    bonusType = BonusType::EnlargePaddle;
                }
                else if (bonusRand == 1)
                {
                    bonusType = BonusType::ShrinkPaddle;
                }
                else
                {
                    bonusType = BonusType::Fireball;
                }
            }
            bricks.emplace_back(j * (BRICK_WIDTH + 10) + 30, i * (BRICK_HEIGHT + 10) + 30, bonusType);
        }
    }
}

void resetBallAndPaddle(Paddle &paddle, Ball &ball)
{
    paddle = Paddle(WINDOW_WIDTH / 2 - PADDLE_WIDTH / 2, WINDOW_HEIGHT - PADDLE_HEIGHT - 10);
    ball = Ball(WINDOW_WIDTH / 2 - BALL_RADIUS, WINDOW_HEIGHT / 2 - BALL_RADIUS);
}

int main()
{
    std::srand(static_cast<unsigned>(std::time(nullptr)));

    sf::RenderWindow window(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "DX-Ball");
    GameState gameState = GameState::HomeScreen;
    int lives = MAX_LIVES;
    float bonusTimer = 0;
    bool isBonusActive = false;
    BonusType activeBonusType = BonusType::None;

    sf::Font font;
    if (!font.loadFromFile("Font/gomarice_no_continue.ttf"))
    {
        std::cerr << "Error loading font\n";
        return 1;
    }

    sf::Text homeTextStart;
    homeTextStart.setFont(font);
    homeTextStart.setCharacterSize(36);
    homeTextStart.setFillColor(sf::Color::White);
    homeTextStart.setString("Start Game");
    homeTextStart.setPosition(WINDOW_WIDTH / 2 - homeTextStart.getLocalBounds().width / 2, WINDOW_HEIGHT / 2 - 50);

    sf::Text homeTextExit;
    homeTextExit.setFont(font);
    homeTextExit.setCharacterSize(36);
    homeTextExit.setFillColor(sf::Color::White);
    homeTextExit.setString("Exit");
    homeTextExit.setPosition(WINDOW_WIDTH / 2 - homeTextExit.getLocalBounds().width / 2, WINDOW_HEIGHT / 2 + 50);

    sf::Text gameOverTextRestart;
    gameOverTextRestart.setFont(font);
    gameOverTextRestart.setCharacterSize(36);
    gameOverTextRestart.setFillColor(sf::Color::White);
    gameOverTextRestart.setString("Restart Game");
    gameOverTextRestart.setPosition(WINDOW_WIDTH / 2 - gameOverTextRestart.getLocalBounds().width / 2, WINDOW_HEIGHT / 2 - 50);

    sf::Text gameOverTextExit;
    gameOverTextExit.setFont(font);
    gameOverTextExit.setCharacterSize(36);
    gameOverTextExit.setFillColor(sf::Color::White);
    gameOverTextExit.setString("Exit");
    gameOverTextExit.setPosition(WINDOW_WIDTH / 2 - gameOverTextExit.getLocalBounds().width / 2, WINDOW_HEIGHT / 2 + 50);

    std::string playerName;
    bool isEnteringName = false;
    sf::Text enterNameText;
    enterNameText.setFont(font);
    enterNameText.setCharacterSize(24);
    enterNameText.setFillColor(sf::Color::White);
    enterNameText.setPosition(WINDOW_WIDTH / 2 - 100, WINDOW_HEIGHT / 2 - 100);

    sf::Text youWinText;
    youWinText.setFont(font);
    youWinText.setCharacterSize(36);
    youWinText.setFillColor(sf::Color::White);
    youWinText.setString("High Score!");
    youWinText.setPosition(WINDOW_WIDTH / 2 - youWinText.getLocalBounds().width / 2, WINDOW_HEIGHT / 2 - 100);

    sf::Text youWinTextExit;
    youWinTextExit.setFont(font);
    youWinTextExit.setCharacterSize(36);
    youWinTextExit.setFillColor(sf::Color::White);
    youWinTextExit.setString("Exit");
    youWinTextExit.setPosition(WINDOW_WIDTH / 2 - youWinTextExit.getLocalBounds().width / 2, WINDOW_HEIGHT / 2 + 50);

    sf::Text livesText;
    livesText.setFont(font);
    livesText.setCharacterSize(24);
    livesText.setFillColor(sf::Color::White);
    livesText.setPosition(10, 10);

    sf::Text scoreText;
    scoreText.setFont(font);
    scoreText.setCharacterSize(24);
    scoreText.setFillColor(sf::Color::White);
    scoreText.setPosition(WINDOW_WIDTH - 100, 10);

    Paddle paddle(WINDOW_WIDTH / 2 - PADDLE_WIDTH / 2, WINDOW_HEIGHT - PADDLE_HEIGHT - 10);
    Ball ball(WINDOW_WIDTH / 2 - BALL_RADIUS, WINDOW_HEIGHT / 2 - BALL_RADIUS);

    std::chrono::milliseconds inputDelay(200);
    auto lastInputTime = std::chrono::steady_clock::now();

    std::vector<Brick> bricks;
    std::vector<Bonus> bonuses;

    refillBricks(bricks, bonuses);

    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
            {
                window.close();
            }

            if (gameState == GameState::YouWin)
            {
                if (event.type == sf::Event::TextEntered)
                {
                    if (event.text.unicode < 128)
                    {
                        char enteredChar = static_cast<char>(event.text.unicode);
                        if (enteredChar == '\b' && !playerName.empty())
                        {
                            playerName.pop_back();
                        }
                        else if (std::isalpha(enteredChar))
                        {
                            playerName += std::toupper(enteredChar);
                        }
                    }
                }

                if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Return)
                {
                    auto now = std::chrono::steady_clock::now();
                    if (now - lastInputTime > inputDelay)
                    {
                        saveScore(playerName, score, loadScores());
                        gameState = GameState::HomeScreen;
                        playerName.clear();
                        score = 0;
                        lastInputTime = now;
                    }
                }
            }

            if (gameState == GameState::HomeScreen)
            {
                if (event.type == sf::Event::MouseButtonPressed)
                {
                    if (isMouseOverText(homeTextStart, window))
                    {
                        gameState = GameState::Playing;
                        lives = MAX_LIVES;
                        paddle = Paddle(WINDOW_WIDTH / 2 - PADDLE_WIDTH / 2, WINDOW_HEIGHT - PADDLE_HEIGHT - 10);
                        ball = Ball(WINDOW_WIDTH / 2 - BALL_RADIUS, WINDOW_HEIGHT / 2 - BALL_RADIUS);
                        bricks.clear();
                        bonuses.clear();
                        for (int i = 0; i < BRICK_ROWS; ++i)
                        {
                            for (int j = 0; j < BRICKS_PER_ROW; ++j)
                            {
                                BonusType bonusType = BonusType::None;
                                if (std::rand() % 5 == 0)
                                {
                                    int bonusRand = std::rand() % 3;
                                    if (bonusRand == 0)
                                    {
                                        bonusType = BonusType::EnlargePaddle;
                                    }
                                    else if (bonusRand == 1)
                                    {
                                        bonusType = BonusType::ShrinkPaddle;
                                    }
                                    else
                                    {
                                        bonusType = BonusType::Fireball;
                                    }
                                }
                                bricks.emplace_back(j * (BRICK_WIDTH + 10) + 30, i * (BRICK_HEIGHT + 10) + 30, bonusType);
                            }
                        }
                    }
                    else if (isMouseOverText(homeTextExit, window))
                    {
                        window.close();
                    }
                }
            }
            else if (gameState == GameState::GameOver)
            {
                if (event.type == sf::Event::MouseButtonPressed)
                {
                    if (isMouseOverText(gameOverTextRestart, window))
                    {
                        gameState = GameState::Playing;
                        lives = MAX_LIVES;
                        paddle = Paddle(WINDOW_WIDTH / 2 - PADDLE_WIDTH / 2, WINDOW_HEIGHT - PADDLE_HEIGHT - 10);
                        ball = Ball(WINDOW_WIDTH / 2 - BALL_RADIUS, WINDOW_HEIGHT / 2 - BALL_RADIUS);
                        bricks.clear();
                        bonuses.clear();
                        for (int i = 0; i < BRICK_ROWS; ++i)
                        {
                            for (int j = 0; j < BRICKS_PER_ROW; ++j)
                            {
                                BonusType bonusType = BonusType::None;
                                if (std::rand() % 5 == 0)
                                {
                                    int bonusRand = std::rand() % 3;
                                    if (bonusRand == 0)
                                    {
                                        bonusType = BonusType::EnlargePaddle;
                                    }
                                    else if (bonusRand == 1)
                                    {
                                        bonusType = BonusType::ShrinkPaddle;
                                    }
                                    else
                                    {
                                        bonusType = BonusType::Fireball;
                                    }
                                }
                                bricks.emplace_back(j * (BRICK_WIDTH + 10) + 30, i * (BRICK_HEIGHT + 10) + 30, bonusType);
                            }
                        }
                    }
                    else if (isMouseOverText(gameOverTextExit, window))
                    {
                        window.close();
                    }
                }
            }
        }

        if (gameState == GameState::Playing)
        {
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left))
            {
                paddle.move(-1.0f);
            }
            else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right))
            {
                paddle.move(1.0f);
            }

            ball.update();

            if (ball.getBounds().intersects(paddle.getBounds()))
            {
                ball.bounce();
                ball.setPosition(ball.getBounds().left, paddle.getBounds().top - BALL_RADIUS * 2);
            }

            for (auto it = bricks.begin(); it != bricks.end();)
            {
                if (ball.getBounds().intersects(it->getBounds()))
                {
                    if (!ball.isFireballActive())
                    {
                        ball.bounce();
                    }
                    if (it->getBonusType() != BonusType::None)
                    {
                        bonuses.emplace_back(it->getBounds().left + BRICK_WIDTH / 2, it->getBounds().top + BRICK_HEIGHT / 2, it->getBonusType());
                    }
                    it = bricks.erase(it);
                    score++;
                }
                else
                {
                    ++it;
                }
            }

            for (auto it = bonuses.begin(); it != bonuses.end();)
            {
                it->update();
                if (it->getBounds().intersects(paddle.getBounds()))
                {
                    if (it->getType() == BonusType::EnlargePaddle)
                    {
                        paddle.enlarge();
                    }
                    else if (it->getType() == BonusType::ShrinkPaddle)
                    {
                        paddle.shrink();
                    }
                    else if (it->getType() == BonusType::Fireball)
                    {
                        ball.activateFireball();
                    }
                    activeBonusType = it->getType();
                    isBonusActive = true;
                    bonusTimer = BONUS_DURATION;
                    it = bonuses.erase(it);
                }
                else if (it->getBounds().top > WINDOW_HEIGHT)
                {
                    it = bonuses.erase(it);
                }
                else
                {
                    ++it;
                }
            }

            if (isBonusActive)
            {
                bonusTimer -= 1.0f / 60.0f; // 60 fps
                if (bonusTimer <= 0)
                {
                    paddle.resetSize();
                    ball.deactivateFireball();
                    isBonusActive = false;
                    activeBonusType = BonusType::None;
                }
            }

            if (ball.getBounds().top + BALL_RADIUS * 2 > WINDOW_HEIGHT)
            {
                lives--;
                if (lives > 0)
                {
                    ball = Ball(WINDOW_WIDTH / 2 - BALL_RADIUS, WINDOW_HEIGHT / 2 - BALL_RADIUS);
                    if (ball.isFireballActive())
                    {
                        ball.deactivateFireball();
                    }
                }
                else
                {

                    if (isHighScore(score, loadScores()))
                    {
                        gameState = GameState::YouWin;
                    }
                    else
                    {
                        gameState = GameState::GameOver;
                        score = 0;
                    }
                }
            }

            if (bricks.empty())
            {
                gameState = GameState::Playing2;
                refillBricks(bricks, bonuses);
                resetBallAndPaddle(paddle, ball);
                std::cout << "Playing2" << std::endl;
            }

            window.clear();
            window.draw(paddle.getShape());
            window.draw(ball.getShape());
            for (const auto &brick : bricks)
            {
                window.draw(brick.getShape());
            }
            for (const auto &bonus : bonuses)
            {
                window.draw(bonus.getShape());
            }
            livesText.setString("Lives: " + std::to_string(lives));
            window.draw(livesText);
            scoreText.setString("Score: " + std::to_string(score));
            window.draw(scoreText);
            window.display();
        }
        else if (gameState == GameState::Playing2)
        {

            // std::cout << "Here" << std::endl;

            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left))
            {
                paddle.move(-1.0f);
            }
            else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right))
            {
                paddle.move(1.0f);
            }

            ball.update();

            if (ball.getBounds().intersects(paddle.getBounds()))
            {
                ball.bounce();
                ball.setPosition(ball.getBounds().left, paddle.getBounds().top - BALL_RADIUS * 2);
            }

            for (auto it = bricks.begin(); it != bricks.end();)
            {
                if (ball.getBounds().intersects(it->getBounds()))
                {
                    if (!ball.isFireballActive())
                    {
                        ball.bounce();
                    }
                    if (it->getBonusType() != BonusType::None)
                    {
                        bonuses.emplace_back(it->getBounds().left + BRICK_WIDTH / 2, it->getBounds().top + BRICK_HEIGHT / 2, it->getBonusType());
                    }
                    it = bricks.erase(it);
                    score++;
                }
                else
                {
                    ++it;
                }
            }

            for (auto it = bonuses.begin(); it != bonuses.end();)
            {
                it->update();
                if (it->getBounds().intersects(paddle.getBounds()))
                {
                    if (it->getType() == BonusType::EnlargePaddle)
                    {
                        paddle.enlarge();
                    }
                    else if (it->getType() == BonusType::ShrinkPaddle)
                    {
                        paddle.shrink();
                    }
                    else if (it->getType() == BonusType::Fireball)
                    {
                        ball.activateFireball();
                    }
                    activeBonusType = it->getType();
                    isBonusActive = true;
                    bonusTimer = BONUS_DURATION;
                    it = bonuses.erase(it);
                }
                else if (it->getBounds().top > WINDOW_HEIGHT)
                {
                    it = bonuses.erase(it);
                }
                else
                {
                    ++it;
                }
            }

            if (isBonusActive)
            {
                bonusTimer -= 1.0f / 60.0f; // 60 fps
                if (bonusTimer <= 0)
                {
                    paddle.resetSize();
                    ball.deactivateFireball();
                    isBonusActive = false;
                    activeBonusType = BonusType::None;
                }
            }

            if (ball.getBounds().top + BALL_RADIUS * 2 > WINDOW_HEIGHT)
            {
                lives--;
                if (lives > 0)
                {
                    ball = Ball(WINDOW_WIDTH / 2 - BALL_RADIUS, WINDOW_HEIGHT / 2 - BALL_RADIUS);
                    if (ball.isFireballActive())
                    {
                        ball.deactivateFireball();
                    }
                }
                else
                {
                    // gameState = GameState::GameOver;
                    if (isHighScore(score, loadScores()))
                    {
                        gameState = GameState::YouWin;
                    }
                    else
                    {
                        gameState = GameState::GameOver;
                        score = 0;
                    }
                }
            }

            if (bricks.empty())
            {
                if (isHighScore(score, loadScores()))
                {
                    gameState = GameState::YouWin;
                }
                else
                {
                    gameState = GameState::GameOver;
                    score = 0;
                }
            }

            window.clear();
            window.draw(paddle.getShape());
            window.draw(ball.getShape());
            for (const auto &brick : bricks)
            {
                window.draw(brick.getShape());
            }
            for (const auto &bonus : bonuses)
            {
                window.draw(bonus.getShape());
            }
            livesText.setString("Lives: " + std::to_string(lives));
            window.draw(livesText);
            scoreText.setString("Score: " + std::to_string(score));
            window.draw(scoreText);
            window.display();
        }
        else if (gameState == GameState::HomeScreen)
        {
            homeTextStart.setFillColor(isMouseOverText(homeTextStart, window) ? sf::Color::Yellow : sf::Color::White);
            homeTextExit.setFillColor(isMouseOverText(homeTextExit, window) ? sf::Color::Yellow : sf::Color::White);
            window.clear();
            window.draw(homeTextStart);
            window.draw(homeTextExit);
            window.display();
        }
        else if (gameState == GameState::GameOver)
        {

            gameOverTextRestart.setFillColor(isMouseOverText(gameOverTextRestart, window) ? sf::Color::Yellow : sf::Color::White);
            gameOverTextExit.setFillColor(isMouseOverText(gameOverTextExit, window) ? sf::Color::Yellow : sf::Color::White);
            window.clear();
            window.draw(gameOverTextRestart);
            window.draw(gameOverTextExit);
            window.display();
        }
        else if (gameState == GameState::YouWin)
        {
            youWinText.setFillColor(isMouseOverText(youWinText, window) ? sf::Color::Yellow : sf::Color::White);
            youWinTextExit.setFillColor(isMouseOverText(youWinTextExit, window) ? sf::Color::Yellow : sf::Color::White);

            sf::Text nameText;
            nameText.setFont(font);
            nameText.setCharacterSize(24);
            nameText.setFillColor(sf::Color::White);
            nameText.setString("Enter your name: " + playerName);
            nameText.setPosition(WINDOW_WIDTH / 2 - nameText.getLocalBounds().width / 2, WINDOW_HEIGHT / 2);

            window.clear();
            window.draw(nameText);
            window.draw(youWinText);
            window.display();
        }
    }

    return 0;
}
