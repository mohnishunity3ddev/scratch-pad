#include "ws.h"
#include <stdio.h>

#ifdef WIN32
int
ws_startup()
{
    // The WSADATA structure contains information about the Windows Sockets implementation. The MAKEWORD(2,2)
    // parameter of WSAStartup makes a request for version 2.2 of Winsock on the system, and sets the passed
    // version as the highest version of Windows Sockets support that the caller can use.
    WSADATA wsa_data;
    int result;

    // The WSAStartup function is called to initiate use of WS2_32.dll.
    result = WSAStartup(MAKEWORD(2, 2), &wsa_data);
    if (result != 0) {
        printf("WSAStartup failed!: %d.\n", result);
        return 1;
    }

    WSACleanup();

    return 0;
}
#endif
