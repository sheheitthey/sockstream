/* example.cc
 * A simple echo client using sockstream.
 */
#include"sockstream.h"
#include<iostream>
#include<cstdlib>
#include<signal.h>
using namespace std;

void example(const char *host, uint16_t port)
{
    sockstream ss(host, port);
    string line;

    while(!cin.eof())
    {
        // get a line from stdin, then put it on the socket
        getline(cin, line);
        ss << line << endl;

        // get the line back from the socket, and put it on stdout
        getline(ss, line);
        cout << line << endl;
    }

    // send a FIN explicitly, although the destructor would call close() and
    // let the kernel take care of it anyway.
    ss.put_eof();
}

int
main(int argc, char **argv)
{
    const char *host;
    uint16_t port;
    // Ignore SIGPIPE so we don't crash.
    signal(SIGPIPE, SIG_IGN);
    if(argc != 3)
    {
        cout << "Usage: example [host] [port]\n" << endl;
        return 0;
    }

    host = argv[1];
    port = atoi(argv[2]);
    example(host, port);

    return 0;
}
