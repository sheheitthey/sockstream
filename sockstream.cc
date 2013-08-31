/* sockstream.cc
 * sockstream, an iostream for sockets.
 */
#include"sockstream.h"
using namespace std;

// This is 4K for read and 4K for write.
const int _buf_len = 8192;

sockstream::sockstream(bool create_socket)
:iostream(NULL), buf_(create_socket), buf_ptr_(NULL)
{
    init_buffer();
}

sockstream::sockstream(socket_t sock)
:iostream(NULL), buf_(sock), buf_ptr_(NULL)
{
    init_buffer();
}

sockstream::sockstream(const char *host, uint16_t port)
:iostream(NULL), buf_(true), buf_ptr_(NULL)
{
    connect(host, port);
    init_buffer();
}

sockstream::~sockstream()
{
    if(buf_ptr_)
        delete[] buf_ptr_;
}

void
sockstream::set_sock(socket_t sock)
{
    buf_.set_sock(sock);
}

socket_t
sockstream::sock() const
{
    return buf_.sock();
}

int
sockstream::connect(const sockaddr *addr, socklen_t addrlen)
{
    return buf_.connect(addr, addrlen);
}

int
sockstream::connect(const char *host, uint16_t port)
{
    int ret;
    sockaddr_in addr;
    if((ret = resolve(host, port, &addr) == 0))
        return connect((sockaddr*)&addr, sizeof(addr));
    return -1;
}

int
sockstream::resolve(const char *host, uint16_t port,
                    sockaddr_in *ret_addr)
{
    return sockstreambuf::resolve(host, port, ret_addr);
}

void
sockstream::put_eof()
{
    buf_.put_eof();
}

int
sockstream::close()
{
    return buf_.close();
}

void
sockstream::init_buffer()
{
    buf_ptr_ = new char[_buf_len];
    buf_.pubsetbuf(buf_ptr_, _buf_len);
    this->init(&buf_);
}
