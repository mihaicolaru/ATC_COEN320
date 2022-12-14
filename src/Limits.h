/*
 * Limits.h
 *
 *  Created on: Nov. 6, 2022
 *      Author: Mihai
 */

#ifndef LIMITS_H_
#define LIMITS_H_

// ATC Specifications
#define SPACE_X_MIN 0
#define SPACE_X_MAX 100000
#define SPACE_Y_MIN 0
#define SPACE_Y_MAX 100000
#define SPACE_Z_MIN 0
#define SPACE_Z_MAX 25000
#define SPACE_ELEVATION 15000

// Airspace Specifications
#define SPACE_X_MIN 0
#define SPACE_X_MAX 100000
#define SPACE_Y_MIN 0
#define SPACE_Y_MAX 100000
#define SPACE_Z_MIN 0
#define SPACE_Z_MAX 25000
#define SPACE_ELEVATION 15000

// PSR Specifications
#define SIZE_SHM_PSR 4096
#define PSR_PERIOD 2000000

// SSR Specifications
#define SIZE_SHM_AIRSPACE 8192
#define SIZE_SHM_SSR 4096
#define SIZE_SHM_PSR 4096
#define SSR_PERIOD 2000000

// Display Specifications
#define SCALER 3000
#define MARGIN 100000
#define PERIOD_D 5000000 // 5sec period
#define SIZE_SHM_DISPLAY 8192

// Computer System specifications
#define CS_PERIOD 2000000
#define SIZE_SHM_PERIOD 64
#define NUM_PRINT 3

// Plane Specifications
#define OFFSET 1000000
#define PLANE_PERIOD 1000000
#define SPACE_X_MIN 0
#define SPACE_X_MAX 100000
#define SPACE_Y_MIN 0
#define SPACE_Y_MAX 100000
#define SPACE_Z_MIN 0
#define SPACE_Z_MAX 25000
#define SPACE_ELEVATION 15000
#define SIZE_SHM_PLANES 128

// Timer Specifications
#define ONE_THOUSAND 1000
#define ONE_MILLION 1000000

// Message send definitions

// Messages
#define MT_WAIT_DATA 2 // Message from client
#define MT_SEND_DATA 3 // Message from client

// Pulses
#define CODE_TIMER 1 // Pulse from timer

// Message reply definitions
#define MT_OK 0       // Message to client
#define MT_TIMEDOUT 1 // Message to client

#endif /* LIMITS_H_ */
