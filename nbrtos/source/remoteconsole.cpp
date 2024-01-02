/* Revision: 3.4.0 */

/******************************************************************************
* Copyright 1998-2024 NetBurner, Inc.  ALL RIGHTS RESERVED
*
*    Permission is hereby granted to purchasers of NetBurner Hardware to use or
*    modify this computer program for any use as long as the resultant program
*    is only executed on NetBurner provided hardware.
*
*    No other rights to use this program or its derivatives in part or in
*    whole are granted.
*
*    It may be possible to license this or other NetBurner software for use on
*    non-NetBurner Hardware. Contact sales@Netburner.com for more information.
*
*    NetBurner makes no representation or warranties with respect to the
*    performance of this computer program, and specifically disclaims any
*    responsibility for any damages, special or consequential, connected with
*    the use of this program.
*
* NetBurner
* 16855 W Bernardo Dr
* San Diego, CA 92127
* www.netburner.com
******************************************************************************/

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
{
  int rv=0;

  if (
    (!dataavail(OldStdio[0])) && 
    ((ws_fd<0) || (!dataavail(ws_fd)))
  ) {
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
  if(fd==ws_fd) {
    switch(ct) {
      case eReadSet: break;
      case eWriteSet: break;
      case eErrorSet: {
        int ows=ws_fd;
        ws_fd=-1;
        close(ows);
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
