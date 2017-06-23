/*
 *   C++ sockets on Unix and Windows
 *   Copyright (C) 2002
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
 * Copied from: https://cs.baylor.edu/~donahoo/practical/CSockets/practical/
 */

#ifndef __PRACTICALSOCKET_INCLUDED__
#define __PRACTICALSOCKET_INCLUDED__

#include <exception>  // For exception class
#include <string>     // For string

using namespace std;

class SocketException : public exception {
   public:
    SocketException(const string &message, bool inclSysMsg = false) throw();

    ~SocketException() throw();

    const char *what() const throw();

   private:
    string userMessage;  // Exception message
};

class Socket {
   public:
    ~Socket();

    string getLocalAddress() throw(SocketException);
    unsigned short getLocalPort() throw(SocketException);
    void setLocalPort(unsigned short localPort) throw(SocketException);
    void setLocalAddressAndPort(
        const string &localAddress,
        unsigned short localPort = 0) throw(SocketException);

    static void cleanUp() throw(SocketException);

    static unsigned short resolveService(const string &service,
                                         const string &protocol = "tcp");

   private:
    // Prevent the user from trying to use value semantics on this object
    Socket(const Socket &sock);
    void operator=(const Socket &sock);

   protected:
    int sockDesc;  // Socket descriptor
    Socket(int type, int protocol) throw(SocketException);
    Socket(int sockDesc);
};

class CommunicatingSocket : public Socket {
   public:
    void connect(const string &foreignAddress,
                 unsigned short foreignPort) throw(SocketException);

    void send(const void *buffer, int bufferLen) throw(SocketException);
    int recv(void *buffer, int bufferLen) throw(SocketException);

    string getForeignAddress() throw(SocketException);

    unsigned short getForeignPort() throw(SocketException);

   protected:
    CommunicatingSocket(int type, int protocol) throw(SocketException);
    CommunicatingSocket(int newConnSD);
};

class TCPSocket : public CommunicatingSocket {
   public:
    TCPSocket() throw(SocketException);
    TCPSocket(const string &foreignAddress,
              unsigned short foreignPort) throw(SocketException);

   private:
    // Access for TCPServerSocket::accept() connection creation
    friend class TCPServerSocket;
    TCPSocket(int newConnSD);
};

class TCPServerSocket : public Socket {
   public:
    TCPServerSocket(unsigned short localPort,
                    int queueLen = 5) throw(SocketException);

    TCPServerSocket(const string &localAddress, unsigned short localPort,
                    int queueLen = 5) throw(SocketException);

    TCPSocket *accept() throw(SocketException);

   private:
    void setListen(int queueLen) throw(SocketException);
};

#endif
