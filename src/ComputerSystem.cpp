#include "ComputerSystem.h"

// contructor
ComputerSystem::ComputerSystem(int numberOfPlanes) {
	numPlanes = numberOfPlanes;
	currPeriod = CS_PERIOD;
	initialize();
}

// destructor
ComputerSystem::~ComputerSystem() {
	shm_unlink("airspace");
	shm_unlink("display");
	for (std::string name : commNames) {
		shm_unlink(name.c_str());
	}
	pthread_mutex_destroy(&mutex);

	delete (timer);
}

// start computer system
void ComputerSystem::start() {
	time(&startTime);
	if (pthread_create(&computerSystemThread, &attr,
			&ComputerSystem::startComputerSystem,
			(void *)this) != EOK) {
		computerSystemThread = 0;
	}
}

// join computer system thread
int ComputerSystem::stop() {
	pthread_join(computerSystemThread, NULL);
	return 0;
}

// entry point for execution function
void *ComputerSystem::startComputerSystem(void *context) {
	((ComputerSystem *)context)->calculateTrajectories();

	return NULL;
}

int ComputerSystem::initialize() {
	// initialize thread members

	// set threads in detached state
	int rc = pthread_attr_init(&attr);
	if (rc) {
		printf("ERROR, RC from pthread_attr_init() is %d \n", rc);
	}

	rc = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	if (rc) {
		printf("ERROR; RC from pthread_attr_setdetachstate() is %d \n", rc);
	}

	// shared memory members

	// shared memory from ssr for the airspace
	shm_airspace = shm_open("airspace", O_RDONLY, 0666);
	if (shm_airspace == -1) {
		perror("in compsys shm_open() airspace");
		exit(1);
	}

	airspacePtr =
			mmap(0, SIZE_SHM_AIRSPACE, PROT_READ, MAP_SHARED, shm_airspace, 0);
	if (airspacePtr == MAP_FAILED) {
		perror("in compsys map() airspace");
		exit(1);
	}

	// open period shm
	shm_period = shm_open("period", O_RDWR, 0666);
	if (shm_period == -1) {
		perror("in shm_open() PSR: period");
		exit(1);
	}

	// map period shm
	periodPtr = mmap(0, SIZE_SHM_PERIOD, PROT_READ | PROT_WRITE, MAP_SHARED,
			shm_period, 0);
	if (periodPtr == MAP_FAILED) {
		perror("in map() PSR: period");
		exit(1);
	}

	// open shm display
	shm_display = shm_open("display", O_RDWR, 0666);
	if (shm_display == -1) {
		perror("in compsys shm_open() display");
		exit(1);
	}

	// map display shm
	displayPtr = mmap(0, SIZE_SHM_DISPLAY, PROT_READ | PROT_WRITE, MAP_SHARED,
			shm_display, 0);
	if (displayPtr == MAP_FAILED) {
		perror("in copmsys map() display");
		exit(1);
	}

	// open comm shm
	int shm_comm = shm_open("waiting_planes", O_RDONLY, 0666);
	if (shm_comm == -1) {
		perror("in compsys shm_open() comm");
		exit(1);
	}

	// map comm shm
	void *commPtr = mmap(0, SIZE_SHM_PSR, PROT_READ, MAP_SHARED, shm_comm, 0);
	if (commPtr == MAP_FAILED) {
		perror("in compsys map() comm");
		exit(1);
	}

	// generate pointers to plane shms
	std::string buffer = "";

	for (int i = 0; i < SIZE_SHM_PSR; i++) {
		char readChar = *((char *)commPtr + i);

		if (readChar == ',') {

			// open shm for current plane
			int shm_plane = shm_open(buffer.c_str(), O_RDWR, 0666);
			if (shm_plane == -1) {
				perror("in compsys shm_open() plane");

				exit(1);
			}

			// map memory for current plane
			void *ptr = mmap(0, SIZE_SHM_PLANES, PROT_READ | PROT_WRITE, MAP_SHARED,
					shm_plane, 0);
			if (ptr == MAP_FAILED) {
				perror("in compsys map() plane");
				exit(1);
			}

			commPtrs.push_back(ptr);
			commNames.push_back(buffer);

			buffer = "";
			continue;
		} else if (readChar == ';') {
			// open shm for current plane
			int shm_plane = shm_open(buffer.c_str(), O_RDWR, 0666);
			if (shm_plane == -1) {
				perror("in compsys shm_open() plane");

				exit(1);
			}

			// map memory for current plane
			void *ptr = mmap(0, SIZE_SHM_PLANES, PROT_READ | PROT_WRITE, MAP_SHARED,
					shm_plane, 0);
			if (ptr == MAP_FAILED) {
				perror("in compsys map() plane");
				exit(1);
			}

			commPtrs.push_back(ptr);
			commNames.push_back(buffer);

			break;
		}

		buffer += readChar;
	}

	return 0;
}

void *ComputerSystem::calculateTrajectories() {
	// create channel to communicate with timer
	int chid = ChannelCreate(0);
	if (chid == -1) {
		std::cout << "couldn't create channel\n";
	}

	Timer *newTimer = new Timer(chid);
	timer = newTimer;
	timer->setTimer(CS_PERIOD, CS_PERIOD);

	int rcvid;
	Message msg;
	// Logging commands
	std::ofstream out("command");

	bool done = false;
	while (1) {
		if (rcvid == 0) {

			pthread_mutex_lock(&mutex);

			// read airspace shm
			done = readAirspace();

			// prune predictions
			cleanPredictions();

			// compute airspace violations for all planes in the airspace
			computeViolations(&out);

			// send airspace info to display / prune airspace info
			writeToDisplay();

			// update the period based on the traffic
			updatePeriod(chid);

			// unlock mutex
			pthread_mutex_unlock(&mutex);
		}

		// termination check
		if (numPlanes <= 0 || done) {
			std::cout << "computer system done\n";
			time(&finishTime);
			double execTime = difftime(finishTime, startTime);
			std::cout << "computer system execution time: " << execTime << " seconds\n";
			sprintf((char *)displayPtr, "terminated");
			ChannelDestroy(chid);
			return 0;
		}

		rcvid = MsgReceive(chid, &msg, sizeof(msg), NULL);
	}

	out.close();

	ChannelDestroy(chid);

	return 0;
}

bool ComputerSystem::readAirspace() {
	std::string readBuffer = "";
	int j = 0;

	// variable buffer, these get overwritten as needed
	int id, arrTime, pos[3], vel[3];

	// find planes in airspace
	for (int i = 0; i < SIZE_SHM_AIRSPACE; i++) {
		char readChar = *((char *)airspacePtr + i);
		if (readChar == 't') {
			return true;
		}
		// end of airspace shm, line termination found
		if (readChar == ';') {
			// i=0 no planes found
			if (i == 0) {
				break;
			}

			// load last field in buffer
			vel[2] = atoi(readBuffer.c_str());

			// check if already in airspace, if yes update with current data
			bool inList = false;
			for (aircraft *craft : flyingPlanesInfo) {
				if (craft->id == id) {
					// if found, update with current info
					craft->pos[0] = pos[0];
					craft->pos[1] = pos[1];
					craft->pos[2] = pos[2];
					craft->vel[0] = vel[0];
					craft->vel[1] = vel[1];
					craft->vel[2] = vel[2];
					// if found, keep, if not will be removed
					craft->keep = true;
					// it is already in the list, do not add new
					inList = true;

					for (trajectoryPrediction *prediction : trajectoryPredictions) {
						if (prediction->id == craft->id) {

							// if end of prediction reached, break
							if (prediction->t >= (int)prediction->posX.size() ||
									prediction->t >= (int)prediction->posY.size() ||
									prediction->t >= (int)prediction->posZ.size()) {
								break;
							}

							// check posx, posy and poz if same
							if (prediction->posX.at(prediction->t) == craft->pos[0] &&
									prediction->posY.at(prediction->t) == craft->pos[1] &&
									prediction->posZ.at(prediction->t) == craft->pos[2]) {
								// set prediction index to next
								// update last entry in predictions
								auto it = prediction->posX.end();
								it -= 2;
								int lastX = *it + (currPeriod / 1000000) * craft->vel[0];
								it = prediction->posY.end();
								it -= 2;
								int lastY = *it + (currPeriod / 1000000) * craft->vel[1];
								it = prediction->posZ.end();
								it -= 2;
								int lastZ = *it + (currPeriod / 1000000) * craft->vel[2];

								// check if still within limits
								bool outOfBounds = false;
								if (lastX > SPACE_X_MAX || lastX < SPACE_X_MIN) {
									outOfBounds = true;
								}
								if (lastY > SPACE_Y_MAX || lastY < SPACE_Y_MIN) {
									outOfBounds = true;
								}
								if (lastZ > SPACE_Z_MAX || lastZ < SPACE_Z_MIN) {
									outOfBounds = true;
								}
								// if not, just increment
								if (outOfBounds) {
									prediction->keep = true;
									prediction->t++;
									break;
								}

								// add new last prediction
								prediction->posX.pop_back();
								prediction->posX.push_back(lastX);
								prediction->posX.push_back(-1);
								prediction->posY.pop_back();
								prediction->posY.push_back(lastY);
								prediction->posY.push_back(-1);
								prediction->posZ.pop_back();
								prediction->posZ.push_back(lastZ);
								prediction->posZ.push_back(-1);

								// keep for computation and increment index
								prediction->keep = true;
								prediction->t++;
							}
							// update the prediction
							else {
								prediction->posX.clear();
								prediction->posY.clear();
								prediction->posZ.clear();

								for (int i = 0; i < (180 / (currPeriod / 1000000)); i++) {
									int currX = craft->pos[0] +
											i * (currPeriod / 1000000) * craft->vel[0];
									int currY = craft->pos[1] +
											i * (currPeriod / 1000000) * craft->vel[1];
									int currZ = craft->pos[2] +
											i * (currPeriod / 1000000) * craft->vel[2];

									prediction->posX.push_back(currX);
									prediction->posY.push_back(currY);
									prediction->posZ.push_back(currZ);

									bool outOfBounds = false;
									if (currX > SPACE_X_MAX || currX < SPACE_X_MIN) {
										outOfBounds = true;
									}
									if (currY > SPACE_Y_MAX || currY < SPACE_Y_MIN) {
										outOfBounds = true;
									}
									if (currZ > SPACE_Z_MAX || currZ < SPACE_Z_MIN) {
										outOfBounds = true;
									}
									if (outOfBounds) {
										break;
									}
								}
								// set termination character
								prediction->posX.push_back(-1);
								prediction->posY.push_back(-1);
								prediction->posZ.push_back(-1);
								// set prediction index to next, keep for computation
								prediction->t = 1;
								prediction->keep = true;
							}
						}
					}
					break;
				}
			}

			// if plane was not already in the list, add it
			if (!inList) {
				// new pointer to struct, set members from read
				aircraft *currentAircraft = new aircraft();
				currentAircraft->id = id;
				currentAircraft->t_arrival = arrTime;
				currentAircraft->pos[0] = pos[0];
				currentAircraft->pos[1] = pos[1];
				currentAircraft->pos[2] = pos[2];
				currentAircraft->vel[0] = vel[0];
				currentAircraft->vel[1] = vel[1];
				currentAircraft->vel[2] = vel[2];
				currentAircraft->keep = true; // keep for first computation
				flyingPlanesInfo.push_back(
						currentAircraft); // add to struct pointer vector

				trajectoryPrediction *currentPrediction = new trajectoryPrediction();

				currentPrediction->id = currentAircraft->id;

				for (int i = 0; i < 180 / (currPeriod / 1000000); i++) {
					int currX = currentAircraft->pos[0] +
							i * (currPeriod / 1000000) * currentAircraft->vel[0];
					int currY = currentAircraft->pos[1] +
							i * (currPeriod / 1000000) * currentAircraft->vel[1];
					int currZ = currentAircraft->pos[2] +
							i * (currPeriod / 1000000) * currentAircraft->vel[2];

					currentPrediction->posX.push_back(currX);
					currentPrediction->posY.push_back(currY);
					currentPrediction->posZ.push_back(currZ);

					bool outOfBounds = false;
					if (currX > SPACE_X_MAX || currX < SPACE_X_MIN) {
						outOfBounds = true;
					}
					if (currY > SPACE_Y_MAX || currY < SPACE_Y_MIN) {
						outOfBounds = true;
					}
					if (currZ > SPACE_Z_MAX || currZ < SPACE_Z_MIN) {
						outOfBounds = true;
					}
					if (outOfBounds) {
						break;
					}
				}
				// set termination character
				currentPrediction->posX.push_back(-1);
				currentPrediction->posY.push_back(-1);
				currentPrediction->posZ.push_back(-1);
				// set prediction index to next, keep for first computation
				currentPrediction->t = 1;
				currentPrediction->keep = true;
				// add to list
				trajectoryPredictions.push_back(currentPrediction);

				break;
			}
			break;
		}
		// found plane in airspace (plane termination character)
		else if (readChar == '/') {
			// load last value in buffer
			vel[2] = atoi(readBuffer.c_str());

			// check if already in airspace, if yes update with current data
			bool inList = false;
			for (aircraft *craft : flyingPlanesInfo) {

				if (craft->id == id) {

					// if found, update with current info
					craft->pos[0] = pos[0];
					craft->pos[1] = pos[1];
					craft->pos[2] = pos[2];
					craft->vel[0] = vel[0];
					craft->vel[1] = vel[1];
					craft->vel[2] = vel[2];
					// if found, keep, if not will be removed
					craft->keep = true;
					// if already in list, do not add new
					inList = true;

					for (trajectoryPrediction *prediction : trajectoryPredictions) {
						if (prediction->id == craft->id) {

							// if end of prediction reached, break
							if (prediction->t >= (int)prediction->posX.size() ||
									prediction->t >= (int)prediction->posY.size() ||
									prediction->t >= (int)prediction->posZ.size()) {
								break;
							}

							// check posx, posy and poz if same
							if (prediction->posX.at(prediction->t) == craft->pos[0] &&
									prediction->posY.at(prediction->t) == craft->pos[1] &&
									prediction->posZ.at(prediction->t) == craft->pos[2]) {
								// set prediction index to next
								// update last entry in predictions
								auto it = prediction->posX.end();
								it -= 2;
								int lastX = *it + (currPeriod / 1000000) * craft->vel[0];
								it = prediction->posY.end();
								it -= 2;
								int lastY = *it + (currPeriod / 1000000) * craft->vel[1];
								it = prediction->posZ.end();
								it -= 2;
								int lastZ = *it + (currPeriod / 1000000) * craft->vel[2];

								// check if still within limits
								bool outOfBounds = false;
								if (lastX > SPACE_X_MAX || lastX < SPACE_X_MIN) {
									outOfBounds = true;
								}
								if (lastY > SPACE_Y_MAX || lastY < SPACE_Y_MIN) {
									outOfBounds = true;
								}
								if (lastZ > SPACE_Z_MAX || lastZ < SPACE_Z_MIN) {
									outOfBounds = true;
								}
								// if not, just increment
								if (outOfBounds) {
									prediction->keep = true;
									prediction->t++;
									break;
								}

								// add new last prediction
								prediction->posX.pop_back();
								prediction->posX.push_back(lastX);
								prediction->posX.push_back(-1);
								prediction->posY.pop_back();
								prediction->posY.push_back(lastY);
								prediction->posY.push_back(-1);
								prediction->posZ.pop_back();
								prediction->posZ.push_back(lastZ);
								prediction->posZ.push_back(-1);

								// keep for computation and increment index
								prediction->keep = true;
								prediction->t++;
							}
							// update the prediction
							else {
								prediction->posX.clear();
								prediction->posY.clear();
								prediction->posZ.clear();

								for (int i = 0; i < 180 / (currPeriod / 1000000); i++) {
									int currX = craft->pos[0] +
											i * (currPeriod / 1000000) * craft->vel[0];
									int currY = craft->pos[1] +
											i * (currPeriod / 1000000) * craft->vel[1];
									int currZ = craft->pos[2] +
											i * (currPeriod / 1000000) * craft->vel[2];

									prediction->posX.push_back(currX);
									prediction->posY.push_back(currY);
									prediction->posZ.push_back(currZ);

									bool outOfBounds = false;
									if (currX > SPACE_X_MAX || currX < SPACE_X_MIN) {
										outOfBounds = true;
									}
									if (currY > SPACE_Y_MAX || currY < SPACE_Y_MIN) {
										outOfBounds = true;
									}
									if (currZ > SPACE_Z_MAX || currZ < SPACE_Z_MIN) {
										outOfBounds = true;
									}
									if (outOfBounds) {
										break;
									}
								}
								// set termination character
								prediction->posX.push_back(-1);
								prediction->posY.push_back(-1);
								prediction->posZ.push_back(-1);
								// set prediction index to next, keep for computation
								prediction->t = 1;
								prediction->keep = true;
							}
						}
					}

					break;
				}
			}

			// if plane was not already in the list, add it
			if (!inList) {
				aircraft *currentAircraft = new aircraft();
				currentAircraft->id = id;
				currentAircraft->t_arrival = arrTime;
				currentAircraft->pos[0] = pos[0];
				currentAircraft->pos[1] = pos[1];
				currentAircraft->pos[2] = pos[2];
				currentAircraft->vel[0] = vel[0];
				currentAircraft->vel[1] = vel[1];
				currentAircraft->vel[2] = vel[2];
				currentAircraft->keep = true; // keep for first computation
				flyingPlanesInfo.push_back(
						currentAircraft); // add to struct pointer vector

				trajectoryPrediction *currentPrediction = new trajectoryPrediction();

				currentPrediction->id = currentAircraft->id;

				for (int i = 0; i < 180 / (currPeriod / 1000000); i++) {
					int currX = currentAircraft->pos[0] +
							i * (currPeriod / 1000000) * currentAircraft->vel[0];
					int currY = currentAircraft->pos[1] +
							i * (currPeriod / 1000000) * currentAircraft->vel[1];
					int currZ = currentAircraft->pos[2] +
							i * (currPeriod / 1000000) * currentAircraft->vel[2];

					currentPrediction->posX.push_back(currX);
					currentPrediction->posY.push_back(currY);
					currentPrediction->posZ.push_back(currZ);

					bool outOfBounds = false;
					if (currX > SPACE_X_MAX || currX < SPACE_X_MIN) {
						outOfBounds = true;
					}
					if (currY > SPACE_Y_MAX || currY < SPACE_Y_MIN) {
						outOfBounds = true;
					}
					if (currZ > SPACE_Z_MAX || currZ < SPACE_Z_MIN) {
						outOfBounds = true;
					}
					if (outOfBounds) {
						break;
					}
				}
				// set termination character
				currentPrediction->posX.push_back(-1);
				currentPrediction->posY.push_back(-1);
				currentPrediction->posZ.push_back(-1);
				// set prediction index to next, keep for first computation
				currentPrediction->t = 1;
				currentPrediction->keep = true;
				// add to list
				trajectoryPredictions.push_back(currentPrediction);
			}

			// reset buffer and index for next plane
			readBuffer = "";
			j = 0;
			continue;
		}
		// found next data field in current plane
		else if (readChar == ',') {
			switch (j) {
			// add whichever character the index j has arrived to
			case 0:
				id = atoi(readBuffer.c_str());
				readBuffer = "";
				j++;
				continue;
			case 1:
				arrTime = atoi(readBuffer.c_str());
				readBuffer = "";
				j++;
				continue;
			case 2:
				pos[0] = atoi(readBuffer.c_str());
				readBuffer = "";
				j++;
				continue;
			case 3:
				pos[1] = atoi(readBuffer.c_str());
				readBuffer = "";
				j++;
				continue;
			case 4:
				pos[2] = atoi(readBuffer.c_str());
				readBuffer = "";
				j++;
				continue;
			case 5:
				vel[0] = atoi(readBuffer.c_str());
				readBuffer = "";
				j++;
				continue;
			case 6:
				vel[1] = atoi(readBuffer.c_str());
				readBuffer = "";
				j++;
				continue;
			default:
				// just reset buffer
				readBuffer = "";
				break;
			}
		}
		readBuffer += readChar;
	}
	return false;
}

void ComputerSystem::cleanPredictions() {
	int j = 0;
	auto itpred = trajectoryPredictions.begin();
	while (itpred != trajectoryPredictions.end()) {
		bool temp = (*itpred)->keep;

		// check if plane was terminated
		if (!temp) {
			delete trajectoryPredictions.at(j);
			itpred = trajectoryPredictions.erase(itpred);
		} else {
			for (int i = (*itpred)->t - 1;
					i < (*itpred)->t + (180 / (currPeriod / 1000000)); i++) {
				int currX = (*itpred)->posX.at(i);
				int currY = (*itpred)->posY.at(i);
				int currZ = (*itpred)->posZ.at(i);

				if (currX == -1 || currY == -1 || currZ == -1) {
					break;
				}
			}

			(*itpred)->keep =
					false; // if found next time, this will become true again

			// only increment if no plane to remove
			j++;
			++itpred;
		}
	}
}

void ComputerSystem::computeViolations(std::ofstream *out) {
	auto itIndex = trajectoryPredictions.begin();
	while (itIndex != trajectoryPredictions.end()) {
		auto itNext = itIndex;
		++itNext;
		while (itNext != trajectoryPredictions.end()) {
			// compare predictions, starting at current
			int j = (*itNext)->t - 1;
			for (int i = (*itIndex)->t - 1;
					i < (*itIndex)->t - 1 + (180 / (currPeriod / 1000000)); i++) {
				int currX = (*itIndex)->posX.at(i);
				int currY = (*itIndex)->posY.at(i);
				int currZ = (*itIndex)->posZ.at(i);
				int compX = (*itNext)->posX.at(j);
				int compY = (*itNext)->posY.at(j);
				int compZ = (*itNext)->posZ.at(j);

				if (currX == -1 || currY == -1 || currZ == -1) {
					break;
				}
				if (compX == -1 || compY == -1 || compZ == -1) {
					break;
				}

				if ((abs(currX - compX) <= 3000 && abs(currY - compY) <= 3000) &&
						abs(currZ - compZ) <= 1000) {
					std::cout << "airspace violation detected between planes "
							<< (*itIndex)->id << " and " << (*itNext)->id
							<< " at time current + " << i * (currPeriod) / 1000000
							<< "\n";
					bool currComm = false;
					bool compComm = false;
					// find comm
					for (void *comm : commPtrs) {
						// comm shm id
						int commId = atoi((char *)comm);

						if (commId == (*itIndex)->id) {
							// find command index in plane shm
							int k = 0;
							char readChar;
							for (; k < SIZE_SHM_PLANES; k++) {
								readChar = *((char *)comm + k);
								if (readChar == ';') {
									break;
								}
							}
							// set pointer after plane termination
							k++;

							// command string (send one of the planes upwards)
							std::string command = "z,200;";

							// write command to plane
							sprintf((char *)comm + k, "%s", command.c_str());

							std::streambuf *coutbuf = std::cout.rdbuf(); // save old buf
							std::cout.rdbuf(out->rdbuf()); // redirect std::cout to command

							// this used to print to stdout but now we redirected it to
							// command
							std::cout << "Command: Plane " << (*itIndex)->id
									<< " increases altitude by 200 feet" << std::endl;

							std::cout.rdbuf(coutbuf); // reset to standard output again

							// current plane comm found
							currComm = true;
						}
						if (commId == (*itNext)->id) {
							// find command index in plane shm
							int k = 0;
							char readChar;
							for (; k < SIZE_SHM_PLANES; k++) {
								readChar = *((char *)comm + k);
								if (readChar == ';') {
									break;
								}
							}
							// set pointer after plane termination
							k++;

							// command string (send one of the planes downwards)
							std::string command = "z,-200;";

							// write command to plane
							sprintf((char *)comm + k, "%s", command.c_str());

							std::streambuf *coutbuf = std::cout.rdbuf(); // save old buf
							std::cout.rdbuf(out->rdbuf()); // redirect std::cout to command

							// this used to print to stdout but now we redirected it to
							// command
							std::cout << "Command: Plane " << (*itNext)->id
									<< " decreases altitude by 200 feet" << std::endl;

							std::cout.rdbuf(coutbuf); // reset to standard output again

							// compared plane comm found
							compComm = true;
						}
						if (currComm && compComm) {
							break;
						}
					}
					// set associate craft info request to true, display height
					currComm = false;
					compComm = false;
					for (aircraft *craft : flyingPlanesInfo) {
						if (craft->id == (*itIndex)->id) {
							craft->moreInfo = true;
							craft->commandCounter = NUM_PRINT;
							currComm = true;
						}
						if (craft->id == (*itNext)->id) {
							craft->moreInfo = true;
							craft->commandCounter = NUM_PRINT;
							compComm = true;
						}
						if (currComm && compComm) {
							break;
						}
					}
					break;
				}
				j++;
			}
			++itNext;
		}
		++itIndex;
	}
}

void ComputerSystem::writeToDisplay() {

	std::string displayBuffer = "";
	std::string currentPlaneBuffer = "";
	// print what was found, remove what is no longer in the airspace
	int i = 0;
	auto it = flyingPlanesInfo.begin();
	while (it != flyingPlanesInfo.end()) {
		bool temp = (*it)->keep; // check if plane was terminated

		if (!temp) {
			delete flyingPlanesInfo.at(i);
			it = flyingPlanesInfo.erase(it);
			numPlanes--;
		} else {
			// add plane to buffer for display

			// id,posx,posy,posz,info
			// ex: 1,15000,20000,5000,0
			displayBuffer = displayBuffer + std::to_string((*it)->id) + "," +
					std::to_string((*it)->pos[0]) + "," +
					std::to_string((*it)->pos[1]) + "," +
					std::to_string((*it)->pos[2]) + ",";

			// check if more info requested
			if ((*it)->moreInfo) {
				(*it)->commandCounter--;
				if ((*it)->commandCounter <= 0) {
					(*it)->moreInfo = false;
				}
				displayBuffer += "1/";
			} else {
				displayBuffer += "0/";
			}

			(*it)->keep = false; // if found next time, this will become true again

			// only increment if no plane to remove
			i++;
			++it;
		}
	}
	// termination character
	displayBuffer = displayBuffer + ";";

	sprintf((char *)displayPtr, "%s", displayBuffer.c_str());
}

void ComputerSystem::updatePeriod(int chid) {
	int newPeriod = 0;
	if (flyingPlanesInfo.size() <= 5) {
		// set period low
		newPeriod = 5000000;
	}
	if (flyingPlanesInfo.size() > 5 && flyingPlanesInfo.size() <= 20) {
		// set period medium
		newPeriod = 3000000;
	}
	if (flyingPlanesInfo.size() > 20 && flyingPlanesInfo.size() <= 50) {
		// set period high
		newPeriod = 2000000;
	}
	if (flyingPlanesInfo.size() > 50 && flyingPlanesInfo.size() <= 200) {
		// set period overdrive
		newPeriod = 1000000;
	}

	if (currPeriod != newPeriod) {
		currPeriod = newPeriod;
		std::string CSPeriod = std::to_string(currPeriod);
		sprintf((char *)periodPtr, CSPeriod.c_str());
		timer->setTimer(currPeriod, currPeriod);
	}
}
