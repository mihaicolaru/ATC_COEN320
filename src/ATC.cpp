#include "ATC.h"

ATC::ATC() {
  initialize();

  start();
}

ATC::~ATC() {
  // release all shared memory pointers
  shm_unlink("airspace");
  shm_unlink("waiting_planes");
  shm_unlink("flying_planes");
  shm_unlink("period");
  shm_unlink("display");

  for (Plane *plane : planes) {
    delete plane;
  }
  delete psr;
  delete ssr;
  delete display;
  delete computerSystem;
}

int ATC::start() {
  //		std::cout << "atc start called\n";

  // start threaded objects
  psr->start();
  ssr->start();
  display->start();
  computerSystem->start();
  for (Plane *plane : planes) {
    plane->start();
  }

  // ============ execution time ============

  // join threaded objects
  for (Plane *plane : planes) {
    plane->stop();
  }

  psr->stop();
  ssr->stop();
  display->stop();
  computerSystem->stop();

  return 0; // set to error code if any
}

// read input file and
int ATC::readInput() {
  //		std::cout << "atc readinput\n";
  // open input.txt
  std::string filename = "./input.txt";
  std::ifstream input_file_stream;

  input_file_stream.open(filename);

  if (!input_file_stream) {
    std::cout << "Can't find file input.txt" << std::endl;
    return 1;
  }

  int ID, arrivalTime, arrivalCordX, arrivalCordY, arrivalCordZ, arrivalSpeedX,
      arrivalSpeedY, arrivalSpeedZ;

  std::string separator = " ";

  //		std::cout << "atc before parse\n";

  // parse input.txt to create plane objects
  while (input_file_stream >> ID >> arrivalTime >> arrivalCordX >>
         arrivalCordY >> arrivalCordZ >> arrivalSpeedX >> arrivalSpeedY >>
         arrivalSpeedZ) {

    //			std::cout << ID << separator << arrivalTime <<
    // separator << arrivalCordX
    //					<< separator << arrivalCordY <<
    // separator << arrivalCordZ
    //					<< separator << arrivalSpeedX <<
    // separator << arrivalSpeedY
    //					<< separator << arrivalSpeedZ <<
    // std::endl;

    int pos[3] = {arrivalCordX, arrivalCordY, arrivalCordZ};
    int vel[3] = {arrivalSpeedX, arrivalSpeedY, arrivalSpeedZ};

    // create plane objects and add pointer to each plane to a vector
    Plane *plane = new Plane(ID, arrivalTime, pos, vel);
    planes.push_back(plane);
  }

  //		std::cout << "atc after parse\n";

  //		int i = 0;
  //		for (Plane *plane : planes) {
  //			printf("readinput: %s\n", plane->getFD());
  //		}

  return 0;
}

// initialize thread and shm members
int ATC::initialize() {
  // read input from file
  readInput();

  //		std::cout << "atc initialize after readInput()\n";

  // ============ initialize shm for waiting planes (contains all planes)
  // ============
  shm_waitingPlanes = shm_open("waiting_planes", O_CREAT | O_RDWR, 0666);
  if (shm_waitingPlanes == -1) {
    perror("in shm_open() ATC: waiting planes");
    exit(1);
  }

  // set shm size
  ftruncate(shm_waitingPlanes, SIZE_SHM_PSR);

  // map shm
  waitingPtr = mmap(0, SIZE_SHM_PSR, PROT_READ | PROT_WRITE, MAP_SHARED,
                    shm_waitingPlanes, 0);
  if (waitingPtr == MAP_FAILED) {
    printf("map failed waiting planes\n");
    return -1;
  }

  //		std::cout << "atc initialize before writing waiting planes
  // shm\n";

  // save file descriptors to shm
  int i = 0;
  for (Plane *plane : planes) {
    //			printf("initialize: %s\n", plane->getFD());

    sprintf((char *)waitingPtr + i, "%s,", plane->getFD());

    //			printf("length of planeFD: %zu\n",
    // strlen(plane->getFD()));

    // move index
    i += (strlen(plane->getFD()) + 1);
  }
  sprintf((char *)waitingPtr + i - 1, ";"); // file termination character

  //		std::cout << "atc initialize after writing waiting planes
  // shm\n";

  // ============ initialize shm for flying planes (contains no planes)
  // ============
  shm_flyingPlanes = shm_open("flying_planes", O_CREAT | O_RDWR, 0666);
  if (shm_flyingPlanes == -1) {
    perror("in shm_open() ATC: flying planes");
    exit(1);
  }

  // set shm size
  ftruncate(shm_flyingPlanes, SIZE_SHM_SSR);

  // map shm
  flyingPtr = mmap(0, SIZE_SHM_SSR, PROT_READ | PROT_WRITE, MAP_SHARED,
                   shm_flyingPlanes, 0);
  if (flyingPtr == MAP_FAILED) {
    printf("map failed flying planes\n");
    return -1;
  }
  sprintf((char *)flyingPtr, ";");

  //		std::cout << "atc after writing flying planes shm\n";

  // ============ initialize shm for airspace (compsys <-> ssr) ============
  shm_airspace = shm_open("airspace", O_CREAT | O_RDWR, 0666);
  if (shm_airspace == -1) {
    perror("in shm_open() ATC: airspace");
    exit(1);
  }

  // set shm size
  ftruncate(shm_airspace, SIZE_SHM_AIRSPACE);

  // map shm
  airspacePtr = mmap(0, SIZE_SHM_AIRSPACE, PROT_READ | PROT_WRITE, MAP_SHARED,
                     shm_airspace, 0);
  if (airspacePtr == MAP_FAILED) {
    printf("map failed airspace\n");
    return -1;
  }
  sprintf((char *)airspacePtr, ";");
  //		std::cout << "atc after writing airspace shm\n";

  // ============ initialize shm for period update ============
  shm_period = shm_open("period", O_CREAT | O_RDWR, 0666);
  if (shm_period == -1) {
    perror("in shm_open() ATC: period");
    exit(1);
  }

  // set shm size
  ftruncate(shm_period, SIZE_SHM_PERIOD);

  // map shm
  periodPtr = mmap(0, SIZE_SHM_PERIOD, PROT_READ | PROT_WRITE, MAP_SHARED,
                   shm_period, 0);
  if (periodPtr == MAP_FAILED) {
    printf("map failed period\n");
    return -1;
  }
  int per = CS_PERIOD;
  std::string CSPeriod = std::to_string(per);
  sprintf((char *)periodPtr, CSPeriod.c_str());
  //		printf("atc period shm: %s\n", periodPtr);
  //		std::cout << "atc after writing period shm\n";

  // ============ initialize shm for display ============
  shm_display = shm_open("display", O_CREAT | O_RDWR, 0666);

  // set shm size
  ftruncate(shm_display, SIZE_SHM_DISPLAY);

  // map the memory

  void *displayPtr = mmap(0, SIZE_SHM_DISPLAY, PROT_READ | PROT_WRITE,
                          MAP_SHARED, shm_display, 0);
  if (displayPtr == MAP_FAILED) {
    printf("Display ptr failed mapping\n");
    return -1;
  }
  sprintf((char *)displayPtr, ";");
  //		std::cout << "atc after writing display shm\n";

  // ============ create threaded objects ============
  // create PSR object with number of planes
  PSR *current_psr = new PSR(planes.size());
  psr = current_psr;
  //		std::cout << "atc psr created\n";

  SSR *current_ssr = new SSR(planes.size());
  ssr = current_ssr;
  //		std::cout << "atc ssr created\n";

  Display *newDisplay = new Display(); // Add nb of existing plane (in air)
  display = newDisplay;
  //		std::cout << "atc display created\n";

  ComputerSystem *newCS = new ComputerSystem(planes.size());
  computerSystem = newCS;

  //		std::cout << "atc compsys created\n";

  return 0; // set to error code if any
}
