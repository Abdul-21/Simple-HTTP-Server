#include "server.h"

int maxConnectionAllowed,portNumber;
char rootDir[LINES],indexName[LINES];
int threadCount = 0,currentConn = 0;
pthread_t thread[32];
pthread_mutex_t currentConn_lock;
char httpHeader[8000]=
	"HTTP/1.0 200 OK\r\n"
	"Content-Type: text/html\r\n\r\n";

void *threadFunction(void * arg) {
  char buf[BUFSIZE];
	struct threadArg *tArg = (struct threadArg *) malloc(sizeof(struct threadArg));
  tArg = arg;
  //Checks that something was received
  ssize_t status;
  status = recv(tArg->clientfd, &buf, sizeof(buf), 0);
  if (status < 0){
    perror("Nothing received");
  }
	//printf("%s\n",buf);
	printf("%s\n",httpHeader);
  //Sends the index page
  send(tArg->clientfd,httpHeader,strlen(httpHeader),0);
	close(tArg->clientfd);
	//free(tArg);
  pthread_mutex_lock(&currentConn_lock);
  currentConn--;
  pthread_mutex_unlock(&currentConn_lock);
  return NULL;
}

//Function to take populate variables from config file
void populateFromConfig(FILE *configFp){
  char parsedLine[LINES];
	printf("\n");
	printf("FOLLOWING CONFIGURATION LOADED:\n");
	printf("========================================\n");
  fgets(parsedLine, sizeof(parsedLine), configFp);
  sscanf(parsedLine, "%d\[^\n]",&maxConnectionAllowed);
  printf("Max simulataneous connections allowed: %d\n",maxConnectionAllowed);

  fgets(parsedLine, sizeof(parsedLine), configFp);
  sscanf(parsedLine, "%s\[^\n]",(char *)&rootDir);
  printf("Root directory to use: %s\n",rootDir);

  fgets(parsedLine, sizeof(parsedLine), configFp);
  sscanf(parsedLine, "%s\[^\n]",(char *)&indexName);
  printf("Index file name: %s\n",indexName);

  fgets(parsedLine, sizeof(parsedLine), configFp);
  sscanf(parsedLine, "%d\[^\n]",&portNumber);
  printf("Port number: %d\n",portNumber);
	printf("========================================\n");
	printf("\n");
}

//Function to log errors and access
void file_logging(){
  int alog = open("../logs/access.log", O_RDWR|O_CREAT|O_APPEND, 0600);
  if (alog == -1){
    perror("Error when opening access log file");
    exit(0);
  }
  int stdout_fd= dup(fileno(stdout));
  if (dup2(alog, fileno(stdout)) == -1){
      perror("Stdout could not be redirected to log file");
      exit(0);
  }
  int elog = open("../logs/error.log", O_RDWR|O_CREAT|O_APPEND, 0600);
  if (elog == -1) {
    perror("Error when opening error log file");
    exit(0);
  }
  int stderr_fd= dup(fileno(stderr));
  if (dup2(elog, fileno(stderr)) == -1){
    perror("Stderr could not be redirected to log file");
    exit(0);
	}
}

//Function to get the full directory of a graphic or html file
char *getFullDirectory(char fileName[], int isInImageFolder){
	char *buf;
	buf = (char *)malloc(sizeof(rootDir)*2);
	strcpy(buf,rootDir);
	if(isInImageFolder == 1){
		strcat(buf,"images/");
		}
	strcat(buf,fileName);
	return (char *)buf;
}

int main(int argc, char *argv[]){
  int s,sd,bout,addrLen;
  int optval = 1;
  char line[100];
  char parsedData[8000];
  struct sockaddr_in sin,address;

	//Allows for errors and messages produced by stdout/stderr to be written to the log files
	printf("If this is not listening for a connection, Please check your error logs\n");
	// file_logging();

	//Populates global variable for config options
	FILE *fp = fopen("../conf/httpd.conf", "r");
	if(fp == NULL){
		perror("Configuration file could not be opened");
		return 1;
	}
	else{
		populateFromConfig(fp);
		fprintf(stdout,"Config file successfully loaded\n");
		fclose(fp);
	}

  //Erases data in "sin" then initializes it
  bzero((char *) &sin, sizeof(sin));
  sin.sin_family = AF_INET;
  sin.sin_addr.s_addr = htonl(INADDR_ANY);
  sin.sin_port = htons(portNumber);

  //Creates a socket
  s = socket(AF_INET, SOCK_STREAM, 0);
  if(s < 0){
    perror("Socket could not be created");
		return 1;
  }else{
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int));
    fprintf(stdout, "Socket setup and opened with return code: \"%d\"\n",s);
  }

  //Binds the socket
  bout = bind(s, (struct sockaddr *)&sin, sizeof(sin));
  if (bout < 0){
    perror("Socket could not be bound");
		return 1;
  }
  else{
    fprintf(stdout, "Socket successfully bound with return code: \"%d\"\n",bout);
  }

  //Waits for a connection
  int l = listen(s, maxConnectionAllowed);
  if (l < 0){
    perror("The socket is not listening");
    return 1;
  }
	else{
			fprintf(stdout,"Now listening for connection\n");
	}

  //Adds a http header to the index page
	FILE *htmlData = fopen(getFullDirectory(indexName,0), "r");
  if(htmlData == NULL){
    perror("File was not opened");
		return 1;
  }
  while(fgets(line, 100, htmlData) != 0){
    strcat(parsedData, line);
  }
  strcat(httpHeader, parsedData);

while(1){
  //Accepts and receives the inbound connection
  addrLen = sizeof(struct sockaddr_in);
  sd = accept(s, (struct sockaddr *)&address, (socklen_t *)&addrLen);
  if(sd < 0){
    perror("Connection could not be completed");
		return 1;
  }
  else {
    fprintf(stdout, "Connection attempted\n");
  }

  struct threadArg *arg = (struct threadArg *) malloc(sizeof(struct threadArg));
  arg->clientfd = sd;

  if(threadCount>=maxConnectionAllowed-1){
    threadCount=0;
  }
  pthread_create(&thread[threadCount], NULL,threadFunction, (void*) arg);


  pthread_mutex_lock(&currentConn_lock);
  threadCount++;
	currentConn--;
  pthread_mutex_unlock(&currentConn_lock);
  if(threadCount == maxConnectionAllowed) {
  	printf("Max concurrent connections reached\n");
    close(sd);
    free(arg);
    continue;
	}
}
  close(sd);
  return 0;
}
