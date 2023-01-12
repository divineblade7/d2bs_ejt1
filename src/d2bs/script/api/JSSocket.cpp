#include "d2bs/script/api/JSSocket.h"

#include "d2bs/utils/Helpers.h"
#include "d2bs/variables.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

struct SocketData {
  int mode;
  SOCKET socket;
};
EMPTY_CTOR(socket)

JSAPI_PROP(socket_getProperty) {
  SocketData* sdata = (SocketData*)JS_GetInstancePrivate(cx, obj, &socket_class, NULL);

  if (sdata) {
    JS::Value ID;
    JS_IdToValue(cx, id, &ID);
    JS_BeginRequest(cx);
    switch (JSVAL_TO_INT(ID)) {
      struct timeval timeout;
      int result;
      case SOCKET_READABLE:
        fd_set read_set;
        timeout.tv_sec = 0;
        timeout.tv_usec = 100;  // 100 ms
        FD_ZERO(&read_set);
        FD_SET(sdata->socket, &read_set);

        result = select(1, &read_set, NULL, NULL, &timeout);
        vp.setInt32(result);
        break;

      case SOCKET_WRITEABLE:
        fd_set write_set;

        timeout.tv_sec = 0;
        timeout.tv_usec = 100;  // 100 ms
        FD_ZERO(&write_set);
        FD_SET(sdata->socket, &write_set);

        result = select(1, NULL, &write_set, NULL, &timeout);

        vp.setInt32(result);
        break;
    }
  }

  return JS_TRUE;
}

JSAPI_STRICT_PROP(socket_setProperty) {
  return JS_TRUE;
}

JSAPI_FUNC(socket_open) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  if (args.length() < 2) {
    args.rval().setBoolean(false);
    return JS_TRUE;
  }
  char* hostName = NULL;
  int32_t port = 0;
  if (args[0].isString()) hostName = JS_EncodeStringToUTF8(cx, args[0].toString());

  if (!strstr(Vars.settings.szHosts, hostName)) THROW_ERROR(cx, "Invalid hostname specified");

  if (JS_ValueToInt32(cx, args[1], &port) == JS_FALSE) THROW_ERROR(cx, "Could not convert parameter 2");

  WSADATA wsaData;

  if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
    return JS_TRUE;
  }

  SocketData* Sdata = new SocketData;

  Sdata->socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

  struct hostent* host;

// warning C4996: 'gethostbyname': Use getaddrinfo() or GetAddrInfoW() instead or define _WINSOCK_DEPRECATED_NO_WARNINGS
// to disable deprecated API warnings
#pragma warning(push)
#pragma warning(disable : 4996)
  host = gethostbyname(hostName);
#pragma warning(pop)
  JS_free(cx, hostName);
  if (host == NULL) THROW_ERROR(cx, "Cannot find host");
  SOCKADDR_IN SockAddr;
  SockAddr.sin_port = htons(static_cast<u_short>(port));
  SockAddr.sin_family = AF_INET;
  SockAddr.sin_addr.s_addr = *((unsigned long*)host->h_addr);
  Sdata->mode = Sdata->socket;

  if (connect(Sdata->socket, (SOCKADDR*)(&SockAddr), sizeof(SockAddr)) != 0) {
    THROW_ERROR(cx, "Failed to connect");
  }

  JSObject* res = BuildObject(cx, &socket_class, socket_methods, socket_props, Sdata);
  if (!res) {
    closesocket(Sdata->socket);
    WSACleanup();
    THROW_ERROR(cx, "Failed to define the socket object");
  }

  args.rval().setObjectOrNull(res);
  return JS_TRUE;
}

JSAPI_FUNC(socket_close) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  auto self = args.thisv().toObjectOrNull();
  SocketData* sData = (SocketData*)JS_GetInstancePrivate(cx, self, &socket_class, NULL);

  closesocket(sData->socket);
  WSACleanup();

  return JS_TRUE;
}

JSAPI_FUNC(socket_send) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  auto self = args.thisv().toObjectOrNull();
  SocketData* sData = (SocketData*)JS_GetInstancePrivate(cx, self, &socket_class, NULL);
  char* msg = NULL;

  if (args.get(0).isString()) msg = JS_EncodeStringToUTF8(cx, args[0].toString());

  send(sData->socket, msg, strlen(msg), 0);

  return JS_TRUE;
}

JSAPI_FUNC(socket_read) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  auto self = args.thisv().toObjectOrNull();
  SocketData* sData = (SocketData*)JS_GetInstancePrivate(cx, self, &socket_class, NULL);

  char buffer[10000] = {0};
  std::string returnVal;

  int iResult = 0;
  do {
    iResult = recv(sData->socket, buffer, 10000, 0);
    if (iResult > 0) returnVal.append(buffer);
    if (iResult == -1) {
      THROW_ERROR(cx, "Socket Error");
    }

  } while (iResult > 10000);

  args.rval().setString(JS_InternString(cx, returnVal.c_str()));
  return JS_TRUE;
}

void socket_finalize(JSFreeOp*, JSObject* obj) {
  SocketData* sData = (SocketData*)JS_GetPrivate(obj);
  if (sData) {
    closesocket(sData->socket);
    WSACleanup();
  }
}
