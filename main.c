#include "mongoose.h"
#include <wiringPi.h>
#include <stdio.h>
#include <string.h>
#include "ncurses.h"
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>

// Pins
const int PI_OUT = 11;
const int FPGA_CLK = 0;
#define DW 10
const int D[DW] = {1, 2, 3, 3, 5, 6, 7, 8, 9, 10};

// Field memory
const int SIZE = 10;
bool* recvField = NULL;
bool* swapField = NULL;
bool* currField = NULL;
bool newField = 0;

// Mutices
pthread_mutex_t swapMutex;

// States of the game of life
#define STATE_RECV 			1 	// FPGA is sending a field
#define STATE_RUNNING 		2	// Waiting for the FPGA to send a field
#define STATE_PAUSED 		3	// FPGA is paused
#define STATE_STARTING 		4 	// Sending start signal to FPGA
#define STATE_SENDING 		5	// Pi is sending... Data?
#define STATE_PAUSING 		6	// Pi is sending one pause bit
#define STATE_RESETTING		7	// Pi is sending resetting sequence
int state = STATE_RUNNING;		// Variable holding the state
int stateCtr = 0;				// Variable to keep track how many signals are sent in the state

// Pausing variables
bool pauseGOL = 0;

// Resetting variables
bool resetGOL = 0;

// Starting variables
bool startGOL = 1;

static int event_handler(struct mg_connection *conn, enum mg_event ev) {
	if (ev == MG_AUTH) {
		return MG_TRUE;   // Authorize all requests
	} else if (ev == MG_REQUEST && !strcmp(conn->uri, "/gol")) {
		if (newField) {
			pthread_mutex_lock(&swapMutex);
			
			bool* tempSwapField = currField;
			currField = swapField;
			swapField = tempSwapField;
			newField = 0;

			pthread_mutex_unlock(&swapMutex);
		}

		for (int i = 0; i < SIZE * SIZE; i += DW) {
			mg_printf_data(conn, "%d%d%d%d%d%d%d%d%d%d",
					currField[i + 0],
					currField[i + 1],
					currField[i + 2],
					currField[i + 3],
					currField[i + 4],
					currField[i + 5],
					currField[i + 6],
					currField[i + 7],
					currField[i + 8],
					currField[i + 9]);
		}

		return MG_TRUE;
	} else if (ev == MG_REQUEST && !strcmp(conn->uri, "/pause")) {
		pauseGOL = 1;

		return MG_TRUE;
	} else if (ev == MG_REQUEST && !strcmp(conn->uri, "/reset")) {
		resetGOL = 1;

		return MG_TRUE;
	} else if (ev == MG_REQUEST && !strcmp(conn->uri, "/start")) {
		startGOL = 1;

		return MG_TRUE;
	} else {
		return MG_FALSE;  // Rest of the events are not processed
	}
}

void printField(bool* field) {
	for (int y = 0; y < SIZE; ++y) {
		for (int x = 0; x < SIZE; ++x) {
			printf("%d", field[y * SIZE + x]);
		}
		printf("\n");
	}
}

void risingFPGA_CLK() {
	switch (state) {
	case STATE_RECV:
		// Read pins D[0..DW] and save them in array
		for (int i = 0; i < DW; ++i) {
			recvField[stateCtr + i] = digitalRead(D[i]) == HIGH ? 1 : 0;
		}
		stateCtr += DW;
		// If last row
		if (stateCtr == SIZE * SIZE) {
			// Swap curr, swap
			pthread_mutex_lock(&swapMutex);
			bool* tempSwapField = swapField;
			swapField = recvField;
			recvField = tempSwapField;
			// newField to true
			newField = 1;
			pthread_mutex_unlock(&swapMutex);
			// Set state to STATE_RUNNING
			state = STATE_RUNNING;
			// Reset stateCtr
			stateCtr = 0;
			// TODO: Temporary
			printField(swapField);
		} else {
			// CARRY ON huehuehuehue
			// As in wait for next delivery
		}
		break;
	case STATE_RUNNING:
		// Set pin to 0
		digitalWrite(PI_OUT, LOW);
		// Check if FPGA wants to send
		if (digitalRead(D[0]) == HIGH) {
			state = STATE_RECV;
			stateCtr = 0;
		} else {
			if (pauseGOL) {
				pauseGOL = 0;
				state = STATE_PAUSING;
			}
		}
		break;
	case STATE_STARTING:
		switch (stateCtr) {
		case 0:
			// Set pin to 1
			digitalWrite(PI_OUT, HIGH);
			// Increment
			++stateCtr;
			break;
		case 1:
			// Set pin to 1
			digitalWrite(PI_OUT, HIGH);
			// Increment
			++stateCtr;
			break;
		case 2:
			// Set pin to 1
			digitalWrite(PI_OUT, HIGH);
			// stateCtr to 0 (which means done)
			stateCtr = 0;
			// Switch to waiting for field state
			state = STATE_RUNNING;
			break;
		}
		break;
	case STATE_PAUSING:
		switch (stateCtr) {
		case 0:
			// Set message pin to 1
			digitalWrite(PI_OUT, HIGH);
			// Increment stateCtr
			++stateCtr;
			break;
		case 1:
			digitalWrite(PI_OUT, LOW);
			stateCtr = 0;
			state = STATE_PAUSED;
			break;
		}
		break;
	case STATE_PAUSED:
		if (resetGOL) {
			state = STATE_RESETTING;
			stateCtr = 0;
		} else if (startGOL) {
			state = STATE_STARTING;
			stateCtr = 0;
		}
		break;
	case STATE_RESETTING:
		switch (stateCtr) {
		case 0:
			digitalWrite(PI_OUT, HIGH);
			++stateCtr;
			break;
		case 1:
			digitalWrite(PI_OUT, LOW);
			++stateCtr;
			break;
		case 2:
			digitalWrite(PI_OUT, HIGH);
			++stateCtr;
			break;
		case 3:
			digitalWrite(PI_OUT, LOW);
			resetGOL = 0;
			state = STATE_PAUSED;
			break;
		}
	case STATE_SENDING:
		stateCtr = 1/0; 	// This block is not allowed to be executed yet
		break;				// Grammar is important
	}
}

int main(void) {
	currField = (bool *) malloc(SIZE * SIZE * sizeof(bool));
	swapField = (bool *) malloc(SIZE * SIZE * sizeof(bool));
	recvField = (bool *) malloc(SIZE * SIZE * sizeof(bool));

	printField(currField);
	printField(swapField);
	printField(recvField);

	pthread_mutex_init(&swapMutex, NULL);

	// printf("Starting server at 130.89.160.100:8080\n");

	// struct mg_server *server = mg_create_server(NULL, event_handler);
	// mg_set_option(server, "document_root", ".");
	// mg_set_option(server, "listening_port", "8080");

	// Initialize wiringPi
	wiringPiSetup();

	// Set the message pin from pi to fpga to out
	pinMode(PI_OUT, OUTPUT);
	digitalWrite(PI_OUT, LOW);

	// Set fpga clock pin to input and register rising edge function
	pinMode(FPGA_CLK, INPUT);
	wiringPiISR(FPGA_CLK, INT_EDGE_RISING, risingFPGA_CLK);

	// Set the data pins to input and register the rising edge function for D[0]
	for (int i = 0; i < DW; ++i) {
		pinMode(D[i], INPUT);
	}

	for (;;) {
		// mg_poll_server(server, 1000);  // Infinite loop, Ctrl-C to stop
	}

	// mg_destroy_server(&server);
	
	pthread_mutex_destroy(&swapMutex);

	free(currField);
	free(recvField);
	free(swapField);

	return 0;
}
