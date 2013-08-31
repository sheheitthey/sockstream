/* sockstreambuf.h
 * sockstreambuf, a streambuf for sockets.
 */
#ifndef __SOCKSTREAMBUF_H
#define __SOCKSTREAMBUF_H

#include<streambuf>
#include<cstdio>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<netdb.h>

typedef int socket_t;
const socket_t _invalid_socket = -1;

class sockstreambuf:public std::streambuf
{
public:
    sockstreambuf(bool create_socket = true);
    sockstreambuf(socket_t sock);
    ~sockstreambuf();

    void set_sock(socket_t sock);
    socket_t sock() const;

    static int resolve(const char *host, uint16_t port,
                       sockaddr_in *ret_addr);

    void put_eof();
protected:
    // Buffer management and positioning
    virtual sockstreambuf* setbuf(char *s, std::streamsize n);
    virtual std::streampos
    seekoff(std::streamoff off, std::ios_base::seekdir way,
            std::ios_base::openmode which = std::ios_base::in |
                                            std::ios_base::out);
    virtual std::streampos
    seekpos(std::streampos sp,
            std::ios_base::openmode which = std::ios_base::in |
                                            std::ios_base::out);
    virtual int sync();

    // Input functions
    virtual std::streamsize showmanyc();
    virtual std::streamsize xsgetn(char *s, std::streamsize n);
    virtual int underflow();
    virtual int uflow();
    virtual int pbackfail(int c = EOF);

    // Output functions
    virtual std::streamsize xsputn(const char *s, std::streamsize n);
    virtual int overflow(int c = EOF);

private:
    socket_t sock_;
    bool send_eof_;
    bool send_done_;
    bool recv_done_;
    std::streamsize glen_;
    char *pbeg_;

    std::streampos check_seekpos(std::streampos sp, char *beg, char *end);

    int get_chunk(char **s, std::streamsize *n);
    int put_chunk(const char **s, std::streamsize *n);

    int done_sending();
    int done_recving();

    // Wrappers around socket calls.
    static socket_t socket();
    int send(const void *buf, int len);
    int recv(void *buf, int len);
    int shutdown();
public:
    int connect(const sockaddr *addr, socklen_t addrlen);
    int close();
};

#endif
