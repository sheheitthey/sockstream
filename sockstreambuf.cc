/* sockstreambuf.cc
 * sockstreambuf, a streambuf for sockets.
 */
#include"sockstreambuf.h"
#include<cstring>
#include<unistd.h>
using namespace std;

sockstreambuf::sockstreambuf(bool create_socket)
:sock_(_invalid_socket), send_eof_(false), send_done_(false),
 recv_done_(false), glen_(0), pbeg_(NULL)
{
    if(create_socket)
        sock_ = socket();
}

sockstreambuf::sockstreambuf(socket_t sock)
:sock_(sock), send_eof_(false), send_done_(false), recv_done_(false),
 glen_(0), pbeg_(NULL)
{
}

sockstreambuf::~sockstreambuf()
{
    if(sock_ != _invalid_socket)
        close();
}

void
sockstreambuf::set_sock(socket_t sock)
{
    sock_ = sock;
}

socket_t
sockstreambuf::sock() const
{
    return sock_;
}

int
sockstreambuf::resolve(const char *host, uint16_t port, sockaddr_in *ret_addr)
{
    const int service_len = 6;
    char service[service_len];
    struct addrinfo hints, *server;
    const struct sockaddr_in *addr;
    int ret;

    // prepare the arguments to getaddrinfo
    memset(&hints, sizeof(hints), 0);
    hints.ai_flags = AI_CANONNAME;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    snprintf(service, service_len, "%u", port);

    // call getaddrinfo
    ret = getaddrinfo(host, service, &hints, &server);
    if(ret != 0)
    {
        return ret;
    }

    // save the results
    addr = (const sockaddr_in*)server->ai_addr;
    if(ret_addr)
    {
        ret_addr->sin_family = AF_INET;
        ret_addr->sin_addr.s_addr = addr->sin_addr.s_addr;
        ret_addr->sin_port = addr->sin_port;
    }
    freeaddrinfo(server);
    return 0;
}

void
sockstreambuf::put_eof()
{
    send_eof_ = true;
    
    // Make it look like the buffer is full so subsequent puts must go through
    // overflow().
    setp(pptr(), pptr());

    if(pptr() == pbeg_)
        done_sending();
}

// Use the first half of the buffer for get and the second half for put.
sockstreambuf*
sockstreambuf::setbuf(char *s, streamsize n)
{
    char *gbeg, *gcur, *gend;
    char *pcur, *pend;

    gbeg = s;
    gcur = gbeg;
    gend = gbeg;
    glen_ = n/2;

    pbeg_ = gbeg + glen_;
    pcur = pbeg_;
    pend = s + n;
    
    setg(gbeg, gcur, gend);
    setp(pcur, pend);

    return this;
}

streampos
sockstreambuf::seekoff(streamoff off, ios_base::seekdir way,
                       ios_base::openmode which)
{
    if(way == ios_base::beg)
        return seekpos(off, which);
    else if(way == ios_base::cur)
    {
        // Return the get position if both are modified.
        if(which & ios_base::in)
        {
            if(which & ios_base::out)
                seekpos(pptr() - pbeg_ + off, ios_base::out);
            return seekpos(gptr() - eback() + off, ios_base::in);
        }
        else if(which & ios_base::out)
            return seekpos(pptr() - pbeg_ + off, ios_base::out);
    }
    else if(way == ios_base::end)
    {
        // Return the get position if both are modified.
        if(which & ios_base::in)
        {
            if(which & ios_base::out)
                seekpos(epptr() - pbeg_ + off, ios_base::out);
            return seekpos(egptr() - eback() + off, ios_base::in);
        }
        else if(which & ios_base::out)
            return seekpos(epptr() - pbeg_ + off, ios_base::out);
    }
    // If nothing happened
    return -1;
}

streampos
sockstreambuf::seekpos(streampos sp, ios_base::openmode which)
{
    streampos pos = -1;

    // Return get position if both are modified.
    if(which & ios_base::in)
    {
        if(which & ios_base::out)
        {
            pos = check_seekpos(sp, pbeg_, epptr());
            if(pos >= 0)
                setp(pptr()+pos, epptr());
        }
        pos = check_seekpos(sp, eback(), egptr());
        if(pos < 0)
            return pos;
        setg(eback(), gptr()+pos, egptr());
    }
    else if(which & ios_base::out)
    {
        pos = check_seekpos(sp, pbeg_, epptr());
        if(pos < 0)
            return pos;
        setp(pptr()+pos, epptr());
    }

    return pos;
}

// Flush the output buffer.
int
sockstreambuf::sync()
{
    while(pptr() != pbeg_)
    {
        if(overflow() == EOF)
            return -1;
    }
    return 0;
}

// We estimate that 0 characters are available if the buffer is empty.
streamsize
sockstreambuf::showmanyc()
{
    return 0;
}

// Override default behavior of repeatedly calling sbumpc, and transfer as
// large chunks as possible.
streamsize
sockstreambuf::xsgetn(char *s, streamsize n)
{
    int ret = 0;

    ret += get_chunk(&s, &n);
    while(n > 0)
    {
        if(underflow() == EOF)
            break;
        ret += get_chunk(&s, &n);
    }
    return ret;
}

// Receive as much into the buffer as possible.
int
sockstreambuf::underflow()
{
    int ret;
    int len = egptr() - gptr();

    // If for some reason underflow was called while the buffer was not empty,
    // shift buffer data over to have maximum room for recv().
    if(len > 0)
        memmove(eback(), gptr(), len);
    setg(eback(), eback(), eback() + len);

    if(recv_done_)
        return EOF;

    ret = recv(egptr(), glen_ - len);

    // error
    if(ret < 0)
    {
        done_recving();
        return EOF;
    }
    // EOF
    if(ret == 0)
    {
        done_recving();
        return EOF;
    }

    setg(eback(), gptr(), egptr() + ret);
    return (int)(*(gptr()));
}

// Use the streambuf default behavior, which is mostly a wrapper around
// underflow().
int
sockstreambuf::uflow()
{
    return streambuf::uflow();
}

// eback() never moves, so it's never possible to recover putback positions.
int
sockstreambuf::pbackfail(int c)
{
    return EOF;
}

streamsize
sockstreambuf::xsputn(const char *s, streamsize n)
{
    int ret = 0;

    ret += put_chunk(&s, &n);
    while(n > 0)
    {
        if(overflow() == EOF)
            break;
        ret += put_chunk(&s, &n);
    }
    return ret;
}

int
sockstreambuf::overflow(int c)
{
    int ret;
    int len = pptr() - pbeg_;
    ret = send(pbeg_, len);

    // error
    if(ret < 0)
    {
        // Make the buffer look full.
        setp(pptr(), pptr());
        done_sending();
        return EOF;
    }
    // sent 0, since for some reason pptr() == pbeg_
    if(ret == 0)
    {
        // if there is indeed no space left
        if(pptr() == epptr())
            return EOF;
    }
    else
    {
        memmove(pbeg_, pbeg_ + ret, len - ret);
        setp(pptr() - ret, epptr());
        if(pptr() == pbeg_ && send_eof_)
            done_sending();
    }

    if(c != EOF)
    {
        *(pptr()) = c;
        setp(pptr() + 1, epptr());
        return c;
    }
    return ret;
}

streampos
sockstreambuf::check_seekpos(streampos sp, char *beg, char *end)
{
    if(sp < 0 || sp > end-beg)
    {
        return -1;
    }
    return sp;
}

int
sockstreambuf::get_chunk(char **s, streamsize *n)
{
    int len = egptr() - gptr();
    if(len > *n)
        len = *n;
    memcpy(*s, egptr(), len);
    seekoff(len, ios_base::cur, ios_base::in);
    (*s) += len;
    (*n) -= len;
    return len;
}

int
sockstreambuf::put_chunk(const char **s, streamsize *n)
{
    int len = epptr() - pptr();
    if(len > *n)
        len = *n;
    memcpy(pptr(), *s, len);
    seekoff(len, ios_base::cur, ios_base::out);
    (*s) += len;
    (*n) -= len;
    return len;
}

int
sockstreambuf::done_sending()
{
    int ret;
    send_done_ = true;
    ret = shutdown();
    if(recv_done_)
        return close();
    return ret;
}

int
sockstreambuf::done_recving()
{
    recv_done_ = true;
    if(send_done_)
        return close();
    return 0;
}

int
sockstreambuf::socket()
{
    return ::socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
}

int
sockstreambuf::send(const void *buf, int len)
{
    return ::send(sock_, buf, len, 0);
}

int
sockstreambuf::recv(void *buf, int len)
{
    return ::recv(sock_, buf, len, 0);
}

int
sockstreambuf::shutdown()
{
    return ::shutdown(sock_, SHUT_WR);
}

int
sockstreambuf::connect(const sockaddr *addr, socklen_t addrlen)
{
    return ::connect(sock_, addr, addrlen);
}

int
sockstreambuf::close()
{
    return ::close(sock_);
}
