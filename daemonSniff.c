#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <syslog.h>
#include <wait.h>

#include<netinet/in.h>
#include<errno.h>
#include<netdb.h>
#include<netinet/ip_icmp.h>   //Provides declarations for icmp header
#include<netinet/udp.h>   //Provides declarations for udp header
#include<netinet/tcp.h>   //Provides declarations for tcp header
#include<netinet/ip.h>    //Provides declarations for ip header
#include<netinet/if_ether.h>  //For ETH_P_ALL
#include<net/ethernet.h>  //For ether_header
#include<sys/socket.h>
#include<arpa/inet.h>


/*---------------------------------------------------------------------------------------*/
int main(int argc, char* argv[]) {

	int fd; 
	char str[80];
	// FIFO file path 
	char * sfifo = "/tmp/sfifo";
  // Creating the named file(FIFO) 
	// mkfifo(<pathname>,<permission>) 
	mkfifo(sfifo, 0666); 

  pid_t parpid, sid;
  
  parpid = fork(); //make the parent process
  if(parpid < 0) {
      exit(1);//error
  } else if(parpid != 0) {
      printf ("sniffer processID: %d\n", parpid);
      //wait (NULL);
      exit(0);//if it parent finish it, because it did all what we need
  } 
  umask(0);//даем права на работу с фс
  sid = setsid();//генерируем уникальный индекс процесса
  if(sid < 0) {
      exit(1);
  }
  if((chdir("/")) < 0) {//выходим в корень фс
      exit(1);
  }
  close(STDIN_FILENO);//закрываем доступ к стандартным потокам ввода-вывода
  close(STDOUT_FILENO);
  close(STDERR_FILENO);
  
  //init sniffer part
  int saddr_size , data_size;
  unsigned char *buffer = (unsigned char *) malloc(1024); //Its Big!
  struct sockaddr saddr;
  int sock_raw = socket( AF_PACKET , SOCK_RAW , htons(ETH_P_ALL)) ;
  if(sock_raw < 0)return 1; //error 
  //init sniffer part end
  
  fd = open(sfifo, O_WRONLY); 
	//DAEMON ITSELF
  while(1) {
    saddr_size = sizeof saddr;
    //Receive a packet - Trouble! - now it blocs execution untill it receives the package
    data_size = recvfrom(sock_raw , buffer , 65536 , 0 , &saddr , (socklen_t*)&saddr_size);
    if(data_size <0 ) return 1;//Recvfrom error , failed to get packets
    //Now process the packet
    //Get the IP Header part of this packet , excluding the ethernet header
    struct iphdr *iph = (struct iphdr*)(buffer + sizeof(struct ethhdr));
    // Open FIFO for write only 
    union{
      unsigned int ip;
      unsigned char cip[4];
    }uip;// IP unpacking 
    uip.ip = iph->saddr;      
	  memset (str, 0 ,sizeof(str));
	  write(fd, uip.cip, sizeof (uip.cip)); 
	  //sprintf (str, "%d.%d.%d.%d\n",(int) uip.cip[0],(int) uip.cip[1],(int) uip.cip[2],(int) uip.cip[3] );
	  //write(fd, str, sizeof (str)); 
	  
		      
    //sleep(1);//wait sec
  }
  close(fd);
  close(sock_raw);
  return 0;
}




