#ifndef _CONNECTION_h
#define _CONNECTION_h

extern void wifiConnect();
extern void wifiCheck();

extern void mdnsInit();
extern void mdnsUpdate();

extern void webServerInit();
extern void webServerHandle();

extern void webSocketInit();
extern void webSocketHandle();

#endif