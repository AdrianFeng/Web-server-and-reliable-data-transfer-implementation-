/*
 * webserver version 1.0 
 * by Zhen Feng and FUN Pramono
 * Ex. This code is based on the sample code and we put some more functions and
 * features to make this one works as the spec indicate.
 */
#include <stdio.h>
#include <sys/types.h>   // definitions of a number of data types used in socket.h and netinet/in.h
#include <sys/socket.h>  // definitions of structures needed for sockets, e.g. sockaddr
#include <sys/stat.h>
#include <time.h>
#include <netinet/in.h>  // constants and structures needed for internet domain addresses, e.g. sockaddr_in
#include <stdlib.h>
#include <strings.h>
#include <sys/wait.h>	/* for the waitpid() system call */
#include <signal.h>	/* signal name macros, and the kill() prototype */
#include <unistd.h>

char* getfilename(char * RequestBuffer);
void postthefile(char * filename, int newskfd,char * RequestBuffer);
long getthefilelength(char * filename);
int filetype(char * filename);
void error(char *msg)
{
    perror(msg);
    exit(1);
}
struct tm *currenttime;
struct stat attrib;
char timenow[20];
void lastmodifiedtime(char* filename);


int main (int argc, char * argv[]){
     int currentskfd, newskfd,PortNumber;
     socklen_t Clilent;
     int ListenResult;
     struct sockaddr_in ServerAddress, ClilentAddress;
     char RequestHoldBuffer[490];
     char copy[490];
     int BufferCheck;
     char *FileName;
     FILE* FileHandler;
     PortNumber=atoi(argv[1]);;// input number assigned as the portnumber 
     currentskfd = socket(AF_INET, SOCK_STREAM, 0);	//create socket
     if (currentskfd < 0){ 
        error("error on opening a socket");
     }
     memset((char *) &ServerAddress, 0, sizeof(ServerAddress));
     ServerAddress.sin_family = AF_INET;
     ServerAddress.sin_addr.s_addr = INADDR_ANY; //The constant INADDR_ANY is the so-called IPv4 wildcard address. The wildcard IP address is useful for applications that bind Internet domain sockets on multihomed hosts. 
     ServerAddress.sin_port = htons(PortNumber);
     if (bind(currentskfd, (struct sockaddr *) &ServerAddress,sizeof(ServerAddress)) < 0) {
     	 error("error on binding, try another port number");
     }
      ListenResult=listen(currentskfd,5);// currently setting 5 connections can work at the same time 
      if (ListenResult<0){
     	 error("Something wrong on Listening");
     }
     // Here starts the loop of acccption
     while(1){
    	Clilent=sizeof(ClilentAddress);
      // accept the request and assign it to the new socket
     	newskfd = accept(currentskfd, (struct sockaddr *) &ClilentAddress, &Clilent);
     	if (newskfd < 0) {
       		error("Something wrong on accepting");
       	}
        // reset the buffer
       	memset(RequestHoldBuffer, 0, 490);
        // read the request and then store the information in the buffer
       	BufferCheck = read(newskfd,RequestHoldBuffer,489);
        // create a copy of the buffer
       	memcpy(copy,RequestHoldBuffer,490);
   	 	if (BufferCheck < 0) error("error on reading from socket");
      // parese the request and get the name 
   	 	FileName=getfilename(RequestHoldBuffer);
   	 	if (FileName != NULL){
        // based on the request file we fetch the file and show it on the browser 
        postthefile(FileName,newskfd,copy);
   	 	}else{
        // sample test connection so no file is requested 
        printf("\n \nServer by Zhen Feng and FNU Pramono:\n");
        printf("HTTP version 1.1 200 OK\n");
        printf("No file is requested \n");
   	 		printf("\nHere is the requested message:\n%s\n \n ",copy);
        write(newskfd,"<h1>Server test OK but no file is requested</h1>",48);
   	 	}
   	 	close(newskfd);
   	 }
   	 close(currentskfd);
   	 return 0;
}


// function that get the file name from the request 
char* getfilename(char * RequestBuffer){

	char * Element;
	const char Separeter[2] = " ";
  // let the space be the separter and we split the request and parse out he request file's name 
	Element = strtok(RequestBuffer, Separeter);
  // and we choose the second one as our file name since it is just after get
	Element = strtok(NULL, Separeter);
  // we can either add . in the front or we totally ignore the / before the actual filename
  // we choose the latter one 
	Element++;
	if(strlen(Element)<=0 ){
		 Element = NULL;
	}
	return Element;
}
// function to show the content to the browser 
void postthefile(char * filename, int newskfd, char * RequestBuffer){
    // slient ignore the nouseful request
    if (strcmp(filename,"favicon.ico")==0){
      return;
    }
    // create a tempory buffer
	  int TemporySize=2048;
	  char content[TemporySize];
    long filesize;
    int type;
    const char *types[] = {"text/html","text/txt","img/jpeg","img/jpg","img/gif","type not specified in spec"};
    // based on the file name provided and we create a file descriptor
    FILE *handler = fopen(filename,"r");
    if(handler == NULL) {
      // case file doesn't exist 
      printf("\n \nServer by Zhen Feng and FNU Pramono:\n");
      printf("HTTP version 1.1 404 file is not Found\n \n");
      printf("Here is the requested message:\n%s\n",RequestBuffer);
		  perror("file doesn't exist");
      write(newskfd,"<h1>404 File Not Found</h1>",27);
		  return;
	  }
    else {
      // case file exist
      fclose(handler);
      type= filetype(filename);
      printf("\n \nServer by Zhen Feng and FNU Pramono:\n");
      printf("HTTP version 1.1 200 OK \n");
      // get the file length
      printf("content type is %s\n", types[type]);
      lastmodifiedtime(filename);
      printf("Last-Modified-Time :%s (DD-MM-YY)\n", timenow);
      filesize=getthefilelength(filename);
      printf("Requested file is ./%s and its size is %ld \n \n",filename,filesize);
      printf("Here is the requested message:\n%s\n",RequestBuffer);
      // use loop to keep reading the file and write the content to the browser until nothing left 
      handler = fopen(filename,"r");
      while(1) {
        ReadSize = fread(content, sizeof(char), TemporySize, handler);
        if (ReadSize==0){
          break;
        }
          write(newskfd,content,ReadSize);
        }
    }
    fclose(handler);
}
// function to get he file length
long getthefilelength(char * filename){
	long FileSize;
  // create a file descriptor 
	FILE *handler = fopen(filename,"r");
	if(handler == NULL) {
		perror("file doesn't exist");
		FileSize=0;
		return FileSize;
	}
  // set the file pointer to the end of the file 
	fseek(handler, 0L, SEEK_END);
  // use ftell to calculate the length of the file 
	FileSize = ftell(handler);
	fclose(handler);
	return FileSize;
}

int filetype(char * filename){
  if (strstr(filename, ".html") != NULL) {
    return 0;
  }
  else if (strstr(filename, ".txt") != NULL){
    return 1;
  }
  else if (strstr(filename, ".jpeg") != NULL){
    return 2;
  }
  else if (strstr(filename, ".jpg") != NULL){
    return 3;
  }
  else if (strstr(filename, ".gif") != NULL){
    return 4;
  }
  else{
    return 5;
  }
}
// get the last modified time of file and store the information into timenow
void lastmodifiedtime(char* filename){
  stat(filename,&attrib);
  currenttime=localtime(&(attrib.st_ctime));
  strftime(timenow, 20, "%d-%m-%y", currenttime);
}