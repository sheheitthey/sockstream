/* sockstream.h
 * sockstream, an iostream for sockets.
 */
#ifndef __SOCKSTREAM_H
#define __SOCKSTREAM_H

#include<iostream>
#include"sockstreambuf.h"

class sockstream:public std::iostream
{
public:
    sockstream(bool create_socket = true);
    sockstream(socket_t sock);
    sockstream(const char *host, uint16_t port);
    ~sockstream();

    void set_sock(socket_t sock);
    socket_t sock() const;

    int connect(const sockaddr *addr, socklen_t addrlen);
    int connect(const char *host, uint16_t port);

    static int resolve(const char *host, uint16_t port,
                       sockaddr_in *ret_addr);

    void put_eof();
    int close();
private:
    sockstreambuf buf_;
    char *buf_ptr_;

    void init_buffer();
};

#endif
