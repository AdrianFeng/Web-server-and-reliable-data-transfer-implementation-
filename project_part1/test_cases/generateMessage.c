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


//--------------------------------------------------------------------------------------------
//content can be changed
#include <time.h>
#include <sys/stat.h>

#define HTML "Content-Type: text/html\r\n"
#define TXT "Content-Type: text/plain\r\n"
#define JPEG "Content-Type: image/jpeg\r\n"
#define JPG "Content-Type: image/jpg\r\n"
#define GIF "Content-Type: image/gif\r\n"

#define STATUS_200 "HTTP/1.1 200 OK\r\n"
#define STATUS_404 "HTTP/1.1 404 Not Found\r\n\r\n"
#define ERROR_404_HTML "<h1>Error 404: File Not Found!</h1> <br><br> <h3>File requested must be in same directory as server.</h3>"


typedef enum
{
    html, txt, jpeg, jpg, gif
    
}file_extension;


file_extension determineTypeOfFile(char* file_name);
char* changeExtensionToString( file_extension ext);
void generateMessage(int, char*, int);
char* datetime();
char* lastModified();
char* clength(int);
//----------------------------------------------------------------------------------------------

void error(char *msg)
{
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[])
{
    int sockfd, newsockfd, portno, pid;
    socklen_t clilen;
    struct sockaddr_in serv_addr, cli_addr;
    
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
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    
    if (bind(sockfd, (struct sockaddr *) &serv_addr,
             sizeof(serv_addr)) < 0)
        error("ERROR on binding");
    
    listen(sockfd,5);	//5 simultaneous connection at most
    
    while(1) {
    //accept connections
    newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
    
    if (newsockfd < 0)
        error("ERROR on accept");
    
    int n;
    char buffer[256];
    
    memset(buffer, 0, 256);	//reset memory
    
 		 //read client's message
    n = read(newsockfd,buffer,255);
    if (n < 0) error("ERROR reading from socket");
    printf("Here is the message: %s\n",buffer);
    
    //reply to client
    n = write(newsockfd,"I got your message",18);
    if (n < 0) error("ERROR writing to socket");
    
    generateMessage(newsockfd, "hello.html", 50);
    
    close(newsockfd);//close connection
    }
    close(sockfd);
    
    return 0;
}

//--------------------------------------------------------------------------------------------------
void generateMessage(int socket, char* filename, int lengthfile)
{
    
    char response[512];
    
    char *status = STATUS_200;
    char *server = "Server: FNU Pramono/1.0 (WIN 32)\r\n";
    
    char *connection_status = "Connection: closed\r\n";
    
    file_extension fe = determineTypeOfFile(filename);
    char *string_fe = changeExtensionToString (fe);
    
    strcpy(response, status);
    strcat(response, datetime());
    strcat(response, server);
    strcat(response, lastModified(filename));
    strcat(response, clength(lengthfile));
    strcat(response, string_fe);
    strcat(response, connection_status);
    
    printf("\n\nresponse: %s\n\n", response);
    
}

char* clength(int ln)
{
    char strings[80] = "Content-Length: ";
    
    char str[10];
    sprintf(str, "%d", ln);
    
    strcat(strings, str);
    strcat(strings, "\r\n");
    
    return strings;
    
}

char *datetime()
{
    struct tm* times;
    time_t t = time(NULL);
    times = gmtime(&t);
    char responseTime[35];
    strftime(responseTime, 35, "%a, %d %b %Y %T %Z", times);
    
    char date[50] = "Date: ";
    strcat(date, responseTime);
    strcat(date, "\r\n");
    
    return date;
    
}

char *lastModified(char* fn)
{
    struct tm* t;
    struct stat lm;
    
    stat(fn, &lm);
    
    t = gmtime(&(lm.st_mtime));
    
    char times_lastmodify[35];
    strftime(times_lastmodify, 35, "%a, %d %b %Y %T %Z", t);
    
    char strings[50] = "Last-Modified: ";
    strcat(strings, times_lastmodify);
    strcat(strings, "\r\n");
    
    return strings;
}

file_extension determineTypeOfFile(char* file_name)
{
    if (strstr(file_name, ".html") != NULL) return html;
    if (strstr(file_name, ".txt") != NULL) return txt;
    if (strstr(file_name, ".jpeg") != NULL) return jpeg;
    if (strstr(file_name, ".jpg") != NULL) return jpg;
    if (strstr(file_name, ".gif") != NULL) return gif;
    
    // in case of client error, assume plaintext
    return txt;
}

char* changeExtensionToString (file_extension ext)
{
    char *string_type;
    
    string_type = TXT;
    
    if(ext==html) string_type = HTML;
    if(ext==jpeg) string_type = JPEG;
    if(ext==jpg) string_type = JPG;
    if(ext==gif) string_type = GIF;
    
    return string_type;
}

//----------------------------------------------------------------------------------------------------