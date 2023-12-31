# Remote Console

This repository is part of a tutorial on <a href="https://www.netburner.com">NetBurner.com</a>, [Remote Web Console](https://www.netburner.com/learn/remote-web-console/)

This project was developed for NetBurner's NNDK 3.4+ -- for 3.5 no backporting
is required, but for 3.4 please follow the Backporting instructions to include
the necessary OS prerequisites for your NNDK.

All code in this example and repository are Copyright NetBurner, Inc, and may
only be executed on NetBurner provided hardware. See LICENSE.md for more details.

### Backporting to 3.4

These steps will not work for NNDKs older than 3.4.
**If you prefer, you can download the files or ZIP from GitHub instead of copy-pasting.**

1. Create these files (backported from 3.5) in `C:\NetBurner` or your NNDK_ROOT:
  - `nbrtos\include\remoteconsole.h`
```
#ifndef NB_RMTCONSOLE_H
#define NB_RMTCONSOLE_H
void EnableRemoteConsole();
#endif
```
  - `nbrtos\source\remoteconsole.cpp`
```
#include <init.h>
#include <nbrtos.h>
#include <system.h>
#include <remoteconsole.h>

#include <websockets.h>
#include <iointernal.h>
#include <iosys.h>
#include <utils.h>
#include <tcp.h>
#include <nbrtos.h>
#include <fdprintf.h>


extern http_wshandler *TheWSHandler;
int ws_fd = -1;

void BadRequestResponse(int sock, PCSTR url, PCSTR data);
void NotAvailableResponse(int sock, PCSTR url);
using namespace NB;
int httpstricmp(PCSTR s1, PCSTR sisupper2);

static int ShimFd;


void ShimCallBack(int fd ,FDChangeType ct,void *p);

int MyDoWSUpgrade(HTTP_Request *req, int sock, PSTR url, PSTR rxb)
{
    if (httpstricmp(url, "/STDIO"))
    {
        if (ws_fd < 0)
        {
            int rv = WSUpgrade(req, sock);
            if (rv >= 0)
            {
                ws_fd = rv;
                NB::WebSocket::ws_setoption(ws_fd, WS_SO_TEXT);
        RegisterFDCallBack(ws_fd,ShimCallBack,0); 
        return 2;
            }
            else
            {
                return 0;
            }
        }
    return 0;
    }

    NotFoundResponse(sock, url);
    return 0;
}

int shim_fd;
IoExpandStruct shim_io;
int OldStdio[3];


int ShimRead(int fd, char *buf, int nbytes)
{  int rv=0;

/*  fdprintf(OldStdio[1],"fd_ws:%d da=%d ",ws_fd,dataavail(OldStdio[0]));
  if(ws_fd>0) 
    fdprintf(OldStdio[1],",da=%d ",ws_fd,dataavail(ws_fd));
*/
if(
    (!dataavail(OldStdio[0])) && 
  ((ws_fd<0) || (!dataavail(ws_fd)))
   )
{
   // fdprintf(OldStdio[1],"Select");
//Do a select                                               
   fd_set read_fds;                                         
   FD_ZERO(&read_fds);                                      
   FD_SET(fd, &read_fds);                         
   select(FD_SETSIZE, &read_fds,(fd_set *)0,(fd_set *)0,0); 
}

  if((ws_fd>0)&& dataavail(ws_fd)) rv=read(ws_fd,buf,nbytes);
  else
  if(dataavail(OldStdio[0]))rv= read(OldStdio[0],buf,nbytes);

  bool da=((ws_fd>0)&& dataavail(ws_fd));
  da|=dataavail(OldStdio[0]);

  if(da)
    SetDataAvail(fd);
    else
    ClrDataAvail(fd);
  return rv;
}

int ShimWrite(int fd, const char *buf, int nbytes)
{
write(OldStdio[1],buf,nbytes);
if((ws_fd>0)&& (writeavail(ws_fd))) write(ws_fd,buf,nbytes); 

return nbytes;
}

int ShimClose(int fd)
{
return 1;
}
int ShimPeek (int fd, char *buf)
{
  return 0;
}

void ShimCallBack(int fd ,FDChangeType ct,void *p)
{
if(dataavail(fd)) SetDataAvail(shim_fd); 
  //fdprintf(OldStdio[1],"cb fd%d da=%d ",fd,dataavail(fd));
  //fdprintf(OldStdio[1],"charavail=%d ",charavail());


if(fd==ws_fd)
 {
  switch(ct)
  { case eReadSet: break;
    case eWriteSet: break;
    case eErrorSet:
      {int ows=ws_fd;
       ws_fd=-1;
       close(ws_fd);
      }
  }
 }
}

void InitStdioShim()
{
  shim_io.read=ShimRead;
  shim_io.write=ShimWrite;
  shim_io.close=ShimClose;
  shim_io.peek=ShimPeek;
  

  shim_fd=GetExtraFD(0,&shim_io);
  SetWriteAvail(shim_fd);                                  
  OldStdio[0]=ReplaceStdio(0, shim_fd); 
  OldStdio[1]=ReplaceStdio(1, shim_fd); 
  OldStdio[2]=ReplaceStdio(2, shim_fd); 
  RegisterFDCallBack(OldStdio[0],ShimCallBack,0);
                                                                            
}

int ServeValidResponse(int sock, HTTP_Request &pd)
{
    writestring(sock, "HTTP/1.0 200 OK\r\nPragma: no-cache\r\nContent-Type: application/json\r\n\r\n");
    if(ws_fd>0) 
    writestring(sock, "{\"Valid\":true}");
  else
    writestring(sock, "{\"Valid\":false}");
    return 1;
}
extern const unsigned long console_html_size; 
extern const unsigned char console_html_data[];

int ServeConsoleHtml(int sock,HTTP_Request &pd)
{
SendHTMLHeader(sock);
writeall(sock,(const char *)console_html_data,console_html_size);
close(sock);
return 1;
}

CallBackFunctionPageHandler ValidWS("ValidWS.json", ServeValidResponse);

CallBackFunctionPageHandler ServeConsole("console.html", ServeConsoleHtml);

void EnableRemoteConsole()
{
  InitStdioShim();
   TheWSHandler = MyDoWSUpgrade;
}

```
  - `nbrtos\source\console_html.cpp`
```
/*Autogenerated data file */
extern const unsigned long console_html_size; 
extern const unsigned char console_html_data[];
const unsigned long console_html_size = 3200; 
const unsigned char console_html_data[3200] = {

60,33,68,79,67,84,89,80,69,32,72,84,77,76,62,13,
10,60,104,116,109,108,62,13,10,60,104,101,97,100,62,13,
10,60,115,99,114,105,112,116,32,116,121,112,101,61,34,116,
101,120,116,47,106,97,118,97,115,99,114,105,112,116,34,62,
13,10,118,97,114,32,119,115,59,13,10,118,97,114,32,77,
65,88,95,84,69,82,77,73,78,65,76,95,76,69,78,32,
61,32,50,48,48,48,59,13,10,118,97,114,32,98,82,101,
99,111,110,110,101,99,116,61,102,97,108,115,101,59,13,10,
13,10,102,117,110,99,116,105,111,110,32,80,117,116,73,110,
84,101,114,109,105,110,97,108,40,114,101,99,101,105,118,101,
100,95,109,115,103,41,13,10,123,13,10,32,32,118,97,114,
32,116,101,114,109,105,110,97,108,32,61,32,100,111,99,117,
109,101,110,116,46,103,101,116,69,108,101,109,101,110,116,66,
121,73,100,40,34,116,101,114,109,105,110,97,108,34,41,59,
13,10,32,32,118,97,114,32,97,117,116,111,83,99,114,111,
108,108,32,61,32,40,116,101,114,109,105,110,97,108,46,115,
99,114,111,108,108,84,111,112,32,61,61,32,40,116,101,114,
109,105,110,97,108,46,115,99,114,111,108,108,72,101,105,103,
104,116,32,45,32,116,101,114,109,105,110,97,108,46,99,108,
105,101,110,116,72,101,105,103,104,116,41,41,59,13,10,32,
32,118,97,114,32,100,97,116,97,76,101,110,32,61,32,114,
101,99,101,105,118,101,100,95,109,115,103,46,108,101,110,103,
116,104,59,13,10,32,32,118,97,114,32,116,101,114,109,105,
110,97,108,76,101,110,32,61,32,116,101,114,109,105,110,97,
108,46,118,97,108,117,101,46,108,101,110,103,116,104,59,13,
10,32,32,47,47,32,84,104,101,32,102,111,108,108,111,119,
105,110,103,32,99,108,97,117,115,101,32,105,115,32,116,111,
32,112,114,101,118,101,110,116,32,98,114,111,119,115,101,114,
115,32,102,114,111,109,32,99,114,97,115,104,105,110,103,32,
116,104,101,32,112,97,103,101,46,46,46,13,10,32,32,105,
102,32,40,40,116,101,114,109,105,110,97,108,76,101,110,32,
43,32,100,97,116,97,76,101,110,41,32,62,32,77,65,88,
95,84,69,82,77,73,78,65,76,95,76,69,78,41,32,123,
13,10,32,32,32,32,118,97,114,32,100,101,108,116,97,32,
61,32,116,101,114,109,105,110,97,108,76,101,110,32,43,32,
100,97,116,97,76,101,110,32,45,32,77,65,88,95,84,69,
82,77,73,78,65,76,95,76,69,78,59,13,10,32,32,32,
32,116,101,114,109,105,110,97,108,46,118,97,108,117,101,32,
61,32,116,101,114,109,105,110,97,108,46,118,97,108,117,101,
46,115,117,98,115,116,114,105,110,103,40,100,101,108,116,97,
44,32,116,101,114,109,105,110,97,108,76,101,110,41,59,13,
10,32,32,125,13,10,32,32,116,101,114,109,105,110,97,108,
46,118,97,108,117,101,32,43,61,32,114,101,99,101,105,118,
101,100,95,109,115,103,59,13,10,32,32,105,102,32,40,97,
117,116,111,83,99,114,111,108,108,41,32,123,13,10,32,32,
32,32,116,101,114,109,105,110,97,108,46,115,99,114,111,108,
108,84,111,112,32,61,32,116,101,114,109,105,110,97,108,46,
115,99,114,111,108,108,72,101,105,103,104,116,32,45,32,116,
101,114,109,105,110,97,108,46,99,108,105,101,110,116,72,101,
105,103,104,116,59,13,10,32,32,125,13,10,125,13,10,13,
10,102,117,110,99,116,105,111,110,32,77,97,107,101,68,97,
116,97,83,111,99,107,101,116,40,114,101,115,111,117,114,99,
101,44,32,113,117,101,114,121,41,13,10,123,13,10,32,32,
105,102,32,40,34,87,101,98,83,111,99,107,101,116,34,32,
105,110,32,119,105,110,100,111,119,41,32,123,13,10,32,32,
32,32,47,47,32,105,102,32,40,40,119,115,61,61,110,117,
108,108,41,32,124,124,32,40,119,115,46,114,101,97,100,121,
83,116,97,116,101,62,87,101,98,83,111,99,107,101,116,46,
79,80,69,78,41,41,32,123,13,10,32,32,32,32,32,32,
47,47,32,76,101,116,32,117,115,32,111,112,101,110,32,97,
32,119,101,98,32,115,111,99,107,101,116,13,10,32,32,32,
32,32,32,104,111,115,116,32,61,32,119,105,110,100,111,119,
46,108,111,99,97,116,105,111,110,46,104,111,115,116,110,97,
109,101,59,13,10,32,32,32,32,32,32,112,111,114,116,32,
61,32,40,119,105,110,100,111,119,46,108,111,99,97,116,105,
111,110,46,112,111,114,116,33,61,39,39,41,32,63,32,40,
39,58,39,43,119,105,110,100,111,119,46,108,111,99,97,116,
105,111,110,46,112,111,114,116,41,32,58,32,39,39,59,13,
10,32,32,32,32,32,32,119,115,32,61,32,110,101,119,32,
87,101,98,83,111,99,107,101,116,40,34,119,115,58,47,47,
34,43,104,111,115,116,43,112,111,114,116,43,34,47,34,43,
114,101,115,111,117,114,99,101,43,34,63,34,43,113,117,101,
114,121,41,59,13,10,13,10,32,32,32,32,32,32,119,115,
46,111,110,111,112,101,110,32,61,32,102,117,110,99,116,105,
111,110,40,41,123,125,59,13,10,32,32,32,32,32,32,119,
115,46,111,110,109,101,115,115,97,103,101,32,61,32,102,117,
110,99,116,105,111,110,32,40,101,118,116,41,13,10,32,32,
32,32,32,32,123,13,10,32,32,32,32,32,32,32,32,118,
97,114,32,114,101,99,101,105,118,101,100,95,109,115,103,32,
61,32,101,118,116,46,100,97,116,97,59,13,10,32,32,32,
32,32,32,32,32,47,47,32,118,97,114,32,99,117,114,114,
78,111,100,101,59,13,10,32,32,32,32,32,32,32,32,47,
47,32,114,101,99,101,105,118,101,100,95,109,115,103,46,114,
101,112,108,97,99,101,40,34,92,114,34,44,32,34,34,41,
59,13,10,32,32,32,32,32,32,32,32,98,82,101,99,111,
110,110,101,99,116,61,102,97,108,115,101,59,32,13,10,32,
32,32,32,32,32,32,32,47,47,32,99,111,110,115,111,108,
101,46,108,111,103,40,34,119,115,58,34,44,114,101,99,101,
105,118,101,100,95,109,115,103,41,59,13,10,32,32,32,32,
32,32,32,32,80,117,116,73,110,84,101,114,109,105,110,97,
108,40,114,101,99,101,105,118,101,100,95,109,115,103,41,59,
13,10,32,32,32,32,32,32,125,59,13,10,13,10,32,32,
32,32,32,32,119,115,46,111,110,99,108,111,115,101,32,61,
32,102,117,110,99,116,105,111,110,40,101,41,32,123,13,10,
32,32,32,32,32,32,32,32,99,111,110,115,111,108,101,46,
108,111,103,40,39,83,111,99,107,101,116,32,105,115,32,99,
108,111,115,101,100,46,32,82,101,99,111,110,110,101,99,116,
32,119,105,108,108,32,98,101,32,97,116,116,101,109,112,116,
101,100,32,105,110,32,49,32,115,101,99,111,110,100,46,39,
44,32,101,46,114,101,97,115,111,110,41,59,13,10,32,32,
32,32,32,32,32,32,100,101,108,101,116,101,32,119,115,59,
13,10,32,32,32,32,32,32,32,32,115,101,116,84,105,109,
101,111,117,116,40,102,117,110,99,116,105,111,110,40,41,32,
123,13,10,32,32,32,32,32,32,32,32,32,32,77,97,107,
101,68,97,116,97,83,111,99,107,101,116,40,39,115,116,100,
105,111,39,41,59,13,10,32,32,32,32,32,32,32,32,125,
44,32,49,48,48,48,41,59,13,10,32,32,32,32,32,32,
125,59,13,10,13,10,32,32,32,32,32,32,119,115,46,111,
110,101,114,114,111,114,32,61,32,102,117,110,99,116,105,111,
110,40,101,114,114,41,32,123,13,10,32,32,32,32,32,32,
32,32,99,111,110,115,111,108,101,46,101,114,114,111,114,40,
39,83,111,99,107,101,116,32,101,110,99,111,117,110,116,101,
114,101,100,32,101,114,114,111,114,58,32,39,44,32,101,114,
114,46,109,101,115,115,97,103,101,44,32,39,67,108,111,115,
105,110,103,32,115,111,99,107,101,116,39,41,59,13,10,32,
32,32,32,32,32,32,32,119,115,46,99,108,111,115,101,40,
41,59,13,10,32,32,32,32,32,32,125,59,13,10,32,32,
32,32,47,47,32,125,32,101,108,115,101,32,123,13,10,32,
32,32,32,32,32,47,47,32,99,111,110,115,111,108,101,46,
108,111,103,40,34,83,107,105,112,112,101,100,32,111,112,101,
110,63,34,41,59,13,10,32,32,32,32,47,47,32,125,13,
10,32,32,125,32,101,108,115,101,32,123,13,10,32,32,32,
32,47,47,32,84,104,101,32,98,114,111,119,115,101,114,32,
100,111,101,115,110,39,116,32,115,117,112,112,111,114,116,32,
87,101,98,83,111,99,107,101,116,13,10,32,32,32,32,97,
108,101,114,116,40,34,87,101,98,83,111,99,107,101,116,32,
78,79,84,32,115,117,112,112,111,114,116,101,100,32,98,121,
32,121,111,117,114,32,66,114,111,119,115,101,114,33,34,41,
59,13,10,32,32,125,13,10,125,13,10,13,10,102,117,110,
99,116,105,111,110,32,99,108,111,115,101,87,101,98,83,111,
99,107,101,116,40,41,13,10,123,13,10,32,32,105,102,32,
40,119,115,41,32,123,13,10,32,32,32,32,119,115,46,99,
108,111,115,101,40,41,59,13,10,32,32,125,13,10,125,13,
10,13,10,102,117,110,99,116,105,111,110,32,99,108,101,97,
114,76,111,103,40,41,13,10,123,13,10,32,32,118,97,114,
32,116,101,114,109,105,110,97,108,32,61,32,100,111,99,117,
109,101,110,116,46,103,101,116,69,108,101,109,101,110,116,66,
121,73,100,40,34,116,101,114,109,105,110,97,108,34,41,59,
13,10,32,32,116,101,114,109,105,110,97,108,46,118,97,108,
117,101,32,61,32,34,34,59,13,10,125,13,10,13,10,102,
117,110,99,116,105,111,110,32,67,104,101,99,107,115,111,99,
107,101,116,40,41,13,10,123,13,10,32,32,102,101,116,99,
104,40,39,86,97,108,105,100,87,83,46,106,115,111,110,39,
41,13,10,32,32,46,116,104,101,110,40,114,101,115,32,61,
62,32,114,101,115,46,106,115,111,110,40,41,41,13,10,32,
32,46,116,104,101,110,40,40,111,117,116,41,32,61,62,32,
123,13,10,32,32,32,32,105,102,40,111,117,116,46,86,97,
108,105,100,41,32,114,101,116,117,114,110,59,13,10,32,32,
32,32,105,102,32,40,119,115,41,32,13,10,32,32,32,32,
123,13,10,32,32,32,32,32,32,105,102,40,33,98,82,101,
99,111,110,110,101,99,116,41,80,117,116,73,110,84,101,114,
109,105,110,97,108,40,34,92,110,82,69,67,79,78,78,69,
67,84,73,78,71,92,110,34,41,59,13,10,32,32,32,32,
32,32,98,82,101,99,111,110,110,101,99,116,61,116,114,117,
101,59,13,10,32,32,32,32,125,13,10,32,32,32,32,77,
97,107,101,68,97,116,97,83,111,99,107,101,116,40,39,115,
116,100,105,111,39,41,59,13,10,32,32,125,41,46,99,97,
116,99,104,40,101,114,114,61,62,99,111,110,115,111,108,101,
46,108,111,103,40,101,114,114,41,41,59,13,10,125,13,10,
13,10,102,117,110,99,116,105,111,110,32,84,101,120,116,73,
110,112,117,116,40,41,13,10,123,13,10,32,32,118,97,114,
32,105,110,112,117,116,32,61,32,100,111,99,117,109,101,110,
116,46,103,101,116,69,108,101,109,101,110,116,66,121,73,100,
40,34,105,110,112,117,116,102,105,101,108,100,34,41,59,13,
10,32,32,118,97,114,32,100,97,116,97,32,61,32,105,110,
112,117,116,46,118,97,108,117,101,59,13,10,32,32,119,115,
46,115,101,110,100,40,100,97,116,97,41,59,13,10,32,32,
47,47,80,117,116,73,110,84,101,114,109,105,110,97,108,40,
100,97,116,97,41,59,32,47,47,32,108,111,99,97,108,32,
101,99,104,111,13,10,32,32,105,110,112,117,116,46,118,97,
108,117,101,32,61,32,34,34,59,13,10,125,13,10,13,10,
13,10,119,105,110,100,111,119,46,111,110,108,111,97,100,32,
61,32,102,117,110,99,116,105,111,110,40,41,32,13,10,123,
32,13,10,32,32,77,97,107,101,68,97,116,97,83,111,99,
107,101,116,40,39,115,116,100,105,111,39,41,59,13,10,32,
32,118,97,114,32,105,110,116,101,114,118,97,108,32,61,32,
115,101,116,73,110,116,101,114,118,97,108,40,67,104,101,99,
107,115,111,99,107,101,116,44,32,53,48,48,48,41,59,13,
10,125,59,13,10,60,47,115,99,114,105,112,116,62,13,10,
60,47,104,101,97,100,62,13,10,60,98,111,100,121,62,13,
10,60,100,105,118,32,105,100,61,34,115,115,101,34,62,13,
10,32,32,32,60,97,32,104,114,101,102,61,34,106,97,118,
97,115,99,114,105,112,116,58,99,108,101,97,114,76,111,103,
40,41,34,62,67,108,101,97,114,32,76,111,103,60,47,97,
62,13,10,60,47,100,105,118,62,13,10,60,100,105,118,32,
105,100,61,34,68,73,86,95,84,101,114,109,34,62,13,10,
60,116,101,120,116,97,114,101,97,32,110,97,109,101,61,34,
116,101,114,109,105,110,97,108,34,32,105,100,61,34,116,101,
114,109,105,110,97,108,34,32,114,111,119,115,61,34,51,48,
34,32,99,111,108,115,61,34,49,48,48,34,32,100,105,115,
97,98,108,101,100,32,62,60,47,116,101,120,116,97,114,101,
97,62,13,10,60,47,100,105,118,62,13,10,60,100,105,118,
62,13,10,60,116,101,120,116,97,114,101,97,32,110,97,109,
101,61,34,105,110,112,117,116,102,105,101,108,100,34,32,105,
100,61,34,105,110,112,117,116,102,105,101,108,100,34,32,114,
111,119,115,61,34,49,34,32,99,111,108,115,61,34,49,48,
48,34,32,111,110,105,110,112,117,116,61,34,84,101,120,116,
73,110,112,117,116,40,41,34,62,60,47,116,101,120,116,97,
114,101,97,62,13,10,60,47,100,105,118,62,13,10,60,47,
98,111,100,121,62,13,10,60,47,104,116,109,108,62,13,10 };

```
  - `nbrtos\source\console.html`
```
<!DOCTYPE HTML>
<html>
<head>
<script type="text/javascript">
var ws;
var MAX_TERMINAL_LEN = 2000;
var bReconnect=false;

function PutInTerminal(received_msg)
{
  var terminal = document.getElementById("terminal");
  var autoScroll = (terminal.scrollTop == (terminal.scrollHeight - terminal.clientHeight));
  var dataLen = received_msg.length;
  var terminalLen = terminal.value.length;
  // The following clause is to prevent browsers from crashing the page...
  if ((terminalLen + dataLen) > MAX_TERMINAL_LEN) {
    var delta = terminalLen + dataLen - MAX_TERMINAL_LEN;
    terminal.value = terminal.value.substring(delta, terminalLen);
  }
  terminal.value += received_msg;
  if (autoScroll) {
    terminal.scrollTop = terminal.scrollHeight - terminal.clientHeight;
  }
}

function MakeDataSocket(resource, query)
{
  if ("WebSocket" in window) {
    // if ((ws==null) || (ws.readyState>WebSocket.OPEN)) {
      // Let us open a web socket
      host = window.location.hostname;
      port = (window.location.port!='') ? (':'+window.location.port) : '';
      ws = new WebSocket("ws://"+host+port+"/"+resource+"?"+query);

      ws.onopen = function(){};
      ws.onmessage = function (evt)
      {
        var received_msg = evt.data;
        // var currNode;
        // received_msg.replace("\r", "");
        bReconnect=false; 
        // console.log("ws:",received_msg);
        PutInTerminal(received_msg);
      };

      ws.onclose = function(e) {
        console.log('Socket is closed. Reconnect will be attempted in 1 second.', e.reason);
        delete ws;
        setTimeout(function() {
          MakeDataSocket('stdio');
        }, 1000);
      };

      ws.onerror = function(err) {
        console.error('Socket encountered error: ', err.message, 'Closing socket');
        ws.close();
      };
    // } else {
      // console.log("Skipped open?");
    // }
  } else {
    // The browser doesn't support WebSocket
    alert("WebSocket NOT supported by your Browser!");
  }
}

function closeWebSocket()
{
  if (ws) {
    ws.close();
  }
}

function clearLog()
{
  var terminal = document.getElementById("terminal");
  terminal.value = "";
}

function Checksocket()
{
  fetch('ValidWS.json')
  .then(res => res.json())
  .then((out) => {
    if(out.Valid) return;
    if (ws) 
    {
      if(!bReconnect)PutInTerminal("\nRECONNECTING\n");
      bReconnect=true;
    }
    MakeDataSocket('stdio');
  }).catch(err=>console.log(err));
}

function TextInput()
{
  var input = document.getElementById("inputfield");
  var data = input.value;
  ws.send(data);
  //PutInTerminal(data); // local echo
  input.value = "";
}


window.onload = function() 
{ 
  MakeDataSocket('stdio');
  var interval = setInterval(Checksocket, 5000);
};
</script>
</head>
<body>
<div id="sse">
   <a href="javascript:clearLog()">Clear Log</a>
</div>
<div id="DIV_Term">
<textarea name="terminal" id="terminal" rows="30" cols="100" disabled ></textarea>
</div>
<div>
<textarea name="inputfield" id="inputfield" rows="1" cols="100" oninput="TextInput()"></textarea>
</div>
</body>
</html>

```

## Add remote console to project

If you prefer, you can copy `example-console/main.cpp` instead.

Add to the top of your project:

`#include <remoteconsole.h>`

Add to your UserMain function if not already:
```
  init();                                       // Initialize network stack
  // Start web server, default port 80, required for remote console
  StartHttp();
  WaitForActiveNetwork(TICKS_PER_SECOND * 5);   // Wait for DHCP address
  // Enable remote HTTP "serial" console. Not PW protected.
  EnableRemoteConsole();
```

- Clean NetBurner System Library (make -j20 clean-nblibs)
- Run As NetBurner Application
- Compile and browse to /console.html (all lowercase) on your NetBurner device

Any stdout like `getchar()`'s output or `printf()` will be displayed on the console.
Stdin is available via `charavail()` and `getchar()`.

### Adding custom I/O handling

This minimal program stores user input in a command buffer and returns data or
executes actions based on that input.

```
#include <init.h>
#include <nbrtos.h>
#include <iosys.h>
#include <nbstring.h>
#include <remoteconsole.h>
#include <hal.h>
#include <ShutDownNotifications.h>

#define SHUTDOWN_CUSTOM_REBOOT_REASON 100

void StoreCmdBuf(NBString &s, char c) {
  // 100-char command limit for safety
  if (s.length() < 100)
    s += c;
}
void ExecuteCmdBuf(NBString &s) {
  if (s == "time"){
    printf("> Uptime is %lu seconds.\r\n",Secs);
  } else if (s == "reboot") {
    if( NBApproveShutdown(SHUTDOWN_CUSTOM_REBOOT_REASON))
    {
      printf("> Rebooting.\r\n");
      OSTimeDly(TICKS_PER_SECOND * 5);
      ForceReboot();
    }
  } else {
    printf("\r\n> HELP:\r\n> (You typed [%s])\r\n", s.c_str());
    printf("> time: get time\r\n> reboot: reboot\r\n");
  }
}
void ClearCmdBuf(NBString &s) {
  s.clear();
}

void OutputTask(void * pd)
{
  while(1) {
    iprintf("> Tick at %lu\r\n",Secs);
    OSTimeDly(TICKS_PER_SECOND * 5); // delay 5 seconds
  }
}

/*-----------------------------------------------------------------------------
 * UserMain
 *----------------------------------------------------------------------------*/
void UserMain(void * pd)
{
  NBString cmdBuf; // store user-entered commands in a buffer

  init();                                       // Initialize network stack
  //Enable system diagnostics. Probably should remove for production code.
  EnableSystemDiagnostics();
  StartHttp();                                  // Start web server, default port 80
  WaitForActiveNetwork(TICKS_PER_SECOND * 5);   // Wait for DHCP address

  // Enable console.html piped to stdin/stdout
  EnableRemoteConsole();

  // Make a background task for outputting some text without blocking
  OSSimpleTaskCreatewName(OutputTask,20,"Output");

  // Main app loop
  while (1)
  {
    while(charavail())
    {
      char c=getchar(); // getchar will echo each char to stdout
      if(c=='\n') { // newline executes command and clears buffer
        ExecuteCmdBuf(cmdBuf);
        ClearCmdBuf(cmdBuf);
      } else { // all other chars add to buffer
        StoreCmdBuf(cmdBuf,c);
      }
    }
  }
}
```

### Changing the console HTML

Add this rule to `nbrtos\source\makefile`:
Make sure the second line is indented with a tab, not spaces.

```
CONSOLE_html.cpp: console.html
    compfile console.html console_html_data console_html_size console_html.cpp
```

Edit this line:

`CPP_SRC   += release_tagdata.cpp UI_html.cpp ROOT_html.cpp RAW_html.cpp LOGO_gif.cpp $(wildcard *.cpp)`

to be like this (indented with tabs not spaces)

`CPP_SRC   += release_tagdata.cpp UI_html.cpp ROOT_html.cpp RAW_html.cpp LOGO_gif.cpp CONSOLE_html.cpp $(wildcard *.cpp)`

**TODO:** The makefile sometimes doesn't execute quite right, so for now run `compfile console.html console_html_data console_html_size console_html.cpp` manually after making HTML changes. Try to keep the total HTML file size small to avoid storage issues.
