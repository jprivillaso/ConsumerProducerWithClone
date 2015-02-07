/* parsim.cpp */
#include <iostream>
#include <algorithm>
#include <sstream>
#include <string>
#include <vector>
#include <sched.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <signal.h>
#include <stdio.h>
#include <cstring>

#define S "-s"
#define C "-c"
#define B "-b"
#define COMMA ','
#define TWO_POINTS ':'
#define WHITE_SPACE ' '
#define STACK_SIZE 16384

// Error definitions
#define SYNTAX_ERROR "Syntax Error. Try again"
#define MESSAGE_ERROR "Message Error. Try Again"
#define SERVICE_NOT_INITIALIZED_ERR "Service not initialized"
#define ZERO_SERVICES_ACTIVE_ERR "At least one service must be started"
#define SET_DEFAULT_SIZE_ERR "You must send a default queue size"
#define SET_BACKEND_QUEUE_ERR "You may pass backend queue size"
#define UNRECOGNIZED_OPERATION_ERR "Unrecognized operation"
#define DEFAULT_QUEUE_SIZE_ERR "Send a correct default queue size"
#define CONVERSION_EXCEPTION "Error. There is a number too big to cast"

//Service definitions
#define SUM 0
#define SUB 1
#define MULT 2
#define DIV 3
#define MOD 4
#define AND 5
#define OR 6
#define XOR 7
#define NAND 8
#define NOR 9

using namespace std;

vector<pid_t> threads;

struct BufferInMiddleEnd {
  int sequence;
  long long number1;
  long long number2;
  unsigned int delay;
};

struct BufferInBackEnd {
  int sequence;
  short service;
  long long result;
};

class BackEnd {
  private:
    BufferInBackEnd * itemsBackEnd;
    int bufferSize;
    int in;
    int out;
    sem_t full, empty, mutex;
  public:
    static int consume (void *);
    void produce(BufferInBackEnd);
    void start (int);
};

void BackEnd::start(int bufferSize) {

  this->bufferSize = bufferSize;
  itemsBackEnd = new BufferInBackEnd[bufferSize];

  in = 0;
  out = 0;

  sem_init(&mutex, 0, 1);
  sem_init(&full, 0, 0);
  sem_init(&empty, 0, bufferSize);

  //Assign the stack that will be used by the service's thread
  //STACK_SIZE = 16384 => 2^14
  void ** stack = (void **) malloc(STACK_SIZE) + STACK_SIZE / sizeof(*stack);
    /*
  CLONE FLAGS:
  - CLONE_VM: Clones the virtual machine. The calling process and the child
              process run in the same memory space
    CLONE_FILES: The calling process and the child share the same file
                 file descriptor.
    SIGCHLD: After  all of the threads in a thread group terminate the parent
             process of the thread group is sent a SIGCHLD (or other termina‐
             tion) signal.
  */
  pid_t thread = ::clone(BackEnd::consume, stack, CLONE_VM | CLONE_FILES |
                         SIGCHLD, this);

  threads.push_back(thread);

}

void BackEnd::produce(BufferInBackEnd item) {

  sem_wait(&empty);
  sem_wait(&mutex);
  itemsBackEnd[in] = item;
  in = (in + 1) % bufferSize;
  sem_post(&mutex);
  sem_post(&full);

}

int BackEnd::consume (void * arg) {

  //Get the reference of the BackEnd
  BackEnd * backEnd = (BackEnd*) arg;

  while(true){
    sem_wait(&backEnd->full);
    sem_wait(&backEnd->mutex);
    int sequence = backEnd->itemsBackEnd[backEnd->out].sequence;
    int result = backEnd->itemsBackEnd[backEnd->out].result;
    short service = backEnd->itemsBackEnd[backEnd->out].service;

    //Print result to the user
    cout << sequence << ":" << service << ":" << result << endl;

    backEnd->out = (backEnd->out + 1) % backEnd->bufferSize;
    sem_post(&backEnd->mutex);
    sem_post(&backEnd->empty);
  }

}

class Service {
  private:
    BufferInMiddleEnd * itemsMiddleEnd;
    BackEnd * backEnd;
    bool status;
    int type;
    int bufferSize;
    int in;
    int out;
    sem_t full, empty, mutex;
  public:
    Service();
    ~Service();
    bool getStatus();
    void start(int, int, BackEnd *);
    void produce(BufferInMiddleEnd);
    static int consume (void *);
    long long calculate(int, long long, long long);
    void produceBackEnd();
};

Service::Service() {
  status = false;
}

Service::~Service() {
    status = false;
    sem_destroy(&mutex);
    sem_destroy(&empty);
    sem_destroy(&full);
}

bool Service::getStatus() {
  return status;
}

void Service::start(int type, int bufferSize, BackEnd * backEnd){

  this->type = type;
  this->bufferSize = bufferSize;
  this->backEnd = backEnd;

  itemsMiddleEnd = new BufferInMiddleEnd[bufferSize];
  status = true;
  in = 0;
  out = 0;

  sem_init(&mutex, 0, 1);
  sem_init(&full, 0, 0);
  sem_init(&empty, 0, bufferSize);

}

void Service::produce(BufferInMiddleEnd item) {

  sem_wait(&empty);
  sem_wait(&mutex);
  itemsMiddleEnd[in] = item;
  in = (in + 1) % bufferSize;
  sem_post(&mutex);
  sem_post(&full);

}

int Service::consume (void * arg) {

  //Get the reference of the service
  Service * service = (Service*) arg;

  while(service->status){
    sem_wait(&service->full);
    sem_wait(&service->mutex);
    int delay = service->itemsMiddleEnd[service->out].delay;

    /*
    Wait the amount of time sent by the user and then produce the result
    in the backend queue
    */
    usleep(delay*1000);
    service->produceBackEnd();

    service->out = (service->out + 1) % service->bufferSize;
    sem_post(&service->mutex);
    sem_post(&service->empty);
  }

}

long long Service::calculate (int type, long long number1, long long number2) {

  long long result;

  switch(type){
    case SUM:
      result = number1 + number2;
      break;
    case SUB:
      result = number1 - number2;
      break;
    case MULT:
      result = number1 * number2;
      break;
    case DIV:
      result = number1 / number2;
      break;
    case MOD:
      result = number1 % number2;
      break;
    case AND:
      result = number1 & number2;
      break;
    case OR:
      result = number1 | number2;
      break;
    case XOR:
      result = number1 ^ number2;
      break;
    case NAND:
      result = ~(number1 & number2);
      break;
    case NOR:
      result = ~(number1 | number2);
      break;
    default:
      cout << UNRECOGNIZED_OPERATION_ERR << endl;
      break;
  }

  return result;

}

void Service::produceBackEnd() {

  //Calculate and produce the result in the BackEnd
  long long number1 = itemsMiddleEnd[out].number1;
  long long number2 = itemsMiddleEnd[out].number2;
  BufferInBackEnd item;
  item.sequence = itemsMiddleEnd[out].sequence;
  item.service = type;
  item.result = calculate(type, number1, number2);
  backEnd->produce(item);

}

class MiddleEnd{
  private:
    Service sumService;
    Service subService;
    Service multService;
    Service divService;
    Service modService;
    Service andService;
    Service orService;
    Service xorService;
    Service nandService;
    Service norService;
  public:
    Service * getService (int);
    void startService (int, int, BackEnd *);
};

Service * MiddleEnd::getService(int service) {
  switch (service) {
    case SUM :
      return &sumService;
      break;
    case SUB :
      return &subService;
      break;
    case MULT:
      return &multService;
      break;
    case DIV :
      return &divService;
      break;
    case MOD :
      return &modService;
      break;
    case AND :
      return &andService;
      break;
    case OR:
      return &orService;
      break;
    case XOR:
      return &xorService;
      break;
    case NAND:
      return &nandService;
      break;
    case NOR:
      return &norService;
      break;
  }
}

void MiddleEnd::startService (int type, int bufferSize, BackEnd * backEnd) {

  Service * service;
  //Assign the stack that will be used by the service's thread
  //STACK_SIZE = 16384 => 2^14
  void ** stack = (void **) malloc(STACK_SIZE) + STACK_SIZE / sizeof(*stack);
  pid_t thread;

  switch(type){
    case SUM:
      service = &sumService;
      break;
    case SUB:
      service = &subService;
      break;
    case MULT:
      service = &multService;
      break;
    case DIV:
      service = &divService;
      break;
    case AND:
      service = &andService;
      break;
    case OR:
      service = &orService;
      break;
    case MOD:
      service = &modService;
      break;
    case NAND:
      service = &nandService;
      break;
    case XOR:
      service = &xorService;
      break;
    case NOR:
     service = &norService;
      break;
    default:
      break;
  }

  //Going to create a thread consumer for an specific service
  service->start(type, bufferSize, backEnd);
  /*
  CLONE FLAGS:
  - CLONE_VM: Clones the virtual machine. The calling process and the child
              process run in the same memory space
    CLONE_FILES: The calling process and the child share the same file
                 file descriptor.
    SIGCHLD: After  all of the threads in a thread group terminate the parent
             process of the thread group is sent a SIGCHLD (or other termina‐
             tion) signal.
  */
  thread = clone(&Service::consume, stack, CLONE_VM | CLONE_FILES | SIGCHLD,
                 service);
  threads.push_back(thread);

}

class FrontEnd{
  private:
    int defaultQueueSize;
    int activeServices;
    bool backendQueueSent;
    bool defaultQueueSent;
  public:
    FrontEnd();
    vector<string> splitMessage(string, char);
    bool messageValidations(vector<string>);
    bool isNumber(string &, bool);
    bool allIntegersInVector(vector<string>, bool);
    void initServices(int, char **,  BackEnd *, MiddleEnd *);
    void startService(int, char **, int, MiddleEnd *, BackEnd *);
    void startBackendService(int, char **, int, BackEnd *);
    void setDefaultQueueSize(int, char **, int);
    void serviceValidations(string);
    void setProducer(vector<string>, MiddleEnd *);
    void waitForMessages(MiddleEnd *);
};

FrontEnd::FrontEnd(void){
  defaultQueueSize = 1;
  backendQueueSent = false;
  defaultQueueSent = false;
  activeServices = 0;
}

void FrontEnd::setDefaultQueueSize(int argc, char* argv[],
                                   int currentPosition) {

  // Preventing from array index out of bounds exception
  if (currentPosition + 1 < argc) {

    string queueSize = argv[currentPosition+1];

    if (isNumber(queueSize, false)) {

      defaultQueueSize = atoi(queueSize.c_str());
      defaultQueueSent = true;

    } else {
      cerr << DEFAULT_QUEUE_SIZE_ERR << endl;
      exit(0);
    }

  }

}

void FrontEnd::startBackendService(int argc, char * argv[],
                                   int currentPosition, BackEnd * backend) {

  // Preventing from array index out of bounds exception
  if (currentPosition + 1 < argc) {

    string queueSize = argv[currentPosition+1];
    int size = atoi(queueSize.c_str());

    if (size > 0) {
      backend->start(atoi(queueSize.c_str()));
      backendQueueSent = true;
    }

  } else {
    cerr << SET_BACKEND_QUEUE_ERR << endl;
    exit(0);
  }

}

// Method took from http://www.cplusplus.com/forum/beginner/31141/
bool FrontEnd::isNumber(string & s, bool canBeNegative) {
  /*
    if s[i] is between '0' and '9' or if it's a whitespace
    (there may be some before and after the number) it's ok.
    otherwise it isn't a number.
  */
  for(int i = 0; i < s.length(); i++) {

    if (canBeNegative) {

      /* If the number is negative, the minus must be in the first position*/
      if (s[i] == '-' && i != 0) return false;

      if(! (s[i] >= '0' && s[i] <= '9' || s[i] == ' '
        || s[i] == '-') ) return false;

    } else{
      if(! (s[i] >= '0' && s[i] <= '9' || s[i] == ' ') ) return false;
    }

  }

  return true;

}

bool FrontEnd::allIntegersInVector(vector<string> s, bool validateRange) {

  for (int i = 0; i < s.size(); i++) {
    if (!isNumber(s.at(i) , false)) {
      return false;
    }

    if (validateRange) {
      if (!(atoi(s.at(i).c_str()) >= 0 && atoi(s.at(i).c_str()) < 10 )) {
        return false;
      }
    }

  }

  return true;

}

bool FrontEnd::messageValidations(vector<string> messageParsed) {

  /* It is mandatory to send 5 parameters
    - Service Id
    - Services to consume
    - Parameter 1
    - Parameter 2
    - Delays
  */
  if (messageParsed.size() != 5) {
    cerr << MESSAGE_ERROR << endl;
    return false;
  }

  string serviceId = messageParsed.at(0);

  /* Service Id must not have any special character*/
  if (serviceId.find(COMMA) < serviceId.length()) {
    cerr << MESSAGE_ERROR << endl;
    return false;
  }

  /* Service id must be a number*/
  if (!isNumber(serviceId, false)) {
    cerr << MESSAGE_ERROR << endl;
    return false;
  }

  string servicesToConsume = messageParsed.at(1);
  vector<string> servicesSplitted = splitMessage(servicesToConsume, COMMA);

  /* At least on service must be consumed. Also, each service must be a
  positive integer */
  if (servicesSplitted.size() == 0 ||
     !allIntegersInVector(servicesSplitted, true)) {
    cerr << MESSAGE_ERROR << endl;
    return false;
  }

  string parameter1 = messageParsed.at(2);
  string parameter2 = messageParsed.at(3);

  /* Parameters 1 and 2 must be numbers*/
  if (!isNumber(parameter1, true) || !isNumber(parameter2, true)) {
    cerr << MESSAGE_ERROR << endl;
    return false;
  }

  try {

    string::size_type sz = 0;   // alias of size_t
    long long longParameter1 = stoll(parameter1,&sz,0);
    long long longParameter2 = stoll(parameter2,&sz,0);

  } catch (out_of_range) {
    cerr << CONVERSION_EXCEPTION << endl;
    return false;
  }

  string delays = messageParsed.at(4);
  vector<string> delaysSplitted = splitMessage(delays, COMMA);

  /* All delays must me a positive number*/

  if (!allIntegersInVector(delaysSplitted, false)) {
    cerr << MESSAGE_ERROR << endl;
    return false;
  }

  return true;

}

/* Splits a string depending on a separator sent as a parameter*/
vector<string> FrontEnd::splitMessage(string str, char separator) {

  for (int i=0; i<str.length(); i++){

    if (str[i] == separator){
      str[i] = WHITE_SPACE;
    }

  }

  vector<string> array;
  stringstream ss(str);
  string temp;
  while (ss >> temp){
    array.push_back(temp);
  }

  return array;

}

void FrontEnd::startService (int argc, char * argv[], int currentPosition,
                             MiddleEnd * middleEnd, BackEnd * backEnd) {

    int defaultSize = 1;
    string queueSize;
    string service = currentPosition + 1 < argc ?
                     argv[currentPosition + 1] : "";

    if (!isNumber(service, false)) {
      cerr << SYNTAX_ERROR << endl;
      exit(0);
    }

    // Preventing an array index out of bounds exception
    if (currentPosition + 2 < argc) {
      queueSize = argv[currentPosition + 2];
    }


    if (!queueSize.empty()) {
      // Queue size sent
      if (isNumber(queueSize, false)) {
        defaultSize = atoi(queueSize.c_str());
      } else {

        /*
        Guarrantee that, if queue size wasn't sent, then the next char must
        be -s || -c || -b
        */
        if (queueSize.find(S) > queueSize.length() && queueSize.find(B) >
            queueSize.length() && queueSize.find(C) > queueSize.length()){

          cerr << SYNTAX_ERROR << endl;
          exit(0);

        }

        // Queue size was sent but it has errors
        if (queueSize.length() > 0 && !isNumber(queueSize, false)) {
          defaultSize = 1;
        }

        /* If defaultQueueSize it means the user hasn't sent a default queue
           size */
        if (defaultQueueSize != 0) {
          defaultSize = defaultQueueSize;
        }
      }

    }

    middleEnd->startService(atoi(service.c_str()), defaultSize, backEnd);

    activeServices++;

}

void FrontEnd::initServices (int argc, char * argv [], BackEnd * backend,
                             MiddleEnd * middleEnd) {

  /* Going to parse the chain from the end to the start in order to set the
  default queue size as soon as possible and detect if there is an error with
  the chain entered by the user*/
  for (int i = argc-1; i > 0; i--) {

      string parameter = argv[i];
      /* Check special characters
      -s: Service
      -c: Default Queue
      -b: Backend Queue
      */
      if (parameter.find(S) < parameter.length()) {
        startService(argc, argv, i, middleEnd, backend);
      } else if (parameter.find(C) < parameter.length()) {
        setDefaultQueueSize(argc, argv, i);
      } else if (parameter.find(B) < parameter.length()) {
        startBackendService(argc, argv, i, backend);
      }
    }

    // Validate that backend queue was sent
    if (!backendQueueSent) {
      backend->start(1);
    }

    // Validate when a queue wasn't sent and a service has no queue size
    if (!defaultQueueSent && activeServices == 0){
      cerr << SET_DEFAULT_SIZE_ERR << endl;
      exit(0);
    }

    // Validate that at least there are one service active
    if (activeServices == 0){
      cerr << ZERO_SERVICES_ACTIVE_ERR << endl;
      exit(0);
    }
}

void FrontEnd::waitForMessages (MiddleEnd * middleEnd) {

  string input;
  bool isMessageCorrect = false;

  while (getline(cin, input)) {

    /* Termination condition 0 */
    if (input == "0") {
      break;
    }

    /* If input is empty then wait again for an user input */
    if (input.empty()) {
      continue;
    }

    if (count(input.begin(), input.end(), ':') != 4) {
      cerr << SYNTAX_ERROR << endl;
      continue;
    }

    vector<string> messageParsed = splitMessage(input, TWO_POINTS);
    isMessageCorrect = messageValidations(messageParsed);

    if (isMessageCorrect) {
      setProducer(messageParsed, middleEnd);
    }
  }
}

void FrontEnd::setProducer (vector<string> messages, MiddleEnd * middleEnd) {

  string sequence = messages.at(0);
  /* We have right now the certainty that all parameters are ok so we can
  use them directly*/
  string parameter1 = messages.at(2);
  string parameter2 = messages.at(3);

  string servicesToConsume = messages.at(1);
  vector<string> servicesSplitted = splitMessage(servicesToConsume, COMMA);

  string delays = messages.at(4);
  vector<string> delaysSplitted = splitMessage(delays, COMMA);

  int servicesSize = servicesSplitted.size();
  int delaysSize = delaysSplitted.size();
  int diff = 0;

  /*
  Validating the length of delays and servicesToConsume.
  There are two heuristics:
    - If delays are greater than services array, then delays have to be omitted
    - If services are greater than delays array, then delays have to be
        repeated
  */
  if (delaysSize > servicesSize) {

    diff = delaysSize - servicesSize;
    for (int i = 0; i < diff; i++) {
      delaysSplitted.pop_back();
    }

  } else if (servicesSize > delaysSize){

    diff = servicesSize - delaysSize;
    for (int i = 0; i < diff; i++) {
      delaysSplitted.push_back(delaysSplitted.at(delaysSize-1));
    }

  }

  //Validate that the services are initialized
  for (int i = 0; i < servicesSplitted.size(); i++) {
    int actualService = atoi(servicesSplitted.at(i).c_str());
    Service * s = middleEnd->getService(actualService);

    if (!s->getStatus()) {
      cerr << SERVICE_NOT_INITIALIZED_ERR << endl;
      return;
    }
  }

  for (int i = 0; i < servicesSplitted.size(); i++) {

    string::size_type sz = 0;   // alias of size_t

    //Create the items that are going to be produced
    BufferInMiddleEnd itemMiddleEnd;
    itemMiddleEnd.sequence = atoi(sequence.c_str());
    itemMiddleEnd.number1 = stoll(parameter1,&sz,0);
    itemMiddleEnd.number2 = stoll(parameter2,&sz,0);
    itemMiddleEnd.delay = atoi(delaysSplitted.at(i).c_str());

    //Get the service and produce the item for it
    int actualService = atoi(servicesSplitted.at(i).c_str());
    Service * s = middleEnd->getService(actualService);
    s->produce(itemMiddleEnd);
  }

}

int main(int argc, char * argv[]) {

    BackEnd backend;
    MiddleEnd middleEnd;
    FrontEnd frontEnd;

    /*Front end will start services and parse messages if the former was
    done right*/
    frontEnd.initServices(argc, argv, &backend, &middleEnd);
    frontEnd.waitForMessages(&middleEnd);

    /*
      Wait for the threads to finish. The parent with be signaled when the
      children are dead and then the parent kill them
    */
    pid_t w;
    int status;
    int threadsSize = threads.size();

    for (int i=0; i < threadsSize; i++) {
      w = waitpid(threads[i], &status, 0);
    }

    /* The following code is commented for a purpose. The day of the explanation
       this approach will be mentioned

       sleep(20);
       for (int i = 0; i < threads.size(); ++i){
         kill(threads.at(i), 9);
       }

    */

    if (w == -1) {
         perror("waitpid");
         exit(EXIT_FAILURE);
    }

    return 0;

}