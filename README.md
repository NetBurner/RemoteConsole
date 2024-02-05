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

2. Add these lines to `nbrtos\source\library.mak`. Make sure the second line is indented with a tab, not spaces.

```
$(OBJDIR)/CONSOLE_html.cpp: $(subst $(NNDK_ROOT)/,,$(LIB_PATH))/console.html
	compfile $< CONSOLE_html_data CONSOLE_html_size $@

#...

LIB_CPP_SRC		+= $(addprefix $(OBJDIR)/, CONSOLE_html.cpp)
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

If you have trouble getting changes to `console.html` to take effect, just run `compfile console.html console_html_data console_html_size console_html.cpp` manually after making HTML changes. Try to keep the total HTML file size small to avoid storage issues.
