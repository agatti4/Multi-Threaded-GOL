//StewartGatti_PrgAsst_05.c
//Name: Jeffrey Stewart and Anthony Gatti
//Description: This program simulates a multithreaded Conway's Game of Life
//             using input from the user. The user will input a filename, 
//             number of threads, row or col for row partitioning or col 
//             partitioning respectfully, a wrap or no wrap argument, a hide 
//             argument, a show argument, and a speed argument 
//             (only if show == "show").
//             In the input file the first line will have the # of rows, the
//             second line will have the # of columns, and the third line will
//             have the # of iterations to run the simulation. Every subsequent
//             line will be a pair of row, column positions to set to live in
//             the initial state.

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <pthread.h>
#include "pthread_barrier.h"

pthread_barrier_t threadBarrier; // Global barrier

int currentLiveCount = 0; //Global iteration live count

int totalLiveCount = 0; //Global total live count

pthread_mutex_t m;

typedef struct threadArg{
	int maxRow; // End row of board
	int maxCol; // End col of board 
	int begCol; // Beginning col of board for thread
	int begRow; // Beginning row of board for thread
	int endCol; // End col of board for thread
	int endRow; // End row of board for thread
	int iterations; // Number of iterations
	int showSpeed; // speed argument
	int wrap; // wrap argument
	int threadNum;
	int numThreads;
	int** currentBoard; // Pointer to current board
	int** nextBoard; // Pointer to next board
} threadArg;

void verifyCommandArg(char** argv, int argc,FILE* inFile);

int getSizeIterations(int* row, int*col, FILE* inFile);

void GetBoardStateFromFile(int** board, FILE* inFile);

int** initializeBoard(int row, int col);

void displayBoard(int** board, int row, int col);

int checkNeighbors(int** board, int curRow, int curCol, 
	int rowMax, int colMax, int wrap);

void calcNextIteration(int** currentBoard, int** nextBoard,int row,int col,
	int begRow, int begCol,int endCol,int endRow,int wrap);

int getShowType(char *argv[]);

void partition(int numThreads, int row, int col, char* partType, 
	threadArg* threadInput);

threadArg* initializeThreadInput(int numThreads, int iterations,int row,
	int col,char* wrap, int show, int** currentBoard, int** nextBoard);

void* runSlice(void* arguments);

void printFinal(int** currentBoard, int maxRow, int maxCol);

void freeMem(int** currentBoard, int** nextBoard,threadArg* threadInput,
		pthread_t* threadID,int row);

/**
 * Main. Calls the functions to run the simulation in the appropriate order
 * and calculates the run time of the program.
 * @param argc: amount of arguments in argv
 * @param argv: array of command line arguments
 * @return 0 
 * @throws exit(1) if file entered by user is invalid
 */
int main(int argc, char* argv[]){
	int** currentBoard;
	int** nextBoard; 
	int row,col,iterations,i,j;
	struct timeval start_time, end_time;
	float runTime;
	FILE* inFile;
	struct threadArg* threadInput;
	pthread_t* threadID;

	inFile = fopen(argv[1], "r");
	verifyCommandArg(argv,argc,inFile);

	iterations = getSizeIterations(&row,&col,inFile);
	currentBoard = initializeBoard(row,col);
	nextBoard = initializeBoard(row,col);
	//get starting board state
	GetBoardStateFromFile(currentBoard,inFile);
	fclose(inFile);

	threadInput = initializeThreadInput(atoi(argv[2]),iterations,
		row,col,argv[4],getShowType(argv),currentBoard,nextBoard);

	partition(atoi(argv[2]),row,col,argv[3],threadInput);

	pthread_barrier_init (&threadBarrier,NULL,atoi(argv[2]));
	pthread_mutex_init(&m, NULL);

	threadID = (pthread_t*) malloc(atoi(argv[2])* sizeof(pthread_t));

	gettimeofday(&start_time, 0); //get start time

	for(i=0;i<atoi(argv[2]);i++){
		pthread_create(&threadID[i],NULL,runSlice,&threadInput[i]);
	}

	for(i=0;i<atoi(argv[2]);i++){
		pthread_join(threadID[i],NULL);
	}
	gettimeofday(&end_time, 0); //get end time
	
	//calculate run time
	runTime = (end_time.tv_sec - start_time.tv_sec) + 
			  ((end_time.tv_usec - start_time.tv_usec))/1000000.0;

	printf("\nTotal time for %d iterations of %dx%d is %0.6f secs\n\n",
		iterations,row,col,runTime);
	freeMem(currentBoard, nextBoard,threadInput,threadID,row);

	return 0;
}

/**
 * Frees the allocated memory for each malloc
 * @param currentBoard: pointer to the grid for the current iteration
 * @param nextBoard: pointer to the grid for the next iteration
 * @param threadInput: The array of thread inputs
 * @param threadID: The array of thread ids
 * @param row: The number of rows in the grid
 * @return nothing
 */
void freeMem(int** currentBoard, int** nextBoard,threadArg* threadInput,
		pthread_t* threadID,int row){
	int i;

	//Free the memory allocated for the grids
	for(i = 0; i < row; i++){
		free(currentBoard[i]);
		free(nextBoard[i]);
	}
	free(currentBoard);
	free(nextBoard);
	free(threadInput);
	free(threadID);

}

/**
 * Verifies that the command line arguments entered by the user are valid and
 * that there are enough of them. Also verifies the input file exists.
 * @param argv: array containing the command line arguments input by the user
 * @param argc: The amount of command line arguments
 * @returns nothing
 * @throws exit(1) error if there are not enough arguments or if any are invalid
 */
void verifyCommandArg(char** argv, int argc,FILE* inFile){
	//verify amount of arguments is appropriate
	if(argc < 6){
		printf("%s", "Not enough command line arguments");
		exit(1);
	}
	if(strcmp(argv[5],"show") == 0){
		//verify amount of arguments is appropriate
		if(argc < 7){
			printf("%s", "Not enough command line arguments");
			exit(1);		
		}
		//verify speed argument is valid
		if(strcmp(argv[6],"slow") != 0){ 
			if(strcmp(argv[6],"med") != 0){
				if(strcmp(argv[6],"fast") != 0){
					printf("%s", "invalid speed argument");
					exit(1);			
				}
			} 

		}
	}
	//verify hide argument is valid
	else if(strcmp(argv[5],"hide") != 0){
		printf("%s", "invalid show argument");
		exit(1);	
	}

	//verify wrap argument is valid
	if(strcmp(argv[4],"wrap") != 0 && strcmp(argv[4],"nowrap") != 0){
		printf("%s", "invalid wrap argument");
		exit(1);
	}

	if(strcmp(argv[3],"row") != 0 && strcmp(argv[3],"col") != 0){
		printf("%s", "invalid partition argument");
		exit(1);
	}

	if(atoi(argv[2]) <= 0){
		printf("%s", "number of threads must be greater than 0");
		exit(1);
	}
	if(inFile == NULL){
		printf("%s", "Error opening file.");
		exit(1);
	}
}

/**
 * Gets the size of the grid and the number of iterations from the user
 * @param row: pointer to int row in main, contains # of rows in the grid
 * @param col: pointer to int col in main, contains # of columns in the grid
 * @param inFile: FILE ptr that has opened the file entered by the user
 * @returns: returns the number of iterations from file
 */
int getSizeIterations(int* row, int*col, FILE* inFile){
	int iterations;

	//get first 3 lines from file (row, col, iterations)
	if(fscanf(inFile, "%d", row) < 1){
		printf("%s", "error getting rows from file");
		exit(1);
	}
	if(fscanf(inFile, "%d", col) < 1){
		printf("%s", "error getting columns from file");
		exit(1);
	}
	if(fscanf(inFile, "%d", &iterations) < 1){
		printf("%s", "error getting iterations from file");
		exit(1);
	}

	return iterations;
}

/**
 * Gets the live cells in the initial state of the grid from the file and sets
 * those cells to alive on the board.
 * @param board: pointer to the 2d integer array that makes up the initial board
 * @param inFile: FILE ptr that has opened the file entered by the user
 * @returns: nothing
 */
void GetBoardStateFromFile(int** board, FILE* inFile){
	int row;
	int col;
	int scanResult;
	int success = 1;

	//get live cells from file and set them to alive in the initial board
	while(success == 1){
		scanResult = fscanf(inFile, "%d%d", &row,&col);
		if (scanResult != EOF){
			if(scanResult < 2){
				printf("%s", "error reading file");
				exit(1);
			}
			board[row][col] = 1;
			currentLiveCount++;
		}
		else{
			success = 0;
		}		
	}

}

/**
 * Dynamically allocates memory for a grid(2D array) of size row x col and sets
 * allinitial values to 0
 * @param row: the number of rows in the grid
 * @param col: the number of columns in the grid
 * @return: returns a pointer to the newly initialized 2D array
 */
int** initializeBoard(int row, int col){
	int i;
	int j;
	int** board;

	//allocate memory space for 2D int array of size[row,col]
	board = (int**) malloc(row * sizeof(int*));
	for(i = 0; i < row; i++){
		board[i] = (int*) malloc(col * sizeof(int));
	}
	//set all initial values in 2D array to 0 (dead)
	for(i = 0; i < row; i++){
		for(j = 0; j < row; j++){
			board[i][j] = 0;
		}
	}
	return board;
}

/**
 * Prints the partition information for a thread
 * @param threadNum: number of the thread being printed 
 * @param numThreads: The total number of threads
 * @param begRow: The start row for the thread
 * @param begCol: The start col for the thread
 * @param endRow: The end row for the thread
 * @param endCol: The end col for the thread
 * @param rowMax: The number of rows in the grid
 * @param colMax: The number of columns in the grid
 * @return nothing
 */
void printPartition(int threadNum,int numThreads ,int begRow, int endRow, 
		int begCol,int endCol,int rowMax,int colMax){

	int num,rowForm,colForm,threadNumForm;
	colForm = 0;
	rowForm = 0;
	threadNumForm = 0;
	num = rowMax+1;
   	while(num != 0) {
      	num = num / 10;
      	rowForm++;
   	}
   	num = colMax+1;
   	while(num != 0) {
      	num = num / 10;
      	colForm++;
   	}
   	num = numThreads-1;
   	while(num != 0) {
      	num = num / 10;
      	threadNumForm++;
   	}

	printf("Thread %*d: Rows: %*d:%*d (%*d) Cols: %*d:%*d (%*d)\n",
		threadNumForm,threadNum,rowForm,begRow,rowForm,endRow,rowForm,
		endRow-begRow+1,colForm,begCol,colForm,endCol,colForm,endCol-begCol+1);
	fflush(stdout);
}

/**
 * Prints the contents of a 2D integer array on screen
 * @param board: pointer to the 2D integer array to be printed on screen
 * @param row: the number of rows in the array
 * @param col: the number of columns in the array 
 * @return nothing
 */
void displayBoard(int** board, int row, int col){
	int i;
	int j;

	//print each cell of the board in order
	for(i = 0; i < row; i++){
		for(j = 0; j < col; j++){
			if(board[i][j] == 0){
				printf("-");
			}
			else{
				printf("@");
			}
		}
		//end of row, new line
		printf("\n");
	}
}

/**
 * Calculates and returns the number of living neighbors that the cell at
 * [curRow,curCol] in the grid has. Used for checking neighbors when
 * the grid is both wrapped and not wrapped
 * @param board: pointer to the current grid 
 * @param curRow: row of the cell whose neighbors are being checked
 * @param curCol: col of the cell whose neighbors are being checked
 * @param rowMax: number of rows in the grid
 * @param colMax: number of columns in the grid
 * @param wrap: string containing "wrap" or "nowrap"
 * @return: returns the number of living neighbors the cell has
 */
int checkNeighbors(int** board, int curRow, int curCol, int rowMax,
	int colMax, int wrap){
	int i,j; //loop variables
	//wrapRow and wrapCol are used for calculations in place of i and j
	//in the wrap section so i and j remain unchanged and iterate properly.
	int wrapRow,wrapCol;
	int liveNeighbors = 0;

	//Calculate neighbors if nowrap
	if (wrap == 0){
		for(i = curRow-1; i < curRow+2; i++){
			if(i >= 0 && i < rowMax){
				for(j = curCol-1;j < curCol+2;j++){
					if(j >= 0 && j < colMax){
						if(board[i][j] == 1 && (i != curRow || j != curCol)){
							liveNeighbors++;
						}
					}
				}
			}
		}
	}
	//calculate neighbors if wrap
	else{
		for(i = curRow-1; i < curRow+2; i++){
			wrapRow = i;
			//if neighbor is out of bounds row-wise, wrap row
			if(wrapRow < 0){
				wrapRow = rowMax -1;
			}
			else if(wrapRow >= rowMax){
				wrapRow = 0;
			}
			for(j = curCol-1;j < curCol+2;j++){
				wrapCol = j;
				//if neighbor is out of bounds col-wise, wrap col
				if(wrapCol < 0){
					wrapCol = colMax -1;
				}
				else if(wrapCol >= colMax){
					wrapCol = 0;
				}
				//if neighbor is alive and is not the cell itself		
				if(board[wrapRow][wrapCol] == 1 && (i!=curRow || j!=curCol)){
					liveNeighbors++;
				}
			}
		}
	}
	return liveNeighbors;	
}

/**
 * Calculates the status of each cell for the next iteration and assigns the
 * corresponding values in the board for the next iteration.
 * @param currentBoard: pointer to the grid for the current iteration
 * @param nextBoard: pointer to the grid for the next iteration
 * @param row: The number of rows in the grid
 * @param col: The number of columns in the grid
 * @param begRow: The start row for the thread
 * @param begCol: The start col for the thread
 * @param endRow: The end row for the thread
 * @param endCol: The end col for the thread
 * @param wrap: string containing 0 for nowrap 1 for wrap
 * @return nothing
 */
void calcNextIteration(int** currentBoard, int** nextBoard,int row,int col,
		int begRow, int begCol,int endCol,int endRow,int wrap){
	int i;
	int j;
	int curNeighbors;

	for(i = begRow; i < endRow; i++){
		for(j = begCol; j < endCol; j++){
			curNeighbors = checkNeighbors(currentBoard,i,j,row,col,wrap);
			//if cell is dead
			if(currentBoard[i][j] == 0){
				if(curNeighbors == 3){
					nextBoard[i][j] = 1;
					pthread_mutex_lock(&m);
					currentLiveCount++;
					totalLiveCount++;
					// release lock
					pthread_mutex_unlock(&m);
				}
				else{
					nextBoard[i][j] = 0;
				}
			}
			//if cell is alive
			else{
				if(curNeighbors == 2 || curNeighbors == 3){
					nextBoard[i][j] = 1;
					pthread_mutex_lock(&m);
					currentLiveCount++;
					totalLiveCount++;
					// release lock
					pthread_mutex_unlock(&m);
					
				}
				else{
					nextBoard[i][j] = 0;
				}
			}
		}
	}
}

/**
* Retrieves the show type for the game of life
* @param argv an array of input arguments to the program
* @return int for which type of show
* @throws no errors
*/
int getShowType(char *argv[]) {
    int show; // Variable for if show or hide and how fast if show

    // Determines what was inputted
    if (strcmp(argv[5], "show") == 0) {
        if (strcmp(argv[6], "slow") == 0) {
            show = 333333;
        } else if (strcmp(argv[6], "med") == 0) {
            show = 100000;
        } else if (strcmp(argv[6], "fast") == 0) {
            show = 33333;
        }
    } else if (strcmp(argv[5], "hide") == 0) {
        show = 0;
    }
    return show;
}

/**
 * Partitions the boards into sections based on row or col partitioning.
 * This is based on the number of threads so the number of rows are columns
 * are spread as evenly as they can be across the threads.
 * @param numThreads: The number of threads created
 * @param row: The number of rows in the grid
 * @param col: The number of columns in the grid
 * @param partType: The type of partition, row or col
 * @param threadInput: The array of thread inputs
 * @return nothing
 */
void partition(int numThreads,int row,int col,char* partType,
	threadArg* threadInput){
	int i;
	int numGetMaxPart;
	int maxPart;
	int begRowCol;
	int endRowCol;

	if(strcmp(partType,"row") == 0){
		numGetMaxPart = row % numThreads;
		maxPart = row / numThreads;
	}
	else{
		numGetMaxPart = col % numThreads;
		maxPart = col / numThreads;
	}

	if(numGetMaxPart != 0){
		maxPart++;
	}
	else{
		numGetMaxPart = numThreads;
	}

	for(i = 0; i < numThreads;i++){
		if(i < numGetMaxPart){
			begRowCol = maxPart*i;
			endRowCol = maxPart*(i+1)-1;
		}
		else{
			begRowCol = maxPart*numGetMaxPart + (maxPart-1)*(i-numGetMaxPart);
			endRowCol = maxPart*numGetMaxPart+(maxPart-1)*(i+1-numGetMaxPart)-1;
		}
		if(strcmp(partType,"row") == 0){
			threadInput[i].begRow = begRowCol;
			threadInput[i].endRow = endRowCol;
			threadInput[i].begCol = 0;
			threadInput[i].endCol = col-1;
		}
		else{
			threadInput[i].begCol = begRowCol;
			threadInput[i].endCol = endRowCol;
			threadInput[i].begRow = 0;
			threadInput[i].endRow = row-1;
		}
	}
}

/**
 * Passes values into the threadArg array so each thread gets the info it 
 * needs to run the specific slice of the game given to them.
 * @param numThreads: The number of threads created 
 * @param iterations: Number of iterations to run game specified in input file. 
 * @param row: The number of rows in the grid
 * @param col: The number of columns in the grid
 * @param wrap: string containing "wrap" or "nowrap"
 * @param show: Type of show containing hide, slow, med, fast as int speeds
 * @param currentBoard: pointer to the grid for the current iteration
 * @param nextBoard: pointer to the grid for the next iteration
 * @return an array of threadArg inputs
 */
threadArg* initializeThreadInput(int numThreads, int iterations,int row,
	int col,char* wrap, int show, int** currentBoard, int** nextBoard){
	int i = 0;
	int wrapInt = 0;
	threadArg* threadInput;

	if(strcmp(wrap,"wrap") == 0){
		wrapInt = 1;
	}
	threadInput = (threadArg*) malloc(numThreads*sizeof(threadArg));
	for(i = 0; i < numThreads; i++){
		threadInput[i].iterations = iterations;
		threadInput[i].maxRow = row;
		threadInput[i].maxCol = col;
		threadInput[i].wrap = wrapInt;
		threadInput[i].showSpeed = show;
		threadInput[i].currentBoard = currentBoard;
		threadInput[i].nextBoard = nextBoard;
		threadInput[i].threadNum = i;
		threadInput[i].numThreads = numThreads;
	}

	return threadInput;
}

/**
 * Driver function for the simulation. Calls the function to calculate the
 * next iteration the appropriate amount of times and calls the function to
 * display the board for each iteration (if show). Sleeps the origin thread
 * each iteration for the appropriate amount of time corresponding to the 
 * speed entered by the user (if show). Each thread is given a slice.
 * @param arguments: pointer to a struct of arguments for the thread
 * @return NULL
 */
void* runSlice(void* arguments){
	threadArg localArg = ((threadArg*) arguments)[0];
	int i;
	int** swapTemp;
	
	if(localArg.begRow == 0 && localArg.begCol == 0){
		system("clear");
	}
	for(i = 0; i < localArg.iterations;i++){
		//display each iteration if show
		if(localArg.showSpeed != 0){
			if(localArg.begRow == 0 && localArg.begCol == 0){
				displayBoard(localArg.currentBoard,
				localArg.maxRow,localArg.maxCol);
				//print number of live cells
				printf("\nThere are %d live cells in this board\n", 
				currentLiveCount);
				fflush(stdout);
				usleep(localArg.showSpeed);
				system("clear");
			}
		}
		if(localArg.begRow == 0 && localArg.begCol == 0){
			currentLiveCount = 0;
		}
		pthread_barrier_wait(&threadBarrier);
		calcNextIteration(localArg.currentBoard,localArg.nextBoard,
			localArg.maxRow,localArg.maxCol,localArg.begRow,localArg.begCol,
			localArg.endCol+1,localArg.endRow+1,localArg.wrap);
		//swap boards so the previous "next iteration" is the current board
		swapTemp = localArg.currentBoard;
		localArg.currentBoard = localArg.nextBoard;
		localArg.nextBoard = swapTemp;
		pthread_barrier_wait(&threadBarrier);
	}
	//display final grid
	if(localArg.begRow == 0 && localArg.begCol == 0){
		printFinal(localArg.currentBoard,localArg.maxRow,localArg.maxCol);
	}
	pthread_barrier_wait(&threadBarrier);
	// printf("Thread %d: ",localArg.threadNum);
	printPartition(localArg.threadNum,localArg.numThreads,localArg.begRow,
		localArg.endRow,localArg.begCol,localArg.endCol,localArg.maxRow,
		localArg.maxCol);

	return NULL;
}

/**
 * Prints the final board and related live cell count information
 * @param currentBoard: pointer to the grid for the current iteration
 * @param maxRow: The number of rows in the grid
 * @param maxCol: The number of columns in the grid
 * @return nothing
 */
void printFinal(int** currentBoard, int maxRow, int maxCol){
	displayBoard(currentBoard,maxRow,maxCol);
	//print number of live cells
	printf("\nThere are %d live cells in this board.\n", currentLiveCount);
	// Print total live cells
	printf("There were a total of %d live cells during the simulation.\n\n",
	totalLiveCount);
	fflush(stdout);
}
