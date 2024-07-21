#include <SFML/Graphics.hpp>
#include <list>
#include <random>
#include <iostream>
#include <chrono>

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;
const int GRID_SIZE = 20;
const int INITIAL_SNAKE_LENGTH = 3;
const int FOOD_SIZE = 20;
const int NUM_WALLS = 10;

struct SnakeSegment {
    int x, y;
    SnakeSegment(int _x, int _y) : x(_x), y(_y) {}
};

enum class Direction { Up, Down, Left, Right };

enum class GameState { Playing, GameOver };

class SnakeGame {
public:

    void updateFoodCounterText() {
        foodCounter.setString("Food: " + std::to_string(foodCount));
    }

    SnakeGame() : window(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "Snake Game") {
        gameState = GameState::Playing; // Start in the playing state
        snake.push_back(SnakeSegment(WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2));
        direction = Direction::Right;

        if (foodTexture.loadFromFile("C:/Users/theon/Downloads/food.png")) {
            foodSprite.setTexture(foodTexture);
            foodSprite.setScale(static_cast<float>(FOOD_SIZE) / foodTexture.getSize().x, static_cast<float>(FOOD_SIZE) / foodTexture.getSize().y);
        }
        else {
            std::cerr << "Failed to load food.png! A red block will be used instead." << std::endl;
            foodRect.setSize(sf::Vector2f(FOOD_SIZE, FOOD_SIZE));
            foodRect.setFillColor(sf::Color::Red);
        }

        if (backgroundTexture.loadFromFile("C:/Users/theon/Downloads/grass.png")) {
            backgroundTexture.setRepeated(true);
            backgroundSprite.setTexture(backgroundTexture);
            backgroundSprite.setTextureRect(sf::IntRect(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT));
        }

        if (snakeTexture.loadFromFile("C:/Users/theon/Downloads/snake.png")) {
            snakeSprite.setTexture(snakeTexture);
            snakeSprite.setScale(static_cast<float>(GRID_SIZE) / snakeTexture.getSize().x, static_cast<float>(GRID_SIZE) / snakeTexture.getSize().y);
        }
        else {
            std::cerr << "Failed to load snake.png! A blue block will be used instead." << std::endl;
            snakeRect.setSize(sf::Vector2f(GRID_SIZE, GRID_SIZE));
            snakeRect.setFillColor(sf::Color::Blue);
        }

        // Load the wall texture
        if (wallTexture.loadFromFile("C:/Users/theon/Downloads/wall.png")) {
            wallSprite.setTexture(wallTexture);
        }
        else {
            std::cerr << "Failed to load wall.png! No walls will be used." << std::endl;
        }

        spawnFood();
        spawnWalls();

        // Initialize the food count and the food counter text
        foodCount = 0;
        font.loadFromFile("C:/Users/theon/Downloads/YoungSerif-Regular.ttf");
        foodCounter.setFont(font);
        foodCounter.setCharacterSize(20);
        foodCounter.setFillColor(sf::Color::White);
        foodCounter.setPosition(10, 10);
        updateFoodCounterText();

        // Initialize game over screen properties
        gameOverText.setFont(font);
        gameOverText.setCharacterSize(40);
        gameOverText.setFillColor(sf::Color::Red);
        gameOverText.setString("Game Over");
        sf::FloatRect textBounds = gameOverText.getLocalBounds();
        gameOverText.setPosition(WINDOW_WIDTH / 2 - textBounds.width / 2, WINDOW_HEIGHT / 2 - textBounds.height / 2);

        // Game over animation properties
        gameOverAnimationSpeed = 0.1f;
        gameOverAnimationTimer.restart();
        gameOverAnimationProgress = 0.0f;

        // Additional game over properties
        gameOverTime = std::chrono::seconds(3);  // Game over screen displays for 3 seconds
        gameEndTime = std::chrono::steady_clock::time_point();
    }

    void run() {
        sf::Clock clock;
        float deltaTime = 0.2f;
        bool canMove = true;  // To delay wall collision at the start
        while (window.isOpen()) {
            handleEvents();

            if (gameState == GameState::Playing) {
                if (clock.getElapsedTime().asSeconds() >= deltaTime) {
                    if (canMove) {
                        canMove = false;
                    }
                    else {
                        update();
                    }
                    render();
                    clock.restart();
                }
            }
            else if (gameState == GameState::GameOver) {
                // Animate the game over screen
                animateGameOverScreen();
                renderGameOverScreen();

                // Check if it's time to close the window
                if (gameEndTime == std::chrono::time_point<std::chrono::steady_clock>() && gameOverAnimationProgress == 1.0f) {
                    gameEndTime = std::chrono::steady_clock::now() + gameOverTime;
                }

                if (gameEndTime != std::chrono::time_point<std::chrono::steady_clock>() && std::chrono::steady_clock::now() > gameEndTime) {
                    window.close();
                }
            }
        }
    }

private:
    sf::RenderWindow window;
    std::list<SnakeSegment> snake;
    Direction direction;
    int foodX, foodY;

    sf::Texture foodTexture;
    sf::Sprite foodSprite;
    sf::RectangleShape foodRect;

    sf::Texture backgroundTexture;
    sf::Sprite backgroundSprite;

    sf::Texture snakeTexture;
    sf::Sprite snakeSprite;
    sf::RectangleShape snakeRect;

    sf::Texture wallTexture;
    sf::Sprite wallSprite;
    std::vector<sf::Vector2f> walls;

    int foodCount;
    sf::Font font;
    sf::Text foodCounter;

    GameState gameState; // Added game state

    // Game Over Screen
    sf::Text gameOverText;
    sf::Clock gameOverAnimationTimer;
    float gameOverAnimationSpeed;
    float gameOverAnimationProgress;

    std::chrono::seconds gameOverTime;
    std::chrono::steady_clock::time_point gameEndTime;

    void handleEvents() {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }
            else if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::Up && direction != Direction::Down) {
                    direction = Direction::Up;
                }
                else if (event.key.code == sf::Keyboard::Down && direction != Direction::Up) {
                    direction = Direction::Down;
                }
                else if (event.key.code == sf::Keyboard::Left && direction != Direction::Right) {
                    direction = Direction::Left;
                }
                else if (event.key.code == sf::Keyboard::Right && direction != Direction::Left) {
                    direction = Direction::Right;
                }
            }
        }
    }

    void update() {
        moveSnake();
        checkCollision();
        checkFoodCollision();
    }

    void moveSnake() {
        int headX = snake.front().x;
        int headY = snake.front().y;

        if (direction == Direction::Up) {
            headY -= GRID_SIZE;
        }
        else if (direction == Direction::Down) {
            headY += GRID_SIZE;
        }
        else if (direction == Direction::Left) {
            headX -= GRID_SIZE;
        }
        else if (direction == Direction::Right) {
            headX += GRID_SIZE;
        }

        if (headX >= WINDOW_WIDTH) {
            headX = 0;
        }
        else if (headX < 0) {
            headX = WINDOW_WIDTH - GRID_SIZE;
        }
        if (headY >= WINDOW_HEIGHT) {
            headY = 0;
        }
        else if (headY < 0) {
            headY = WINDOW_HEIGHT - GRID_SIZE;
        }

        snake.push_front(SnakeSegment(headX, headY));

        if (headX == foodX && headY == foodY) {
            // Snake eats food
            foodCount++;  // Increment the food count
            updateFoodCounterText();  // Update the food counter text
            spawnFood();
        }
        else {
            snake.pop_back();
        }
    }

    void checkCollision() {
        int headX = snake.front().x;
        int headY = snake.front().y;

        for (auto it = std::next(snake.begin()); it != snake.end(); ++it) {
            if (it->x == headX && it->y == headY) {
                gameState = GameState::GameOver;
                return;
            }
        }

        for (const sf::Vector2f& wall : walls) {
            if (headX == wall.x && headY == wall.y) {
                gameState = GameState::GameOver;
                return;
            }
        }
    }

    void checkFoodCollision() {
        int headX = snake.front().x;
        int headY = snake.front().y;

        if (headX == foodX && headY == foodY) {
            spawnFood();

            // Extend the snake by adding a new segment at the last position
            snake.push_back(SnakeSegment(headX, headY));
        }
    }


    void spawnFood() {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<int> xDist(0, (WINDOW_WIDTH / GRID_SIZE) - 1);
        std::uniform_int_distribution<int> yDist(0, (WINDOW_HEIGHT / GRID_SIZE) - 1);

        int maxAttempts = 100;
        int attempts = 0;

        while (attempts < maxAttempts) {
            foodX = xDist(gen) * GRID_SIZE;
            foodY = yDist(gen) * GRID_SIZE;

            // Ensure the food doesn't coincide with the walls or food counter
            bool foodValid = true;
            for (const sf::Vector2f& wall : walls) {
                if (foodX == wall.x && foodY == wall.y) {
                    foodValid = false;
                    break;
                }
            }

            for (const SnakeSegment& segment : snake) {
                if (foodX == segment.x && foodY == segment.y) {
                    foodValid = false;
                    break;
                }
            }

            if (foodX == foodCounter.getPosition().x && foodY == foodCounter.getPosition().y) {
                foodValid = false;
            }

            if (foodValid) {
                return;
            }

            attempts++;
        }
        // If we couldn't find a valid location in the given attempts, the food remains in its current position.
    }

    void spawnWalls() {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<int> xDist(0, (WINDOW_WIDTH / GRID_SIZE) - 1);
        std::uniform_int_distribution<int> yDist(0, (WINDOW_HEIGHT / GRID_SIZE) - 1);

        for (int i = 0; i < NUM_WALLS; ++i) {
            int x, y;
            bool validWall = false;
            int wallWidth = (std::rand() % 4 + 1) * GRID_SIZE;  // Random width up to 4 blocks
            int wallHeight = (std::rand() % 4 + 1) * GRID_SIZE; // Random height up to 4 blocks

            while (!validWall) {
                x = xDist(gen) * GRID_SIZE;
                y = yDist(gen) * GRID_SIZE;

                // Ensure that the walls don't spawn near the snake in the start
                bool tooCloseToSnake = false;
                for (const SnakeSegment& segment : snake) {
                    if (abs(x - segment.x) < wallWidth || abs(y - segment.y) < wallHeight) {
                        tooCloseToSnake = true;
                        break;
                    }
                }

                if (tooCloseToSnake) {
                    continue;
                }

                validWall = true;
                for (const sf::Vector2f& wall : walls) {
                    if (abs(wall.x - x) < wallWidth && abs(wall.y - y) < wallHeight) {
                        validWall = false;
                        break;
                    }
                }
            }

            for (int wx = 0; wx < wallWidth; wx += GRID_SIZE) {
                for (int wy = 0; wy < wallHeight; wy += GRID_SIZE) {
                    walls.push_back(sf::Vector2f(x + wx, y + wy));
                }
            }
        }
    }

    void render() {
        window.clear();

        window.draw(backgroundSprite);

        if (foodTexture.getSize().x > 0) {
            foodSprite.setPosition(foodX, foodY); // Set the position of foodSprite
            window.draw(foodSprite);
        }
        else {
            foodRect.setPosition(foodX, foodY);
            window.draw(foodRect);
        }

        for (const sf::Vector2f& wall : walls) {
            wallSprite.setPosition(wall);
            window.draw(wallSprite);
        }

        for (const SnakeSegment& segment : snake) {
            if (snakeTexture.getSize().x > 0) {
                snakeSprite.setPosition(segment.x, segment.y);
                window.draw(snakeSprite);
            }
            else {
                snakeRect.setPosition(segment.x, segment.y);
                window.draw(snakeRect);
            }
        }

        // Draw the food counter text
        window.draw(foodCounter);

        window.display();
    }

    void animateGameOverScreen() {
        float elapsed = gameOverAnimationTimer.getElapsedTime().asSeconds();
        gameOverAnimationProgress = std::min(1.0f, elapsed / gameOverAnimationSpeed);
    }

    void renderGameOverScreen() {
        // Render the game elements behind the game over screen
        render();

        // Calculate the opacity for the "Game Over" text
        sf::Uint8 textOpacity = static_cast<sf::Uint8>(255 * gameOverAnimationProgress);

        // Create a text object for "Game Over"
        gameOverText.setFillColor(sf::Color(255, 0, 0, textOpacity));

        // Clear the window
        window.clear();

        // Draw the game elements
        window.draw(backgroundSprite);

        if (foodTexture.getSize().x > 0) {
            window.draw(foodSprite);
        }
        else {
            foodRect.setPosition(foodX, foodY);
            window.draw(foodRect);
        }

        for (const sf::Vector2f& wall : walls) {
            wallSprite.setPosition(wall);
            window.draw(wallSprite);
        }

        for (const SnakeSegment& segment : snake) {
            if (snakeTexture.getSize().x > 0) {
                snakeSprite.setPosition(segment.x, segment.y);
                window.draw(snakeSprite);
            }
            else {
                snakeRect.setPosition(segment.x, segment.y);
                window.draw(snakeRect);
            }
        }

        // Draw the "Game Over" text
        window.draw(gameOverText);

        // Draw the food counter text
        window.draw(foodCounter);

        // Display the window
        window.display();
    }

};

int main() {
    SnakeGame game;
    game.run();
    return 0;
}