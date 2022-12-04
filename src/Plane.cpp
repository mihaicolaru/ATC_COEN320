#include "Plane.h"

// constructor
Plane::Plane(int _ID, int _arrivalTime, int _position[3], int _speed[3]) {
  // initialize members
  arrivalTime = _arrivalTime;
  ID = _ID;
  for (int i = 0; i < 3; i++) {
    position[i] = _position[i];
    speed[i] = _speed[i];
  }

  commandCounter = 0;
  commandInProgress = false;

  // initialize thread members
  initialize();
}

// destructor
Plane::~Plane() {
  shm_unlink(fileName.c_str());
  pthread_mutex_destroy(&mutex);
}

// call static function to start thread
int Plane::start() {
  if (pthread_create(&planeThread, &attr, &Plane::startPlane, (void *)this) !=
      EOK) {
    planeThread = 0;
  }

  return 0;
}

// join execution thread
bool Plane::stop() {
  pthread_join(planeThread, NULL);
  return 0;
}

// entry point for execution thread
void *Plane::startPlane(void *context) {
  // set priority
  ((Plane *)context)->flyPlane();
  return 0;
}

// initialize thread and shm members
int Plane::initialize() {
  // set thread in detached state
  int rc = pthread_attr_init(&attr);
  if (rc) {
    printf("ERROR, RC from pthread_attr_init() is %d \n", rc);
  }

  rc = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
  if (rc) {
    printf("ERROR; RC from pthread_attr_setdetachstate() is %d \n", rc);
  }

  // instantiate filename
  fileName = "plane_" + std::to_string(ID);

  // open shm object
  shm_fd = shm_open(fileName.c_str(), O_CREAT | O_RDWR, 0666);
  if (shm_fd == -1) {
    perror("in shm_open() plane");
    exit(1);
  }

  // set the size of shm
  ftruncate(shm_fd, SIZE_SHM_PLANES);

  // map shm
  ptr = mmap(0, SIZE_SHM_PLANES, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
  if (ptr == MAP_FAILED) {
    printf("map failed\n");
    return -1;
  }

  // update string of plane data
  updateString();

  // initial write + space for comm system
  sprintf((char *)ptr, "%s0;", planeString.c_str());

  return 0;
}

// update position every second from position and speed
void *Plane::flyPlane(void) {
  // create channel to link timer
  int chid = ChannelCreate(0);
  if (chid == -1) {
    std::cout << "couldn't create channel!\n";
  }

  // create timer and set offset and period
  Timer timer(chid);
  timer.setTimer(arrivalTime * 1000000, PLANE_PERIOD);

  // buffers for message from timer
  int rcvid;
  Message msg;

  bool start = true;
  while (1) {
    if (start) {
      // first cycle, wait for arrival time
      start = false;
    } else {
      if (rcvid == 0) {
        pthread_mutex_lock(&mutex);

        // check comm system for potential command
        answerComm();

        // update position based on speed
        updatePosition();

        // check for airspace limits, write to shm
        if (checkLimits() == 0) {
          std::cout << getFD() << " terminated\n";
          ChannelDestroy(chid);
          return 0;
        }

        pthread_mutex_unlock(&mutex);
      }
    }
    // wait until next timer pulse
    rcvid = MsgReceive(chid, &msg, sizeof(msg), NULL);
  }

  ChannelDestroy(chid);

  return 0;
}

// check comm system for potential commands
void Plane::answerComm() {
  // check if executing command
  if (commandInProgress) {
    // decrement counter
    commandCounter--;
    if (commandCounter <= 0) {
      commandInProgress = false;
    }
    return;
  }
  speed[2] = 0;

  // find end of plane info
  int i = 0;
  char readChar;
  for (; i < SIZE_SHM_PLANES; i++) {
    readChar = *((char *)ptr + i);
    if (readChar == ';') {
      break; // found end
    }
  }

  // check for command presence
  if (*((char *)ptr + i + 1) == ';' || *((char *)ptr + i + 1) == '0') {
    return;
  }

  // set index to next
  i++;
  int startIndex = i;
  readChar = *((char *)ptr + i);
  std::string buffer = "";
  while (readChar != ';') {
    buffer += readChar;
    readChar = *((char *)ptr + ++i);
  }
  buffer += ';';

  // parse command
  std::string parseBuf = "";
  int currParam;
  for (int j = 0; j <= (int)buffer.size(); j++) {
    char currChar = buffer[j];

    switch (currChar) {
    case ';':
      // reached end of command, apply command
      speed[currParam] = std::stoi(parseBuf);
      break;
    case '/':
      // read end of one velocity command, apply command, clear buffer
      speed[currParam] = std::stoi(parseBuf);
      parseBuf = "";
      continue;
    case 'x':
      // command to x velocity
      currParam = 0;
      continue;
    case 'y':
      // command to y velocity
      currParam = 1;
      continue;
    case 'z':
      // command to z velocity
      currParam = 2;
      continue;
    case ',':
      // spacer, clear buffer
      parseBuf = "";
      continue;
    default:
      parseBuf += currChar;
      continue;
    }
  }

  for (int k = 0; k < 500; k += abs(speed[2])) {
    commandCounter++;
  }

  commandInProgress = true;

  // remove command
  sprintf((char *)ptr + startIndex, "0;");
}

// update position based on speed
void Plane::updatePosition() {
  for (int i = 0; i < 3; i++) {
    position[i] = position[i] + speed[i];
  }
  // save modifications to string
  updateString();
}

// stringify plane data members
void Plane::updateString() {
  std::string s = ",";
  planeString = std::to_string(ID) + s + std::to_string(arrivalTime) + s +
                std::to_string(position[0]) + s + std::to_string(position[1]) +
                s + std::to_string(position[2]) + s + std::to_string(speed[0]) +
                s + std::to_string(speed[1]) + s + std::to_string(speed[2]) +
                ";";
}

// check airspace limits for operation termination
int Plane::checkLimits() {
  if (position[0] < SPACE_X_MIN || position[0] > SPACE_X_MAX) {
    planeString = "terminated";
    sprintf((char *)ptr, "%s0;", planeString.c_str());
    return 0;
  }
  if (position[1] < SPACE_Y_MIN || position[1] > SPACE_Y_MAX) {
    planeString = "terminated";
    sprintf((char *)ptr, "%s0;", planeString.c_str());
    return 0;
  }
  if (position[2] < SPACE_Z_MIN || position[2] > SPACE_Z_MAX) {
    planeString = "terminated";
    sprintf((char *)ptr, "%s0;", planeString.c_str());
    return 0;
  }

  // write plane to shared memory
  sprintf((char *)ptr, "%s0;", planeString.c_str());
  return 1;
}

void Plane::Print() { std::cout << planeString << "\n"; }
