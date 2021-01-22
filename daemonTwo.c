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

/*--------------------------------------------------------------------------*/
void handler (int num);
/*--------------------------------------------------------------------------*/

int Daemon(int * cmd);
char* getTime();
int writeLog(char msg[256]);
char* getCommand(char command[128]);
/*---------------------------------------------------------------------------------------*/
char* getTime() { //функция возвращает форматированную дату и время
    time_t now;
    struct tm *ptr;
    static char tbuf[64];
    bzero(tbuf,64);
    time(&now);
    ptr = localtime(&now);
    strftime(tbuf,64, "%Y-%m-%e %H:%M:%S", ptr);
    return tbuf;
}
/*---------------------------------------------------------------------------------------*/
char* getCommand(char command[128]) { //функция возвращает результат выполнения linux команды
    FILE *pCom;
    static char comText[256];
    bzero(comText, 256);
    char  buf[64];
    pCom = popen(command, "r"); //выполняем
    if(pCom == NULL) {
        writeLog("Error Command");
        return "";
    }
    strcpy(comText, "");
    while(fgets(buf, 64, pCom) != NULL) { //читаем результат
        strcat(comText, buf);
    }
    pclose(pCom);
    return comText;
}
/*---------------------------------------------------------------------------------------*/
int writeLog(char msg[256]) { //функция записи строки в лог
    FILE * pLog;
    pLog = fopen("/tmp/daemonTwo.log", "a");
    if(pLog == NULL) {
        return 1;
    }
    char str[312];
    bzero(str, 312);
    strcpy(str, getTime());
    //strcat(str, " ==========================\n");
    strcat(str, msg);
    strcat(str, "\n");
    fputs(str, pLog);
    //fwrite(msg, 1, sizeof(msg), pLog);
    fclose(pLog);
    return 0;
}
/*---------------------------------------------------------------------------------------*/
int writeStat(char msg[256]) { //функция записи строки в лог
    FILE * pLog;
    pLog = fopen("/tmp/daemonStat.log", "a");
    if(pLog == NULL) {
        return 1;
    }
    fputs(msg, pLog);
    fclose(pLog);
    return 0;
}
/*---------------------------------------------------------------------------------------*/
struct sockaddr_in source,dest;
int total=0, unique=0; 

//define linked list
typedef struct List{ 
  int ip;
  int cntr;
  struct List *next;
}List;

struct List * hptr = NULL;//linked list Head pointer
struct List * cptr = NULL;//linked list current write pointer
struct List * bptr = NULL;//linked list current read pointer

/*---------------------------------------------------------------------------------------*/
List * create_node ()
{
  List * ptr = (List *) malloc (sizeof(List));
  if(ptr == NULL) return NULL;//memory trouble
  memset(ptr, 0, sizeof(List));
  return ptr;
}
/*---------------------------------------------------------------------------------------*/
//incremet counter for existing IP's or add a new one
int udate_list (int ip)
{
  cptr = hptr;//set points to top
  
  while (1){
    if ( (cptr->ip == 0) && (cptr->cntr == 0) ){
      //first try
      unique++;
      cptr->ip = ip;//write ip in new position
      cptr->cntr++;//if ip already exist than inctrement counter
      return 0;// everything allright
    }
    
    if (cptr->ip == ip) {
      cptr->cntr++;//if ip already exist than inctrement counter
      return 0;// everything allright
    } else {
      if (cptr->next) cptr = cptr->next;
      else {
        if ( (cptr->next  = create_node ()) == NULL ) return 1;//memory trouble
        cptr = cptr->next;//go to the new node
        unique++;
        cptr->ip = ip;//write ip in new position
        cptr->cntr++;//if ip already exist than inctrement counter      
        return 0;//success
      }
    }
  }
}
/*---------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------*/
int main(int argc, char* argv[]) {
  
	char command; // command for daemon
  int counter = 0; 
	char str1[80], str2[80];
  
	int fd1; 
	// FIFO file path 
	char * dfifo = "/tmp/dfifo";
  // Creating the named file(FIFO) 
	// mkfifo(<pathname>,<permission>) 
	mkfifo(dfifo, 0666); 
	
  writeLog("Daemon Start");

  pid_t parpid, sid;
  
  parpid = fork(); //make the parent process
  if(parpid < 0) {
      exit(1);//error
  } else if(parpid != 0) {
      printf ("processID: %d\n", parpid);
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
  unsigned char *buffer = (unsigned char *) malloc(65536); //Its Big!
  struct sockaddr saddr;
  //create first item of list
  if ( (hptr = create_node ()) == NULL ) return 1;//memory trouble
  int sock_raw = socket( AF_PACKET , SOCK_RAW , htons(ETH_P_ALL)) ;
  if(sock_raw < 0)return 1; //error 
  //init sniffer part end
  
  //DAEMON ITSELF
  while(1) {
    memset (&str1, 0, 80);
    // First open in read only and read 
    fd1 = open(dfifo, O_RDONLY | O_NONBLOCK); 
    
    if (fd1){
      read(fd1, str1, 80); //read FIFO command 
      command = str1[0];
    }
    
    //--sniffer part
    saddr_size = sizeof saddr;
    //Receive a packet - Trouble! - now it blocs execution untill it receives the package
    data_size = recvfrom(sock_raw , buffer , 65536 , 0 , &saddr , (socklen_t*)&saddr_size);
    //if(data_size <0 ) return 1;//Recvfrom error , failed to get packets
    //Now process the packet
    //Get the IP Header part of this packet , excluding the ethernet header
    if (data_size){
      struct iphdr *iph = (struct iphdr*)(buffer + sizeof(struct ethhdr));
      ++total;
      udate_list (iph->saddr);
      //writeLog("-");
    }
    //--sniffer part end 
           
    switch (command ){
      case 0:
        break;    
      case '1':
        command = 0;
        counter++;
        sprintf(str2, "%d start command\n", counter);
        writeLog(str2);
        break;
      case '2':
        command = 0;
        counter++;
        sprintf(str2, "%d stop command\n", counter);
        writeLog(str2);      
        break;
      case '3':
        command = 0;
        counter++;
        sprintf(str2, "%d show command\n", counter);
        writeLog(str2);      
        break;      
      case '4':
        command = 0;
        counter++;
        sprintf(str2, "%d select command\n", counter);
        writeLog(str2);      
        break;           
      case '5':
        command = 0;
        counter++;
        sprintf(str2, "%d stat command IP Total : %d IP Unique: %d\n ", counter, total, unique);
        writeLog(str2);      

        union{
          unsigned int ip;
          unsigned char cip[4];
        }uip;// IP unpacking  
              
        bptr = hptr; //init pointer to top of list   
        
        while (1) {
          uip.ip  = bptr->ip;
          sprintf (str2, "IP %d.%d.%d.%d count: %d\n", (int)uip.cip[0], (int)uip.cip[1], (int)uip.cip[2], (int)uip.cip[3], bptr->cntr);
          writeStat(str2);
          if (bptr->next) bptr = bptr->next;
          else break;
        }        
        break;  
      default:
        command = 0;
        counter++;
        sprintf(str2, "%d unknown command\n", counter);
        writeLog(str2);      
        break;        
    }
       
    //sleep(1);//wait sec
  }
  close(sock_raw);
  return 0;
}




