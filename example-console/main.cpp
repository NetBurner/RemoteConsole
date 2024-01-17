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
		char c=getchar(); // getchar will block and also echo each char to stdout
		if(c=='\n') { // newline executes command and clears buffer
			ExecuteCmdBuf(cmdBuf);
			ClearCmdBuf(cmdBuf);
		} else { // all other chars add to buffer
			StoreCmdBuf(cmdBuf,c);
		}
	}
}
