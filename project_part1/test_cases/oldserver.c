/* A simple server in the internet domain using TCP
   The port number is passed as an argument 
   This version runs forever, forking off a separate 
   process for each connection
*/
#include <stdio.h>
#include <sys/types.h>   // definitions of a number of data types used in socket.h and netinet/in.h
#include <sys/socket.h>  // definitions of structures needed for sockets, e.g. sockaddr
#include <netinet/in.h>  // constants and structures needed for internet domain addresses, e.g. sockaddr_in
#include <stdlib.h>
#include <strings.h>
#include <sys/wait.h>	/* for the waitpid() system call */
#include <signal.h>	/* signal name macros, and the kill() prototype */
#include <unistd.h>
char *requestDump(int);
void error(char *msg)
{
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[])
{
	 char cwd[1024];//buffer to store the current dir
     int sockfd, newsockfd, portno, pid;
     socklen_t clilen;
     int ListenResult;
     struct sockaddr_in serv_addr, cli_addr;
     /////////////current directory//////////////////////
	if (getcwd(cwd, sizeof(cwd)) == NULL)
    	   perror("getcwd() error");
	//////////////////////////////////////////////////////
     if (argc < 2) {
         fprintf(stderr,"ERROR, no port provided\n");
         exit(1);
     }
     sockfd = socket(AF_INET, SOCK_STREAM, 0);	//create socket
     if (sockfd < 0) 
        error("ERROR opening socket");
     memset((char *) &serv_addr, 0, sizeof(serv_addr));	//reset memory
     //fill in address info
     portno = atoi(argv[1]);
     serv_addr.sin_family = AF_INET;
     serv_addr.sin_addr.s_addr = INADDR_ANY; //The constant INADDR_ANY is the so-called IPv4 wildcard address. The wildcard IP address is useful for applications that bind Internet domain sockets on multihomed hosts. 
     serv_addr.sin_port = htons(portno);
     
     if (bind(sockfd, (struct sockaddr *) &serv_addr,
              sizeof(serv_addr)) < 0) 
              error("ERROR on binding, try another port number");
     ListenResult=listen(sockfd,5);	//5 simultaneous connection at most
     if (ListenResult<0){
     	 error("ERROR on listening");
     }
     //accept connections
     while(1){
     clilen=sizeof(cli_addr);
     newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
         
     if (newsockfd < 0) 
       error("ERROR on accept");
         
     int n;
   	 //read client's message
   	 //n = read(newsockfd,buffer,511);
   	 //if (n < 0) error("ERROR reading from socket");
   	 char *filename;
	filename = requestDump(newsockfd);
   	 //printf("Here is the message: %s\n",buffer);
	 printf("%s\n",filename);
	 ///////////////////////////read file/////////////////////////////////
	 int string_size=2048;
	 char content[string_size];
   int read_size;
   FILE *handler = fopen(filename,"r");
	if(handler == NULL) {
	// ignore
	perror("file doesn't exist");
	 continue;
	}
	while((read_size = fread(content, sizeof(char), string_size, handler))!= 0) {
       //fread doesnt set it so put a \0 in the last position
       //and buffer is now officialy a string
	    write(newsockfd,content,read_size);
	   
    }
    fclose(handler);
	//printf("%s\n",content);
	 /////////////////////////////////////////////////////////////////////
	 //////////////////////////////read file /////////////////////////////
	 /*char line[80];
	 printf("1 \n");
	 FILE* fr = fopen(filename,"r");
	  printf("2 \n");
	 long elapsed_seconds;
	  printf("3 \n");
	 while(fgets(line, 80, fr) != NULL)
   		{
   			 //printf("4 \n");
	 		get a line, up to 80 chars from fr.  done if NULL 
	 		sscanf (line, "%ld", &elapsed_seconds);
	 		convert the string to a long int 
	 		printf ("%ld\n", elapsed_seconds);
	 		//n = write(newsockfd,line,80);
   		}
   	fclose(fr);*/
   	//////////////////////////////////////////////////////////////////
   	 //reply to client
   	 //n = write(newsockfd,"I got your message",18);    
     
     close(newsockfd);//close connection
     }
     close(sockfd);
     return 0; 
}

char *requestDump (int sock)
{
	int n;
	char buffer[512];
	bzero(buffer,512);
	n = read(sock,buffer,511); // read socket request message into buffer

	if (n < 0) perror("Socket: error reading!");
	
	// make a copy of buffer to tokenize
	char fname[512];
	memcpy(fname, buffer, 512);

	// tokenize request message based on space; grab 2nd token (file name)
	char *token;
	const char space[2] = " ";
	token = strtok(fname, space);
	token = strtok(NULL, space);

	// increment the token to get rid of leading '/'
	token++;

	//however, if token is empty (no file selected), set token to '\0'
	if(strlen(token)<=0) token = "\0";

	// print buffer contents into console
	printf("HTTP REQUEST MESSAGE:\n%s\n", buffer);

	// print file name into console
	//printf("TOKEN:\n%s\n", token);

	// Writing more messages to browser
	// n = write(sock,"I got your message",18);
	// if (n < 0) perror("Socket: error writing!");

	return token;
}
