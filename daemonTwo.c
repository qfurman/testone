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

/*---------------------------------------------------------------------------------------*/
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
  union{
    unsigned int ip;
    unsigned char cip[4];
  }uip;// IP unpacking
  
	int fd1, fd2, fdc, fdm; 
	// FIFO file path 
	char * dfifo = "/tmp/dfifo";//read cli
	char * sfifo = "/tmp/sfifo";//read sniffer
	char * cfifo = "/tmp/cfifo";//wrire answer to cli
	char * mfifo = "/tmp/mfifo";//manage the flow data to cli
  // Creating the named file(FIFO) 
	// mkfifo(<pathname>,<permission>) 
	mkfifo(dfifo, 0666); 
	mkfifo(sfifo, 0666); 
	mkfifo(cfifo, 0666);
	mkfifo(mfifo, 0666);
	
  writeLog("Daemon Start");

  pid_t parpid, sid;
  
  parpid = fork(); //make the parent process
  if(parpid < 0) {
      exit(1);//error
  } else if(parpid != 0) {
      printf ("processID: %d\n", parpid);
      //wait (NULL);
      exit(0);//terminate the parent process because it did everything we need
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
  
  //create first item of list
  if ( (hptr = create_node ()) == NULL ) return 1;//memory trouble
  
  //DAEMON ITSELF
  while(1) {
    memset (&str1, 0, 80);
    // First open in read only and read 
    fd1 = open(dfifo, O_RDONLY | O_NONBLOCK); 
    
    if (fd1){
      read(fd1, str1, 80); //read FIFO command 
      command = str1[0];
    }
    
    // open in read only and read 
    fd2 = open(sfifo, O_RDONLY | O_NONBLOCK); 
    if (fd2){
      read(fd2, uip.cip, 4); //read FIFO for new IP
      if (uip.ip){// if non zerro ip 
        total++;
        udate_list (uip.ip); //update or create new reccord
      }
    } 
           
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
        int accq;  
        accq = sprintf(str2, "%d stat command IP Total : %d IP Unique: %d\n", counter, total, unique);
        writeLog(str2);  
        
        fdc = open(cfifo, O_WRONLY);//fifo for answer to cli
        fdm = open(mfifo, O_WRONLY);//fifo for manage answer to cli
        bptr = hptr; //init pointer to top of list 
        accq++;// now it included \0 
        
        write(fdc, str2, accq);//send to cli header
        write(fdm, &accq, sizeof accq );//send to cli count
            
        while (1) {
          uip.ip  = bptr->ip;
          accq = sprintf (str2, "IP %03d.%03d.%03d.%03d count: %d\n", (int)uip.cip[0], (int)uip.cip[1], (int)uip.cip[2], (int)uip.cip[3], bptr->cntr);
          writeStat(str2);
          accq++;// now it included \0 
          write(fdc, str2, accq);//send to cli
          write(fdm, &accq, sizeof accq );//send to cli count
          if (bptr->next) bptr = bptr->next;
          else break;
        }
        close(fdc);
        accq = 0;//end of transmition
        write(fdm, &accq, sizeof accq );//send to cli end flag         
        close(fdm);
        break;  
      default:
        command = 0;
        counter++;
        sprintf(str2, "%d unknown command\n", counter);
        writeLog(str2);      
        break;        
    }       
    sleep(1);//wait sec
  }
  close(fd1);
  close(fd2);   
  return 0;
}




