#include <iostream>
#include <cstdlib>
#include <ctime>
#include <vector>
#include <queue>
#include <atomic>
#include <pthread.h>
#include <unistd.h>
using namespace std;

pthread_mutex_t coutMutex = PTHREAD_MUTEX_INITIALIZER;
    	
class Player {
	private:
    	int id;
    	int boardSize;
    	atomic<int>* board;
    	int* originalBoard;
    	int position = 0;
    	int score = 0;
    	queue<pair<int, char>>& messageQueue;
    	pthread_mutex_t playerMutex = PTHREAD_MUTEX_INITIALIZER; 
    	pthread_cond_t playerCond = PTHREAD_COND_INITIALIZER;  // Declare and initialize condition variable
	public:
//constructor of Player class
    	Player(int playerId, int boardSize, atomic<int>* board, int* originalBoard, queue<pair<int, char>>& messageQueue): id(playerId), boardSize(boardSize), board(board), originalBoard(originalBoard), messageQueue(messageQueue) {
    	}

//getter functions
    	int getId() const {
        	return id;
    	}
	int getPosition() const {
	        return position;
    	}
    void move(char direction) {
        pthread_mutex_lock(&playerMutex);
        messageQueue.push(make_pair(id, direction));
        pthread_cond_signal(&playerCond);
        pthread_mutex_unlock(&playerMutex);
    }
    	int getScore() const {
        	return score;
    	}

    void play() {
        while (true) {
            // Lock mutex before accessing and updating the board
            pthread_mutex_lock(&playerMutex);

            while (messageQueue.empty())
                pthread_cond_wait(&playerCond, &playerMutex);

            auto message = messageQueue.front();
            messageQueue.pop();

            pthread_mutex_unlock(&playerMutex);

            char direction = static_cast<char>(message.second);
            int row = position / boardSize;
            int col = position % boardSize;

            // Lock mutex before accessing and updating the board
            pthread_mutex_lock(&coutMutex);
            cout << "Player " << id << " is at position: " << position << endl;
            board[position].store(0);
            pthread_mutex_unlock(&coutMutex);

            if (direction == 'W' && row > 0) {
                position -= boardSize;
            } else if (direction == 'S' && row < boardSize - 1) {
                position += boardSize;
            } else if (direction == 'A' && col > 0) {
                position--;
            } else if (direction == 'D' && col < boardSize - 1) {
                position++;
            }

            // Lock mutex before accessing and updating the board
            pthread_mutex_lock(&coutMutex);
            cout << "Player " << id << " moved to position: " << position << endl;
            board[position].store(id);
            pthread_mutex_unlock(&coutMutex);

            if (originalBoard[position] == 1) {
                score++;
                originalBoard[position] = 0;
                cout << "Player " << id << " scored! New score: " << score << endl;
            }
        }
    }	

};

int generateBoardSize(int lastdigit) {
    	int generatedNumber = rand() % 90 + 10;
    	int mulumber = generatedNumber * lastdigit;
    	double divResult = static_cast<double>(lastdigit) / static_cast<double>(generatedNumber);
    	double mod= divResult - static_cast<int>(divResult / 25) * 25;
    	if (mod < 10) {
    	    mod += 15;
    	}
    	int boardSize = static_cast<int>(mod + 0.5); // Round to the nearest integer
    	return boardSize;
	}
void initializeBoard(int* board, int size) {
    	for (int i = 0; i < size * size; ++i) {
        	board[i] = rand() % 2;
    	}
}
void displayBoard(int* board, int size, const Player& player1, const Player& player2, char lastMove, int lastScore) {
    system("clear"); // Clear the console screen (works on Unix-based systems)
    cout << "-----------------------------" << endl;
    cout << "|         Board            |" << endl;
    cout << "-----------------------------" << endl;
    
    for (int i = 0; i < size; ++i) {
        cout << "| ";
        for (int j = 0; j < size; ++j) {
            int cellValue = board[i * size + j];
            if (player1.getPosition() == i * size + j) {
                cout << "P" << player1.getId() << " ";
            } else if (player2.getPosition() == i * size + j) {
                cout << "P" << player2.getId() << " ";
            } else {
                cout << cellValue << " ";
            }
        }
        cout << "|" << endl;
    }
    
    cout << "-----------------------------" << endl;
    cout << "Player " << player1.getId() << " Score: " << player1.getScore() << "   ";
    cout << "Player " << player2.getId() << " Score: " << player2.getScore() << endl;
    cout << "-----------------------------" << endl;
    cout << "Last Move: " << lastMove << "   Last Score: " << lastScore << endl;
    cout << "-----------------------------" << endl;
    cout << "Player " << player1.getId() << "'s turn. Enter direction (W/A/S/D): ";
}

int main() {
    srand(static_cast<unsigned int>(time(nullptr)));
    int boardSize = generateBoardSize(7); // create board first size
    atomic<int>* board = new atomic<int>[boardSize * boardSize]; // The dynamic array is declared as an atomic array, representing a game board cell, ensuring atomic operations, particularly important when multiple threads are involved.
    int* originalBoard = new int[boardSize * boardSize]; // dynamic board created

    initializeBoard(originalBoard, boardSize); // initialise the board with 0 and 1 for scores.

    for (int i = 0; i < boardSize * boardSize; ++i) {
        board[i].store(originalBoard[i]);
    }

    queue<pair<int, char>> messageQueue; // here queue is storing msgs for each pair..their id and the move(ASWD)

    vector<Player*> players = { // creating pairs
        new Player(1, boardSize, board, originalBoard, messageQueue),
        new Player(2, boardSize, board, originalBoard, messageQueue)
    };

    pthread_t playerThreads[2]; // 2 threads for each player

    for (size_t i = 0; i < players.size(); ++i) {
        Player* player = players[i];
        pthread_create(&playerThreads[i], nullptr, [](void* arg) -> void* {
            Player* player = static_cast<Player*>(arg);
            player->play();
            return nullptr;
        }, player); // argument passed to the thread function corresponds to the each player in the loop
    }

    char lastMove = ' ';
    int lastScore = 0;

    while (true) { // main game loop 
        for (size_t i = 0; i < players.size(); ++i) {
            displayBoard(originalBoard, boardSize, *players[0], *players[1], lastMove, lastScore);
            cout << "Player " << players[i]->getId() << "'s turn. Enter direction (W/A/S/D): ";
            char direction;
            cin >> direction;
            players[i]->move(direction);
            lastMove = direction;
            lastScore = players[i]->getScore();
        }
    }

    // Join or detach threads before the program exits
    for (size_t i = 0; i < players.size(); ++i) {
        pthread_join(playerThreads[i], nullptr);
    }

    for (size_t i = 0; i < players.size(); ++i) {
        delete players[i]; // delete the dynamically allocated players
    }

    delete[] board;
    delete[] originalBoard;

    return 0;
}
