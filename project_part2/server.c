#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <time.h>
#define SIZE 1024

void error(char *msg)
{
    perror(msg);
    exit(1);
}
struct pkt
{
  // define the 1 means request 
  // 2 means normal data 
  // 3 means Acknowledgement
  // 4 means coonection is over 
  // 5 means retransmit ack
  int type; // 4
  int errortype; // size 4, 0 means corrupted,  1 means good  
  int sequence; // size 4 
  int loss; // size 4, 0 means loss, 1 means good
  //int sizeofdata;
  size_t length; // size 8 
  char buffer[1000];
};
int corrgenerate(double pc);
int lossgenerate(double pl);
int sendpkt(int sequencenum, int cwndsize, int type, struct pkt* window, FILE* FileHandler,double pl, double pc);
int main (int argc, char * argv[]){
    int waitting_mode; // 1 means waitting, 0 means continue;
    int currentskfd, newskfd,PortNumber,answer,temp;
    socklen_t Clilent;
    struct sockaddr_in ServerAddress, ClilentAddress;
    char RequestHoldBuffer[SIZE];
    char copy[1024];
    char *FileName;
    FILE* FileHandler;
    ssize_t check;
    int ReadSize;
    struct pkt packet;
    double pl,pc;
    int cwndsize;
    int sequencenum=0; int uptonumber=-1;
    int windowindex=0;
    struct timeval TP;
    fd_set SET; 
    int resending_flag=0;
    int closeflag =1; // 1 means not finish reading, 0 means finish 
    int corrupt; // 1 means good, 0 means corrupted
    int loss; // 1 means good, 0 means loss;
    int i;
    if (argc !=5 ){
		  printf("please input 5 arguments \n");
		  exit(-1);
	  }
    PortNumber=atoi(argv[1]);
    printf("port number is : %d\n", PortNumber );
    cwndsize=atoi(argv[2]);
    cwndsize=cwndsize/1024;
    printf("current window size %d packets since each packets is 1024 byte\n", cwndsize);
    struct pkt * window = (struct pkt *)malloc(cwndsize*sizeof(struct pkt ));
    pl=atof(argv[3]);
    printf("server loss probaility is %f\n", pl );
	  pc=atof(argv[4]);
    printf("server corruption probaility is %f\n", pc );
	  if (pl>1 || pc>1 || pl<0 || pc<0 || cwndsize<0 || PortNumber<0){
		  printf("value input is invalid\n");
		  exit(-1);
	  }

    currentskfd = socket(AF_INET, SOCK_DGRAM, 0);	//create socket
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
    Clilent=sizeof(ClilentAddress);
      // accept the request and assign it to the new socket
    corrupt = corrgenerate(pc);
    loss = lossgenerate(pl);
    while (corrupt ==0 || loss ==0 ){
      corrupt = corrgenerate(pc);
      loss = lossgenerate(pl);
    }
    memset((struct pkt*) &packet, 0, sizeof(packet));
    check = recvfrom(currentskfd, (struct pkt *) &packet, sizeof(packet),0,(struct sockaddr *) &ClilentAddress, &Clilent);
    if (check < 0) {
       error("Something wrong on receiveing");
    }
    //printf("good here 2\n");
    FileHandler=fopen(packet.buffer,"r");
    printf("requested file is %s\n", packet.buffer );
    printf("\n \n \n");
    printf("-----------------start transmitting------------------- \n");
    printf("\n \n \n");
    for (i=0;(i<cwndsize) && closeflag==1;i++){
      windowindex=sequencenum%cwndsize;
      ReadSize=sendpkt(sequencenum,cwndsize,2,window, FileHandler, pl,pc);
      if (ReadSize == 0){
        ReadSize=sendpkt(sequencenum,cwndsize,4,window, FileHandler,pl,pc);
        closeflag=0;
      }
      check =sendto(currentskfd, (struct pkt *) &window[windowindex] , 1024, 0, (struct sockaddr*) &ClilentAddress, Clilent);
      if (check < 0) {
        error("Something wrong on Sending");
      }
      if (ReadSize !=0){
        printf("sent number seq # %d packet with data content is %d bytes \n",i,ReadSize);
      }
      else{
        printf("no more data needs to transmit so send fin and packet # %d is fin\n",sequencenum);
      }
      sequencenum++;
    }
    //printf("good here 1\n");
    while(1){
      HERE1:
      TP.tv_sec = 0;
      TP.tv_usec=10000;
      FD_ZERO(&SET);
      FD_SET(currentskfd, &SET);
      answer=select(currentskfd+1, &SET, NULL, NULL, &TP);
      if (answer !=-1 && answer !=0){ // normally receive 
        if (FD_ISSET(currentskfd, &SET)){ // current socket have the data
          memset((struct pkt*) &packet, 0, sizeof(packet));
          check = recvfrom(currentskfd, (struct pkt *) &packet, sizeof(packet),0,(struct sockaddr *) &ClilentAddress, &Clilent);
          if (check<0)
            error("ERROR ON RECEIVING\n");
          corrupt = corrgenerate(pc);
          loss = lossgenerate(pl);
          //printf("\n %d is the ack number with loss: %d and error: %d\n", packet.sequence,packet.loss, packet.errortype);
          //-----------------------------------------------------------------------------//
          if (packet.type==4){
            printf("\nreceive ack of fin, close connection\n");
            break;
          }
          if (loss==0){
            printf("\nreceive ack with sequence number %d is loss \n",packet.sequence);
          }
          else if (corrupt ==0){
            printf("\nreceive ack with sequence number %d is corrupted\n",packet.sequence);
          }
          else{
            printf("\nreceive ack with sequence number %d is good\n",packet.sequence);
          }
          //-----------------------------------------------------------------------------//
         if (packet.type==3){
            if (corrupt == 1 && loss ==1){ // good we receive 
              if (packet.sequence>=(uptonumber+1)){
                windowindex=sequencenum%cwndsize;
                ReadSize=sendpkt(sequencenum,cwndsize,2,window, FileHandler, pl,pc);
                if (ReadSize == 0){
                  ReadSize=sendpkt(sequencenum,cwndsize,4,window, FileHandler,pl,pc);
                  printf("no more data needs to transmit so send fin and packet # %d is fin\n",sequencenum);
                }
                else{
                  printf("sent number seq # %d packet with data content is %d bytes \n",sequencenum,ReadSize);  
                }
                check =sendto(currentskfd, (struct pkt *) &window[windowindex] , 1024, 0, (struct sockaddr*) &ClilentAddress, Clilent);
                if (check < 0) 
                  error("Something wrong on Sending");
                sequencenum++;
                uptonumber=packet.sequence;
                resending_flag=0;
              }
            }
            else if (loss ==0){ // pkt loss 
              printf("\nignore\n" );
              continue;
            }
            else { // pkt is corrupted
              printf("\ndrop\n" );
              continue;
            }
          }
          else if (packet.type ==5 ){ 
             if (loss ==0){ // pkt loss 
              printf("\nignore\n" );
              continue;
            }
            else if (corrupt== 0){ // pkt is corrupted
              printf("\ndrop\n" );
              continue;
            }
            uptonumber=packet.sequence;
            printf("\nthis ack is for retransmitting the sequence # %d packet \n",uptonumber+1 );
            continue;
          }
          else { // wrong type we ignore
            printf("wrong type we ignore \n");
            continue;
          }
        }
      }
      else if (answer == -1) { // exception
        error("select error"); // error occurred in select()
      }
      else{ // timeout mode 
        printf("\nthis is time out mode\n");
        temp =uptonumber+cwndsize+1;
        resending_flag=0;
        for (i=0; i<cwndsize;i++){
          windowindex=temp%cwndsize;
          if (window[windowindex].sequence>uptonumber){
            resending_flag=1;
            printf("\nresent seq # %d packet\n",window[windowindex].sequence);
            window[windowindex].errortype=corrgenerate(pc);
            window[windowindex].loss=lossgenerate(pl);
            check =sendto(currentskfd, (struct pkt *) &window[windowindex] , 1024, 0, (struct sockaddr*) &ClilentAddress, Clilent);
            if (check<0)
              error("ERROR ON RECEIVING\n");
          }
          temp++;
          if (resending_flag ==0 ){
            printf("the resending flag is zero and no packet are resent\n");
            printf("upto number is %d\n", uptonumber);
          }
        }
        if (resending_flag==0){
          windowindex=sequencenum%cwndsize;
          ReadSize=sendpkt(sequencenum,cwndsize,2,window, FileHandler, pl,pc);
          if (ReadSize == 0){
            ReadSize=sendpkt(sequencenum,cwndsize,4,window, FileHandler,pl,pc);
          }
         printf("sent number seq # %d packet with data content is %d bytes \n",sequencenum,ReadSize);
          check =sendto(currentskfd, (struct pkt *) &window[windowindex] , 1024, 0, (struct sockaddr*) &ClilentAddress, Clilent);
          if (check < 0) 
            error("Something wrong on Sending");
          sequencenum++;
          uptonumber=packet.sequence;
          resending_flag=0;
        }
      }
    }       
   	fclose(FileHandler);
   	close(currentskfd);
   	return 0;
}

int corrgenerate(double pc){
  time_t t;
  //srand((unsigned) time(&t));
  double value = (double)(rand()%100+1);
  //printf("generate the pc %f\n", value);
  if (pc == 1 ){return 0;}
  if (value < pc*100){
    return 0; // corrupted;
  }
  else{
    return 1;
  }
}

int lossgenerate(double pl){
  time_t t;
  //srand((unsigned) time(&t));
  double value = (double)(rand()%100+1);
  //printf("generate the pl %f\n", value);
  if (pl == 1 ){return 0;}
  if (value < pl*100){
    return 0; // loss;
  }
  else{
    return 1;
  }
}

int sendpkt(int sequencenum, int cwndsize,int type, struct pkt* window, FILE* FileHandler,double pl, double pc){
  int windowindex=sequencenum%cwndsize;
  int ReadSize;
  memset((struct pkt*)&window[windowindex], 0, sizeof(struct pkt));
  if (type ==2){
    window[windowindex].type=type;
    window[windowindex].errortype=corrgenerate(pc);
    window[windowindex].sequence = sequencenum;
    window[windowindex].loss=lossgenerate(pl);
    ReadSize = fread(window[windowindex].buffer, sizeof(char), sizeof(window[windowindex].buffer), FileHandler);
    window[windowindex].length=ReadSize;
  }
  else{ // since only 4 wil be the other wise value, which indicates the FIN
    window[windowindex].type=type;
    window[windowindex].errortype=0;
    window[windowindex].sequence = sequencenum;
    window[windowindex].loss=0;
    ReadSize=0; 
    window[windowindex].length=ReadSize;
  }
  return ReadSize;
}
  







   
