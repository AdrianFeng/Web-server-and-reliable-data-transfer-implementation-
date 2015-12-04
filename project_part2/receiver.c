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
#include <memory.h>
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
void packetsent(struct pkt* packet, int normal, double pl, double pc, int pktseq, int type);
int main(int argc, char *argv[]){
	int answer;
	fd_set SET; 
	struct timeval TP;
	double pl,pc;
	int socketfd;
	int port;
	char* filname;
	char* host;
	struct sockaddr_in serv_addr;
	struct sockaddr_in newreceiver;
	struct hostent *server; 
	ssize_t check;
	char* buffer;
	char input[SIZE];
	socklen_t newsize;
	FILE* FileHandler;
	struct pkt packet;
	int WriteSize;
	int corrupt; // 1 means good, 0 means corrupted
    int loss; // 1 means good, 0 means loss;
	char* temp1;
	char* temp2; temp2="copyof";
	int pktseq=-1; // the sequence number that the packet can have 
	if (argc !=6 ){
		printf("please input 6 arguments \n");
		exit(-1);
	}
	pl=atof(argv[4]);
	printf("server loss probaility is %f\n", pl );
	pc=atof(argv[5]);
	printf("server corruption probaility is %f\n", pc );
	host=argv[1];
	printf("host is %s\n", host );
	port=atoi(argv[2]);
	printf("port is %d\n", port);
	filname=argv[3];
	printf("request file is %s\n",filname);

	if (port< 0 || pl>1 || pc>1 || pl<0 || pc<0){
		printf("value input is invalid\n");
		exit(-1);
	}
	socketfd = socket(AF_INET, SOCK_DGRAM, 0);	//create socket
     if (socketfd < 0) 
        error("ERROR opening socket");
    server = gethostbyname(host); 
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(-1);
    }
    memset((char *) &serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET; //initialize server's address
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(port);
    packet.type=1;
    packet.errortype=1;
    packet.loss=1;
    packet.sequence = 0;
    strcpy(packet.buffer, filname);
    check =sendto(socketfd, (struct pkt *) &packet, sizeof(packet), 0, (struct sockaddr*) &serv_addr, sizeof(serv_addr));
    printf("\n\n\n--------------------sending request----------------------\n\n\n");
    if (check< 0)
		error("ERROR sending request");
	while(1){
      TP.tv_sec = 5;
      TP.tv_usec=100000;
      FD_ZERO(&SET);
      FD_SET(socketfd, &SET);
      answer=select(socketfd+1, &SET, NULL, NULL, &TP);
      if (answer !=-1 && answer !=0){ 
      	break;
      }
      else if (answer == -1) { // exception
        error("select error"); // error occurred in select()
      }
      else{ // timout resent 
      	printf("tmieout resent \n");
      	check =sendto(socketfd, (struct pkt *) &packet, sizeof(packet), 0, (struct sockaddr*) &serv_addr, sizeof(serv_addr));
      	if (check< 0)
		error("ERROR sending request");
      }
  	}
	temp1 = (char *) malloc(1 + strlen(temp2)+ strlen(filname) );
	strcpy(temp1, temp2);
    strcat(temp1, filname);
	FileHandler = fopen(temp1,"w");
	 //printf("good here 1\n");
	while (1){
		memset((char *) &newreceiver, 0, sizeof(newreceiver));
		memset((struct pkt*) &packet, 0, sizeof(packet));
		newsize=sizeof(newreceiver);
		check=recvfrom(socketfd, (struct pkt *) &packet, sizeof(packet), 0, (struct sockaddr*) &newreceiver, &newsize);
		corrupt = corrgenerate(pc);
        loss = lossgenerate(pl);
		if (check<0)
			error("ERROR ON RECEIVING\n");
		// case close connection
		if (packet.type==4){
			if (packet.sequence!= (pktseq+1)){
				goto HERE;
			}
			printf("\nreceive packet with seq = # %d is good and this fin to indicates close connection and we send fin back" , packet.sequence );
			memset((struct pkt*) &packet, 0, sizeof(packet));
			packet.type=4;
			packet.loss =1;
			packet.errortype=1;
			packet.sequence=pktseq+1;
			check =sendto(socketfd, (struct pkt *) &packet, sizeof(packet), 0, (struct sockaddr*) &newreceiver, newsize);
			if (check<0)
              error("ERROR SENDING\n");
          	printf("\n");
          	printf("\nclose connection\n");
			break;
		}
		else if (packet.type == 2){ 
			if (loss == 1 && corrupt ==1 ){ // normal data 
				HERE:
				if (packet.sequence== (pktseq+1)){
					WriteSize=fwrite(packet.buffer, sizeof(char), packet.length, FileHandler);
					printf("\nreceive packet with seq = # %d is good, so we accept and writting size is %d\n", packet.sequence,WriteSize );
					pktseq=packet.sequence;
					packetsent(&packet, 0, pl, pc, pktseq,3);
					check =sendto(socketfd, (struct pkt *) &packet, sizeof(packet), 0, (struct sockaddr*) &newreceiver, newsize);
					if (check<0)
		      			error("ERROR SENDING\n");
		      		printf("\nsend ack # %d to show packet successfully received upto # %d \n",pktseq, pktseq);
				}
				else if (packet.sequence<(pktseq+1)){
					packetsent(&packet, 1, pl, pc, pktseq,5);
					check =sendto(socketfd, (struct pkt *) &packet, sizeof(packet), 0, (struct sockaddr*) &newreceiver, newsize);
					if (check<0)
		      			error("ERROR SENDING\n");
		      		printf("ack # %d for resending \n",pktseq);
		      		printf("\nreceive packet with # %d has been received already, so drop \n", packet.sequence);
				}
				else {
					printf("receive packet with #%d is good, however packet received only up to # %d, so send ack # %d again \n",packet.sequence,pktseq,pktseq);
					packetsent(&packet, 0, pl, pc, pktseq,5);
					check =sendto(socketfd, (struct pkt *) &packet, sizeof(packet), 0, (struct sockaddr*) &newreceiver, newsize);
					if (check<0)
		      			error("ERROR SENDING\n");
		      		//printf("packet with #%d is good, however packet received only up to # %d, so send ack # %d again \n",packet.sequence,pktseq,pktseq);
				}
			}
			else if (loss == 0) {
				printf("\nreceive packet wih sequence number %d is loss, so ignore \n",packet.sequence);
				continue;
			}
			else{
				printf("\nreceive packet wih sequence number %d is corrupted  ",packet.sequence);
				packetsent(&packet, 0, pl, pc, pktseq,5);
				check =sendto(socketfd, (struct pkt *) &packet, sizeof(packet), 0, (struct sockaddr*) &newreceiver,newsize);
				if (check<0)
		      		error("ERROR SENDING\n");
		      	printf("resending ack with # %d to let server retransmit the # %d packet\n",pktseq, pktseq+1);
			}
		}
		else { //unknown packet type silent ignore 
			continue;
		}
	}
	fclose(FileHandler);
	close(socketfd);
	free(temp1);
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

void packetsent(struct pkt* packet, int normal, double pl, double pc, int pktseq, int type){ 
   	// normal 1 means normal packet, 0 otherwise
	memset((struct pkt*) packet, 0, sizeof(struct pkt));
	(*packet).type=type;
	pc=(normal==1)?1:pc;
	pl=(normal==1)?1:pl;
	(*packet).errortype=corrgenerate(pc);
	(*packet).loss=lossgenerate(pl);
	(*packet).sequence=pktseq;
}



	