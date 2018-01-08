#include "network/socket_library.h"
#include "network/socket.h"
#include "network/tcp_server_reactor.h"
#include <cstring>
#include <iostream>
#include <sstream>
#include <mutex>
using namespace std;

#define IP_ADDRESS "127.0.0.1"
#define PORT 666

#define BENCHMARK

class TCPServerReactorTest : public TCPServerReactor
{
    private :
        int m_clientCounter;
        mutex m_clientCounterMutex;
    public :

        TCPServerReactorTest() : m_clientCounter{0}
        {
        }

        virtual void onUnhandledSocketError(int errorCode) override
        {
#ifdef _WIN32
            if (errorCode != 10022)
#endif
            cout << "Unhandled socket error occured : " << errorCode << endl;
        }

        virtual void onClientConnected(size_t peerIndex) override
        {
#ifndef BENCHMARK
            m_clientCounterMutex.lock();
            m_clientCounter++;
            m_clientCounterMutex.unlock();
            cout << "Client" << peerIndex << " connected : " << m_clientCounter << "TH CONNECTION" << endl;
#endif
            TCPServerReactor::onClientConnected(peerIndex);
        }

        virtual void onClientReady(size_t peerIndex) override
        {
#ifndef BENCHMARK
            cout << "Client" << peerIndex << " event ready" << endl;
#endif
            auto peerSocket = getPeerSocket(peerIndex);
            char buffer[33];
            std::memset(buffer, 0, 33);
#if _WIN32
            auto read = peerSocket->receive(buffer, 32, 10);
#elif __linux__
            auto read = peerSocket->receive(buffer, 32);
#endif
            auto error = Socket::getCurrentThreadLastSocketError();

            if ( read > 0)
            {

                buffer[read] = '\0';
#ifndef BENCHMARK
                cout << endl << "Received from client " << peerIndex << " : " << buffer << endl;
#endif

                stringstream execReport;
                execReport << "Exec report ";
                peerSocket->send(execReport.str());

                if (!strcmp(buffer, "quit"))
                {
                    onClientDisconnected(peerIndex);
#ifndef BENCHMARK
                    cout << "Client" << peerIndex << " disconnected" << endl;
#endif
                }
            }
            else
            {
                if (peerSocket->isConnectionLost(error, read))
                {
#ifndef BENCHMARK
                    cout << "Client" << peerIndex << " disconnected" << endl;
#endif
                    onClientDisconnected(peerIndex);
                }
                else if (error != 0)
                {
                    onUnhandledSocketError(error);
                }
            }
        }
};

int main()
{
    SocketLibrary::initialise();

    TCPServerReactorTest server;
    server.setPollTimeout(600000);
    server.setMaxPollEvents(10240);
    server.start(IP_ADDRESS, PORT);

    while (true)
    {
        std::string value;
        std::cin >> value;

        if (value == "quit")
        {
            break;
        }
    }
    server.stop();
    SocketLibrary::uninitialise();
    return 0;
}