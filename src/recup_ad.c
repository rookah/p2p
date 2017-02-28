#include <stdio.h>
#include <sys/socket.h>
#include <netdb.h>

#include "headers/socklib.h"



int main(int argc, char *argv[]) {
  int res;
  int s;
  struct addrinfo info;
  socklen_t tailleinfo = sizeof(info);
  char hname[NI_MAXHOST], sname[NI_MAXSERV];
  

  s = CreeSocketClient("localhost", "8080");


  // pour avoir sa propre IP a partir d'une socket
  res = getsockname(s, (struct sockaddr *)&info, &tailleinfo);
  if (res == -1) {
    perror("getsockname");
    exit(1);
  }
  res = getnameinfo((struct sockaddr *)&info, tailleinfo,
		    hname, NI_MAXHOST,
		    sname, NI_MAXSERV,
		    NI_NUMERICSERV|NI_NUMERICHOST);
  if (res != 0) {
    fprintf(stdout, "getnameinfo: %s\n", gai_strerror(res));
    exit(1);
  }
  
  printf ("Mon adresse IP et mon port sont %s le port %s\n",
	  hname, sname); 

  // pour avoir l'IP du pair à l'autre bout d'une socket
  res = getpeername(s, (struct sockaddr *)&info, &tailleinfo);
  if (res == -1) {
    perror("getsockname");
    exit(1);
  }
  res = getnameinfo((struct sockaddr *)&info, tailleinfo,
		    hname, NI_MAXHOST,
		    sname, NI_MAXSERV,
		    NI_NUMERICSERV|NI_NUMERICHOST);
  if (res != 0) {
    fprintf(stdout, "getnameinfo: %s\n", gai_strerror(res));
    exit(1);
  }
  
  printf ("L'IP et le port de l'autre sont %s le port %s\n",
	  hname, sname); 
  
  

  return 0;

}
