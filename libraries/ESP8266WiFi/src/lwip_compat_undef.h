/*
 * lwip_compat_undef.h — neutralize lwIP's LWIP_COMPAT_SOCKETS macros.
 *
 * lwIP builds with LWIP_COMPAT_SOCKETS==1 (and LWIP_POSIX_SOCKETS_IO_NAMES==1),
 * which turns the POSIX socket names (socket/connect/bind/listen/accept/read/
 * write/close/send/recv/select/ioctl/setsockopt/... plus gethostbyname) into
 * function-like macros that alias the lwip_* functions. Those collide with the
 * method names of the Arduino Client/Server/UDP classes (WiFiClient::connect,
 * WiFiServer::accept, ...), so every one of them is #undef'd here.
 *
 * Any TU that touches the socket backend must include this header BEFORE
 * declaring or defining anything that uses those names, and must call the
 * explicit `lwip_*` functions instead of the POSIX names.
 *
 * Safe to include at any point: the #ifdef guards make it a no-op before
 * lwip/sockets.h has defined them.
 */
#ifndef LWIP_COMPAT_UNDEF_H
#define LWIP_COMPAT_UNDEF_H

#ifdef accept
#undef accept
#endif
#ifdef bind
#undef bind
#endif
#ifdef shutdown
#undef shutdown
#endif
#ifdef getpeername
#undef getpeername
#endif
#ifdef getsockname
#undef getsockname
#endif
#ifdef setsockopt
#undef setsockopt
#endif
#ifdef getsockopt
#undef getsockopt
#endif
#ifdef closesocket
#undef closesocket
#endif
#ifdef connect
#undef connect
#endif
#ifdef listen
#undef listen
#endif
#ifdef recv
#undef recv
#endif
#ifdef recvmsg
#undef recvmsg
#endif
#ifdef recvfrom
#undef recvfrom
#endif
#ifdef send
#undef send
#endif
#ifdef sendmsg
#undef sendmsg
#endif
#ifdef sendto
#undef sendto
#endif
#ifdef socket
#undef socket
#endif
#ifdef select
#undef select
#endif
#ifdef poll
#undef poll
#endif
#ifdef ioctlsocket
#undef ioctlsocket
#endif
#ifdef inet_ntop
#undef inet_ntop
#endif
#ifdef inet_pton
#undef inet_pton
#endif
#ifdef read
#undef read
#endif
#ifdef readv
#undef readv
#endif
#ifdef write
#undef write
#endif
#ifdef writev
#undef writev
#endif
#ifdef close
#undef close
#endif
#ifdef fcntl
#undef fcntl
#endif
#ifdef ioctl
#undef ioctl
#endif
#ifdef gethostbyname
#undef gethostbyname
#endif
#ifdef gethostbyname_r
#undef gethostbyname_r
#endif
#ifdef getaddrinfo
#undef getaddrinfo
#endif
#ifdef freeaddrinfo
#undef freeaddrinfo
#endif

/* INADDR_ANY and friends: the SDK's inet.h aliases these to IPADDR_*, but the
 * reference ESP8266 core instead defines them as `const IPAddress` globals
 * (see cores/arduino/IPAddress.{h,cpp}), and ESP8266 code writes
 * `IPAddress x = INADDR_ANY;`. Undef so the object wins, as on ESP8266. */
#ifdef INADDR_ANY
#undef INADDR_ANY
#endif
#ifdef INADDR_NONE
#undef INADDR_NONE
#endif
#ifdef INADDR_LOOPBACK
#undef INADDR_LOOPBACK
#endif
#ifdef INADDR_BROADCAST
#undef INADDR_BROADCAST
#endif

#endif /* LWIP_COMPAT_UNDEF_H */
