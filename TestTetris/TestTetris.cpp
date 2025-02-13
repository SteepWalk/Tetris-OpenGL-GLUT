#include <GL/glut.h>
#include <random>
#include <iostream>
#include <string.h>
#include <ctime>
#include <string>
#include <fstream>
#include <vector>

using namespace std;

const int nRow = 23; //4 more in top to accomodate hidden pieces
const int nColumn = 12; //2 more in each side to accomdate empty slots in the piece grid
const int BOX_SIZE = 25; //25 pixel box length
const int incet = 50; 
bool gameOver = false; 
int startPosX = incet; 
int startPosY = incet;
const int nColors = 4;
const int GAP = 1; //1 pixel space between boxes
int score = 0;
char buffer[10];
char frameId = 'm';

struct Color {
	GLclampf red; //GLclampf = 0 - 255 float. Glut datatype
	GLclampf green;
	GLclampf blue;
	

	Color() : red(0.0), green(0.0), blue(0.0) {

	}
};

Color RED;
Color GREEN;
Color BLUE;
Color GREY;
Color* colors[nColors];

struct Line { //Linked list, datatype
	int* line; //line = array of boxes
	int row;
	Line* next; //next line address

	Line(int row) : row(row), next(nullptr) {
		line = new int[nColumn];

		//Green (3) = hidden slot at the top, Blue (2) = boundaries
		int rowBasedValue = (row < 4) ? 2 : (nRow - 1 - row < 1) ? 3 : 0;
		for (int i = 0; i < nColumn; i++) {
			line[i] = rowBasedValue;
			line[i] = (i < 1 || nColumn - 1 - i < 1) ? 3 : line[i];
		}
	}

	~Line() {
		delete[] line;
	}
};

struct LineListController {
	Line* head; // * means pointer
	Line* tail;

	LineListController() : head(nullptr), tail(nullptr) {}

	~LineListController() {
		while (head != nullptr) {
			Line* temp = head;
			head = head->next;
			delete temp;
		}
	}

	// Insert line at the end of the list
	void push(int row) { // 
		Line* newLine = new Line(row);
		if (head == nullptr) {
			head = newLine;
			tail = newLine;
		}
		else {
			tail->next = newLine;
			tail = newLine;
		}
	}

	void remove(int row) {
		if (head == nullptr) return;

		if (head->row == row) {
			Line* temp = head;
			head = head->next;
			if (head == nullptr) tail = nullptr; 
			delete temp;

			insertLineAfterSpecPos(3);
			return;
		}

		Line* prev = head;
		while (prev->next != nullptr && prev->next->row != row) {
			prev = prev->next;
		}

		if (prev->next == nullptr) return;

		Line* toDelete = prev->next;
		prev->next = toDelete->next;
		if (toDelete == tail) {
			tail = prev; 
		}
		Line* nextNode = toDelete->next;
		delete toDelete;

		insertLineAfterSpecPos(3);
	}

	//Inserts a node after a specified node
	void insertLineAfterSpecPos(int row) {
		Line* node = find(row);

		if (node == nullptr) return;
		Line* newNode = new Line(row + 1);
		Line* nodeAfter = node->next;
		node->next = newNode;
		newNode->next = nodeAfter;

		//Adjust row numbers
		Line* temp = head;
		int counter = 0;
		while (temp != nullptr) {
			temp->row = counter;
			counter++;
			temp = temp->next;
		}
	}

	Line* next(Line* current) {
		return (current != nullptr) ? current->next : nullptr;
	}

	Line* find(int row) {
		Line* temp = head;
		while (temp != nullptr) {
			if (temp->row == row) return temp;
			temp = temp->next;
		}
		return nullptr;
	}
};

struct Tetromino { //L I O Z
	int grid[16]; //array 0, 1
	int ori = 0; //Orientation 0 = upright; 1 = toLeft; 2 = upside down; 3 = toRight;
};


struct CurrentPiece {
	Tetromino* piece = NULL;
	int col = 5; //centered by default
	int row = 0; //top by default
	Line* line = nullptr; //starting position within the playing field

};



Tetromino _i;
Tetromino _t;
Tetromino _l;
Tetromino _s;
Tetromino _z;
Tetromino _o;
Tetromino _j;

Tetromino* tetrominoes[7];

CurrentPiece curPiece;

LineListController pField;



void myInit() {
	glClearColor(0.8, 0.8, 0.8, 1.0); //Background
	glMatrixMode(GL_PROJECTION); //Projection 2D 
	glLoadIdentity(); //reset
	gluOrtho2D(0, 640, 650, 0); //projection area = window size
}


void gameInit() { //All of the variables needed for the game are initialized here

	//Inizializing the tetrominoes
	// Tetromino I
	for (int i = 0; i < 16; i++) _i.grid[i] = 0;
	_i.grid[2] = 1;  // Middle column
	_i.grid[6] = 1;
	_i.grid[10] = 1;
	_i.grid[14] = 1;

	tetrominoes[0] = &_i;
	// Tetromino L
	for (int i = 0; i < 16; i++) _l.grid[i] = 0;
	_l.grid[1] = 1;  // One block offset to the left
	_l.grid[5] = 1;
	_l.grid[9] = 1;
	_l.grid[10] = 1;

	tetrominoes[1] = &_l;
	// Tetromino J
	for (int i = 0; i < 16; i++) _j.grid[i] = 0;
	_j.grid[2] = 1;  // One block offset to the right
	_j.grid[6] = 1;
	_j.grid[10] = 1;
	_j.grid[9] = 1;

	tetrominoes[2] = &_j;

	// Tetromino O
	for (int i = 0; i < 16; i++) _o.grid[i] = 0;
	_o.grid[5] = 1;  // Centered as a square block
	_o.grid[6] = 1;
	_o.grid[9] = 1;
	_o.grid[10] = 1;

	tetrominoes[3] = &_o;

	// Tetromino S
	for (int i = 0; i < 16; i++) _s.grid[i] = 0;
	_s.grid[6] = 1;  // Centered and shifted for shape
	_s.grid[7] = 1;
	_s.grid[9] = 1;
	_s.grid[10] = 1;

	tetrominoes[4] = &_s;

	// Tetromino Z
	for (int i = 0; i < 16; i++) _z.grid[i] = 0;
	_z.grid[4] = 1;  // Centered and shifted for shape
	_z.grid[5] = 1;
	_z.grid[9] = 1;
	_z.grid[10] = 1;

	tetrominoes[5] = &_z;

	// Tetromino T
	for (int i = 0; i < 16; i++) _t.grid[i] = 0;
	_t.grid[6] = 1;  // Centered T-shape
	_t.grid[9] = 1;
	_t.grid[10] = 1;
	_t.grid[11] = 1;

	tetrominoes[6] = &_t;

	//Initialize rows
	for (int i = 0; i < nRow; i++) pField.push(i);

	//Initialize color
	RED.red = 0.6;
	GREEN.green = 0.6;
	BLUE.blue = 0.6;
	GREY.red = 0.1;
	GREY.green = 0.1;
	GREY.blue = 0.1;
	colors[0] = &GREY;
	colors[1] = &RED;
	colors[2] = &GREEN;
	colors[3] = &BLUE;

}

void drawString(void* font, const char* ch, int x, int y) {

	unsigned int i;
	glPushAttrib(GL_CURRENT_BIT);
	glRasterPos2i(x, y);
	for (i = 0; i < strlen(ch); i++) {
		glutBitmapCharacter(font, ch[i]);
	}
	glPopAttrib();
}

void drawScore() {
	glColor3f(0.8, 0.8, 0.8);
	glRecti(500, 30, 640, 130);

	glColor3f(0, 0, 0);
	drawString(GLUT_BITMAP_TIMES_ROMAN_24, "Score:", 520, 50);
	string scoreStr = to_string(score);
	drawString(GLUT_BITMAP_TIMES_ROMAN_24, scoreStr.c_str(), 590, 50);

	drawString(GLUT_BITMAP_TIMES_ROMAN_24, buffer, 590, 50);
}

void displayMenu() {
	glClear(GL_COLOR_BUFFER_BIT);

	glColor3f(0.2, 0.2, 0.2);
	glRecti(10, 10, 630, 640);

	glColor3f(1.0, 1.0, 1.0);
	drawString(GLUT_BITMAP_HELVETICA_18, "Main Menu", 260, 210);
	drawString(GLUT_BITMAP_HELVETICA_18, "Press 1 for 'New Game'", 250, 270);
	drawString(GLUT_BITMAP_HELVETICA_18, "Press 2 for 'Score History'", 250, 300);
	drawString(GLUT_BITMAP_HELVETICA_18, "Press 3 for 'How to Play'", 250, 330);

	glFlush();
}

void displayGameOver() {
	glClear(GL_COLOR_BUFFER_BIT);

	glColor3f(0.2, 0.0, 0.0);
	glRecti(10, 10, 630, 640);

	glColor3f(1.0, 0.0, 0.0);
	drawString(GLUT_BITMAP_HELVETICA_18, "GAME OVER", 250, 100);

	glColor3f(1.0, 1.0, 1.0); 
	drawString(GLUT_BITMAP_HELVETICA_18, "Final Score:", 250, 150);
	std::string scoreStr = std::to_string(score);
	drawString(GLUT_BITMAP_HELVETICA_18, scoreStr.c_str(), 350, 150);

	drawString(GLUT_BITMAP_HELVETICA_18, "Press 1 to Start a New Game", 200, 300);
	drawString(GLUT_BITMAP_HELVETICA_18, "Press M to Return to Main Menu", 200, 350);

	glFlush();
}


void createPlayingField() {
	glColor3f(colors[0]->red, colors[0]->blue, colors[0]->green);
	Line* line = pField.head;
	int rowCounter = 0;

	while (line != nullptr) {
		for (int i = 0; i < nColumn; i++) {//12 BOXES
			int colorIndex = line->line[i] % nColors;
			glColor3f(colors[colorIndex]->red, colors[colorIndex]->green, colors[colorIndex]->blue);

			glRecti(
				(startPosX + (BOX_SIZE * i) + GAP),
				(startPosY + (BOX_SIZE * line->row) + GAP),
				(startPosX + (BOX_SIZE * i)) + BOX_SIZE,
				(startPosY + (BOX_SIZE * line->row)) + BOX_SIZE
			);
		}

		line = line->next;
	}
	glFlush();
}
void storeScore(int score);

void clearPlayingField() {
	Line* line = pField.head;

	while (line != nullptr) {
		for (int i = 0; i < nColumn; i++) {
			// Clear any "1" elements representing pieces
			if (line->line[i] == 1) {
				line->line[i] = 0;
			}
		}
		line = line->next;
	}
}

void lockPiece() {
	if (curPiece.piece == nullptr) return;
	if (curPiece.line == nullptr) return;

	Line* line = curPiece.line;
	int col = curPiece.col;

	for (int i = 0; i < 16; i++) {
		if (i != 0 && i % 4 == 0 && line != nullptr) {
			line = line->next;
		}
		if (curPiece.piece->grid[i] == 0) continue;

		int row = curPiece.row + (i / 4);
		if (row < 4) {
			gameOver = true;
			cout << "Game Over: Piece locked in top rows!" << endl;
			storeScore(score);
			frameId = 'o';
			glutPostRedisplay();
			return;
		}

		int index = col + (i % 4);
		line->line[index % nColumn] = 1;
	}

	cout << "Piece locked at row: " << curPiece.row << endl;
}


bool canPieceMove(int dir) {
	if (curPiece.piece == nullptr) return false;
	if (curPiece.line == nullptr) return false;

	int destCol = curPiece.col;
	Line* line = curPiece.line;

	//0 is down, 1 is right and 2 is left
	switch (dir) {
	case 0:
		if (line == nullptr) {
			cout << "Condition one" << endl;
			return false;
		}
		else {
			line = line->next;
		}
		break;

	case 1:
		destCol++;
		break;

	case 2:
		destCol--;
		break;

	}

	for (int i = 0; i < 16; i++) {
		if (i != 0 && i % 4 == 0 && line != nullptr) {
			line = line->next;
		}
		if (curPiece.piece->grid[i] == 0) continue;
		if (line == nullptr) { cout << "Condition two" << endl;return false; }
		int col = destCol + (i % 4);
		if (nColumn - col < 1 || col < 0) { cout << "Condition three" << endl;return false; }
		if (line->line[col] != 0 && line->line[col] != 2) { cout << "Condition four" << endl;cout << line->row << endl;return false; }
	}

	return true;
}



void storeScore(int score) {
	ifstream inputFile("scores.txt");
	vector<string> lines;

	string line;
	while (getline(inputFile, line)) {
		lines.push_back(line);
	}
	inputFile.close();

	time_t now = time(0);
	tm localTime;
	localtime_s(&localTime, &now);

	char dateBuffer[20];
	strftime(dateBuffer, sizeof(dateBuffer), "%Y-%m-%d %H:%M:%S", &localTime);

	string newScoreEntry = "Score: " + to_string(score) + " ----------- Date: " + dateBuffer;
	lines.push_back(newScoreEntry);

	if (lines.size() > 10) {
		lines.erase(lines.begin());
	}

	ofstream outputFile("scores.txt");
	for (const auto& entry : lines) {
		outputFile << entry << "\n";
	}
	outputFile.close();
}


void displayScores() {
	glClear(GL_COLOR_BUFFER_BIT);

	glColor3f(0.2, 0.2, 0.2);
	glRecti(10, 10, 630, 640);

	glColor3f(1.0, 1.0, 1.0);
	drawString(GLUT_BITMAP_HELVETICA_18, "High Scores", 260, 50);

	ifstream inputFile("scores.txt");
	string line;
	int yPosition = 100;

	glColor3f(1.0, 1.0, 1.0);
	while (getline(inputFile, line)) {
		drawString(GLUT_BITMAP_HELVETICA_18, line.c_str(), 50, yPosition);
		yPosition += 30; 
	}
	inputFile.close();

	drawString(GLUT_BITMAP_HELVETICA_18, "Press M to return to Main Menu", 180, 600);

	glFlush();
}

void displayHowToPlay() {
	glClear(GL_COLOR_BUFFER_BIT);

	glColor3f(0.2, 0.2, 0.2); 
	glRecti(10, 10, 630, 640);

	glColor3f(1.0, 1.0, 1.0);
	drawString(GLUT_BITMAP_HELVETICA_18, "How to Play", 250, 50);

	drawString(GLUT_BITMAP_HELVETICA_18, "Use the following keys to control the game:", 50, 100);

	glColor3f(1.0, 1.0, 0.0);
	drawString(GLUT_BITMAP_HELVETICA_18, "Controls:", 50, 150);

	glColor3f(1.0, 1.0, 1.0);
	drawString(GLUT_BITMAP_HELVETICA_18, "  W - Rotate piece", 50, 180);
	drawString(GLUT_BITMAP_HELVETICA_18, "  A - Move piece left", 50, 210);
	drawString(GLUT_BITMAP_HELVETICA_18, "  D - Move piece right", 50, 240);
	drawString(GLUT_BITMAP_HELVETICA_18, "  S - Move piece down", 50, 270);
	drawString(GLUT_BITMAP_HELVETICA_18, "  P - Pause/Unpause game", 50, 300);

	glColor3f(1.0, 1.0, 0.0); 
	drawString(GLUT_BITMAP_HELVETICA_18, "Game Rules:", 50, 350);

	glColor3f(1.0, 1.0, 1.0); 
	drawString(GLUT_BITMAP_HELVETICA_18, "  - Arrange pieces to form complete lines.", 50, 380);
	drawString(GLUT_BITMAP_HELVETICA_18, "  - Complete lines are cleared for points.", 50, 410);
	drawString(GLUT_BITMAP_HELVETICA_18, "  - The game ends when pieces reach the top.", 50, 440);

	drawString(GLUT_BITMAP_HELVETICA_18, "Press M to return to Main Menu", 180, 600);

	glFlush();
}


bool movePiece(int dir);

void timerCallback(int value) {
	if (!gameOver && frameId == 'g') {
		movePiece(0);

		glutTimerFunc(250, timerCallback, 0);
	}
}


bool doesPieceFit() {
	if (curPiece.piece == nullptr) return false;
	if (curPiece.line == nullptr) return false;

	int destCol = curPiece.col;
	Line* line = curPiece.line;

	for (int i = 0; i < 16; i++) {
		if (i != 0 && i % 4 == 0 && line != nullptr) {
			line = line->next;
		}
		if (curPiece.piece->grid[i] == 0) continue;
		if (line == nullptr) { cout << "Condition two" << endl;return false; }
		int col = destCol + (i % 4);
		if (nColumn - col < 1 || col < 0) { cout << "Condition three" << endl;return false; }
		if (line->line[col] != 0 && line->line[col] != 2) { cout << "Condition four" << endl;cout << line->row << endl;return false; }
	}

	return true;
}

void drawPiece() {
	if (curPiece.piece == nullptr) return;
	if (curPiece.line == nullptr) return;

	glColor3f(colors[1]->red, colors[1]->blue, colors[1]->green);

	for (int i = 0; i < 16; i++) {
		if (curPiece.piece->grid[i] == 1) {
			int col = curPiece.col + (i % 4);
			int row = curPiece.row + (i / 4);
			glRecti(
				(startPosX + (BOX_SIZE * col) + GAP),
				(startPosY + (BOX_SIZE * row) + GAP),
				(startPosX + (BOX_SIZE * col)) + BOX_SIZE,
				(startPosY + (BOX_SIZE * row)) + BOX_SIZE
			);
		}
	}
	glFlush();
}

void clearLine() {
	Line* line = pField.head;
	while (line->next != nullptr) {
		bool lineFound = true;
		for (int i = 0; i < nColumn; i++) {
			if (line->line[i] == 0 || line->line[i] == 2) {
				lineFound = false;
				break;
			}
		}
		cout << "Check this: " << lineFound << endl;
		if (lineFound) {
			Line* temp = line;
			line = line->next;
			pField.remove(temp->row);
			createPlayingField();
			score++;
			drawScore();
			continue;
		}
		line = line->next;
	}
}


//Rotates a piece without handling redrawing
void justRotate() {
	if (curPiece.piece == nullptr) return;
	if (curPiece.line == nullptr) return;


	int newIndices[4];
	int count = 0;


	for (int i = 0; i < 16; i++) {
		if (curPiece.piece->grid[i] == 1) {
			int rotatedIndex = ((4 - (i % 4) - 1) * 4) + (i / 4); 
			newIndices[count++] = rotatedIndex; 
			curPiece.piece->grid[i] = 0;       
		}
	}

	for (int i = 0; i < count; i++) {
		curPiece.piece->grid[newIndices[i]] = 1;
	}

	curPiece.piece->ori = (curPiece.piece->ori + 1) % 4;
}

bool simulateRotate(int* grid, int* newIndices, int& count) {
	count = 0;

	for (int i = 0; i < 16; i++) {
		if (grid[i] == 1) {
			int rotatedIndex = ((4 - (i % 4) - 1) * 4) + (i / 4);
			newIndices[count++] = rotatedIndex;
		}
	}

	return true;
}

bool canPieceRotate() {
	if (!curPiece.piece || !curPiece.line) return false;

	int tempGrid[16];
	int newIndices[4];
	int count = 0;
	memcpy(tempGrid, curPiece.piece->grid, sizeof(tempGrid));

	if (!simulateRotate(tempGrid, newIndices, count)) return false;

	for (int i = 0; i < 16; i++) tempGrid[i] = 0;
	for (int i = 0; i < count; i++) tempGrid[newIndices[i]] = 1;

	Line* line = curPiece.line;
	int destCol = curPiece.col;

	for (int i = 0; i < 16; i++) {
		if (i % 4 == 0 && line) line = line->next;

		if (tempGrid[i] == 0) continue;
		if (!line) return false;

		int col = destCol + (i % 4);
		if (col < 0 || col >= nColumn || (line->line[col] != 0 && line->line[col] != 2)) {
			return false;
		}
	}

	return true;
}

bool rotatePiece() {
	if (!canPieceRotate()) return false;
	Line* line = curPiece.line;
	for (int i = 0; i < 16; i++) {


		if (line->next != nullptr && i != 0 && i % 4 == 0)
			line = line->next;

		int col = curPiece.col + (i % 4);
		int row = curPiece.row + (i / 4);
		int colorIndex = line->line[col] % 4;
		glColor3f(colors[colorIndex]->red, colors[colorIndex]->green, colors[colorIndex]->blue);
		glRecti(
			(startPosX + (BOX_SIZE * col) + GAP),
			(startPosY + (BOX_SIZE * row) + GAP),
			(startPosX + (BOX_SIZE * col)) + BOX_SIZE,
			(startPosY + (BOX_SIZE * row)) + BOX_SIZE
		);
	}

	glFlush();
	justRotate();
	drawPiece();
	return true;
}

void newPiece() {

	static bool isSeeded = false;
	if (!isSeeded) {
		srand(static_cast<unsigned int>(std::time(nullptr)));
		isSeeded = true;
	}

	int randomIndex = rand() % 7;

	curPiece.piece = tetrominoes[randomIndex];
	curPiece.col = 4;
	curPiece.row = 0;
	curPiece.line = pField.head;
	drawPiece();
}

bool movePiece(int dir) {
	if (curPiece.piece == nullptr) return false;
	if (curPiece.line == nullptr) return false;

	if (!canPieceMove(dir) && dir == 0) {

		lockPiece();
		clearLine();
		cout << "Leaving clearLine" << endl;
		createPlayingField();
		newPiece();
	}

	if (!canPieceMove(dir)) return false;

	Line* line = curPiece.line;


	//Remove the piece from the playing field before redrawing it in the next postion
	for (int i = 0; i < 16; i++) {

		if (line->next == nullptr) break;
		if (i != 0 && i % 4 == 0)
			line = line->next;


		int col = curPiece.col + (i % 4);
		int row = curPiece.row + (i / 4);

		if (nColumn - col < 1 || col < 0) { cout << "Outside the playing field" << endl; continue; }
		int colorIndex = line->line[col] % 4;
		glColor3f(colors[colorIndex]->red, colors[colorIndex]->green, colors[colorIndex]->blue);
		glRecti(
			(startPosX + (BOX_SIZE * col) + GAP),
			(startPosY + (BOX_SIZE * row) + GAP),
			(startPosX + (BOX_SIZE * col)) + BOX_SIZE,
			(startPosY + (BOX_SIZE * row)) + BOX_SIZE
		);

	}
	glFlush();

	switch (dir) {
	case 0:
		curPiece.line = curPiece.line->next;
		curPiece.row++;
		break;
	case 1:
		curPiece.col += 1;
		break;
	case 2:
		curPiece.col -= 1;
		break;
	}
	drawPiece();
	return true;
}

void handleKeyboadInput(unsigned char key, int x, int y) {
	switch (key) {
	case '1': 
		if (frameId == 'o') { // Reset previous game state before stating a new game
			gameOver = false;
			score = 0;
			clearPlayingField();
		}
		frameId = 'g'; 
		glutPostRedisplay(); 
		break;
	case '2':
		frameId = 's';
		glutPostRedisplay();
		break;
	case '3': 
		frameId = 'h';
		glutPostRedisplay();
		break;
	case 'm': 
		frameId = 'm';
		glutPostRedisplay();
		break;
	case 's': case 'S': 
		if (frameId == 'g') movePiece(0);
		break;

	case 'w': case 'W': 
		if (frameId == 'g') rotatePiece();
		break;

	case 'a': case 'A': 
		if (frameId == 'g') movePiece(2);
		break;

	case 'd': case 'D': 
		if (frameId == 'g') movePiece(1);
		break;
	}
}


void handleSpecialKey(int key, int x, int y) {
	if (curPiece.piece == nullptr) return;
	if (curPiece.line == nullptr) return;

	switch (key) {
	case GLUT_KEY_DOWN:
		movePiece(0);
		break;

	case GLUT_KEY_UP:
		rotatePiece();
		break;

	case GLUT_KEY_LEFT:
		movePiece(2);
		break;

	case GLUT_KEY_RIGHT:
		movePiece(1);
		break;
	}
}

void game() {
	//Game Logic =============================================================================


	//Display =====================================================================================
	createPlayingField();
	newPiece();
	drawScore();

	glutTimerFunc(100, timerCallback, 0);
}

void myDisplay() {
	glClear(GL_COLOR_BUFFER_BIT);
	glColor3f(0.2, 0.2, 0.2); 

	
	switch (frameId) {
	case 'm': // Main menu
		displayMenu();
		break;
	case 'g': // Game
		game();
		break;
	case 's': // Scores
		displayScores();
		break;
	case 'h': // How to Play
		displayHowToPlay();
		break;
	case 'o':
		displayGameOver();
		break;
	default:
		displayMenu(); 
		break;
	}
	glFlush();
}


int main(int argc, char* argv[]) {
	glutInit(&argc, argv);
	glutInitWindowSize(640, 650);
	glutInitWindowPosition(10, 10);
	glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
	glutCreateWindow("Tetris Practice");
	glutDisplayFunc(myDisplay);
	glutKeyboardFunc(handleKeyboadInput);
	glutSpecialFunc(handleSpecialKey);
	myInit();
	gameInit();
	glutMainLoop();
}
