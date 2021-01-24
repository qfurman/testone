#include <stdio.h>
#include <string.h>
#include <fcntl.h> 
#include <sys/stat.h> 
#include <sys/types.h> 
#include <unistd.h> 

int main(int argc, char **argv) { 
	int fd, fdc, fdm;
	int command = 0; 
	char msg[128];
  /*
  union{
    unsigned int ip;
    unsigned char cip[4];
  }uip;// 
  int ip[4];//IP request 
  */
  
	// FIFO file path 
	char * dfifo = "/tmp/dfifo"; 
  char * cfifo = "/tmp/cfifo";//read answer to daemon
  char * mfifo = "/tmp/mfifo";//manage the flow data to cli
  
	// Creating the named file(FIFO) 
	mkfifo(dfifo, 0666); // mkfifo(<pathname>, <permission>) 
	mkfifo(cfifo, 0666); // mkfifo(<pathname>, <permission>) 
	mkfifo(mfifo, 0666);
	  
  if (argc < 2) {
    printf ("Nothig to do. Type --help, to get help\n");
    return 0;
  }
  
  if (strcasecmp ("start", argv[1]) == 0)command = 1;
  if (strcasecmp ("stop", argv[1]) == 0)command = 2;
  if (strcasecmp ("show", argv[1]) == 0)command = 3;
  if (strcasecmp ("select", argv[1]) == 0)command = 4;
  if (strcasecmp ("stat", argv[1]) == 0)command = 5;
  if (argc == 3 && strcasecmp ("count", argv[2]) == 0){
    command = 6;
    //sscanf (argv[1], "%d.%d.%d.%d", &ip[0], &ip[1], &ip[2], &ip[3]);
    //for (int i=0;i<4;i++) uip.cip[i] = ip[i];
  } 
  if (command == 0) printf ("got an unknown command, type --help to get help\n");
  
  if (command){
    // Open FIFO for write only 
		fd = open(dfifo, O_WRONLY | O_NONBLOCK); 
		// Write on FIFO 
		// and close it 
		if (fd){
		  switch (command){
		    case 1:
		      printf ("got a start command\n");
          write(fd, "1", 2);
          break;
        case 2:
		      printf ("got a stop command\n");
          write(fd, "2", 2);
          break;  
        case 3:
		      printf ("got a show command\n");
          write(fd, "3", 2);
          break;          
        case 4:
		      printf ("got a select command\n");
          write(fd, "4", 2);
          break;          
        case 5:
        case 6:
          if (command == 5){
            printf ("got a stat command\n");
            write(fd, "5", 2);
          } else {
            printf ("got a count command\n");
            memset (msg, 0, sizeof msg);
            int qb = sprintf (msg, "6.%s", argv[1]) + 1;//write ip direct from argement, count include /0
            write(fd, msg, qb);          
          }		      
		      int accm;
		      
		      fdc = open(cfifo, O_RDONLY); 
		      if (fdc < 0 ) break;
		      fdm = open(mfifo, O_RDONLY);
		      if (fdm < 0 ) break;
		      while (1){
		        read(fdm, &accm, sizeof accm); //read count from FIFO flow control
		        if (accm){
              memset (msg, 0, sizeof msg);
              read(fdc, msg, accm); //read FIFO for new IP
              printf ("%s", msg);
            } else break;
          }      
          close(fdc);
          close(fdm);
          break;

        break;           
        default:
		      printf ("got a unknown command\n");
          break;                   
      }     
		  close(fd); 
		}else {
		  printf ("sniffer has not started yet\n");
		}
  }
  
  if (strcasecmp ("--help", argv[1]) == 0){
    printf ("start​​\n(packets are being sniffed from now on from default iface(eth0))\nstop​​\n(packets are not sniffed)\nshow [ip] count\n(print number of packets received from ip address)\nselect iface [iface]\n(select interface for sniffing eth0, wlan0, ethN, wlanN...)\nstat​​ ​ [iface]​​\nshow all collected statistics for particular interface, if iface omitted - for all interfaces\n--help\n(show usage information)\n");
  }

  return 0;
}




