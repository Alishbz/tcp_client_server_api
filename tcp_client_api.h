/****************************************************************************************
* ALI SAHBAZ
*
*
* Client TCP Port Maneger
*
* Date          : 22.02.2024
* By            : Ali Sahbaz
* e-mail        : ali_sahbaz@outlook.com
*/
#pragma once
#include <iostream>
#include <functional>

#ifdef _WIN32
#include <WinSock2.h>
#include <WS2tcpip.h>
#if  defined(__x86_64__) || defined(__ppc64__)
#pragma comment(lib, "ws2_64.lib")
#else
#pragma comment(lib, "ws2_32.lib")
#endif
#elif __linux__
#warning "you must add linux socket lib"
#else
#warning "OS is not known"
#endif



class tcp_client_api {

public:
 
    tcp_client_api(ULONG ip, int port) : m_port(0), my_ip(0), m_conn_state(false), m_rcv_callback(nullptr), m_log_send_func(nullptr) , m_socket(NULL)
    {
        if (m_ll_connect(ip, port)) {
            m_conn_state = true;
            m_port = port;
            my_ip = ip;  
            m_log("CLIENT CONNECTED \n");
        }
    }

    tcp_client_api(std::string ip, int port) : m_port(0), my_ip(0), m_conn_state(false), m_rcv_callback(nullptr), m_log_send_func(nullptr), m_socket(NULL)
    {
        if (m_ll_connect(ip, port)) {
            m_conn_state = true;
            m_port = port;
            inet_pton(AF_INET, ip.c_str(), &my_ip);
            m_log("CLIENT CONNECTED \n");
        }
    }

    tcp_client_api() : m_port(0), my_ip(0), m_conn_state(false), m_rcv_callback(nullptr), m_log_send_func(nullptr), m_socket(NULL)
    {
    }
      
    ~tcp_client_api() {
        if (m_conn_state == true) {
            closesocket(m_socket);
            WSACleanup();
            m_conn_state == false;
            m_log("CLIENT DELETED \n");
        }
    }

    bool try_connection(ULONG ip, int port)
    {
        if (m_conn_state == false &&
            m_ll_connect(ip, port)) {
            m_conn_state = true;
            m_port = port;
            my_ip = ip;
            m_log("CLIENT CONNECTED \n");
            return true;
        }

        return false;;
    }

    bool try_connection(std::string ip, int port)
    {
        if (m_conn_state == false &&
            m_ll_connect(ip, port)) {
            m_conn_state = true;
            m_port = port;
            inet_pton(AF_INET, ip.c_str(), &my_ip);
            m_log("CLIENT CONNECTED \n");
            return true;
        }

        return false;;
    }
     
    template <typename Function>
    void logger_bind(Function&& function) {
        m_log_send_func = std::bind(std::forward<Function>(function), std::placeholders::_1, std::placeholders::_2);
    }

    /******************************/

    template <typename Function, typename Class>
    void bind(Function&& function, Class* handler) {
        m_rcv_callback = std::bind(std::forward<Function>(function), handler, std::placeholders::_1, std::placeholders::_2);
    }

    template <typename Function>
    void bind(Function&& function) {
        m_rcv_callback = std::bind(std::forward<Function>(function), std::placeholders::_1, std::placeholders::_2);
    }

    bool break_connection()
    {
        if (m_conn_state == true)
        {
            closesocket(m_socket);
            WSACleanup();
            m_conn_state = false;
            m_log("CLIENT DELETED \n");
            return true;
        }

        return false;
    }

    void transmit(const char* data, int size)
    {
        if (m_conn_state == true) {

            int status = send(m_socket, data, size, 0);

            if (status == SOCKET_ERROR) {

                int error = WSAGetLastError();

                if (error > 10000 - 1) //  WSAENOTSOCK 
                {
                    m_log("CLIENT BREAK x1005 \n");
                    m_conn_state == false;
                }
            }
        }
    }

    void transmit(std::string str)
    {
        const char* cstr = str.c_str();

        int length = static_cast<int>(str.length());

        if (m_conn_state == true) {

            int status = send(m_socket, cstr, length, 0);

            if (status == SOCKET_ERROR) {

                int error = WSAGetLastError();

                if (error > 10000 - 1) //  WSAENOTSOCK 
                {
                    m_log("CLIENT BREAK x1006 \n");
                    break_connection();
                }
            }
        }
    }

    bool operator()()const
    {
        return m_conn_state;
    }

    void run(void)    // polling - timer
    {
        constexpr int MAX_RECEIVE_SIZE = 1024 * 4;

        if (m_conn_state == true) {

            char buffer[MAX_RECEIVE_SIZE];

            int bytesRead = recv(m_socket, buffer, sizeof(buffer), 0);

            if (bytesRead > 0 && bytesRead < MAX_RECEIVE_SIZE - 1) {

                buffer[bytesRead++] = '\0';  // END with NULL

                if (m_rcv_callback) {
                    m_rcv_callback((const char*)buffer, bytesRead);
                }
            }
            else if (bytesRead == 0) {
                m_log("CLIENT BREAK x1006 \n");
                break_connection();
            }
        }
    }


private:

    std::function<void(const char*, int)> m_rcv_callback;

    SOCKET m_socket;

    int m_port;

    ULONG my_ip;

    bool m_conn_state;  /** connection state **/

    std::function<void(const char*, int)> m_log_send_func;  // optional
     
    bool m_ll_connect(ULONG ip, int port)
    {
        WSADATA m_wsa_data;

        if (WSAStartup(MAKEWORD(2, 2), &m_wsa_data) != 0) {
            m_log("CLIENT BREAK x2001 \n");
            return false;
        }

        m_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (m_socket == INVALID_SOCKET) {
            m_log("CLIENT BREAK x2002 \n");
            WSACleanup();
            return false;
        }

        sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(port);
        serverAddr.sin_addr.s_addr = ip;

        if (connect(m_socket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR) {
            m_log("CLIENT BREAK x2003 \n");
            closesocket(m_socket);
            WSACleanup();
            return false;
        }


        u_long mode = 1;  // 1: non-blocking, 0: blocking
        if (ioctlsocket(m_socket, FIONBIO, &mode) != 0) {
            m_log("CLIENT BREAK x2004 \n");
            closesocket(m_socket);
            WSACleanup();
            return false;
        }

        return true;
    }

    bool m_ll_connect(std::string ip, int port)
    {
        WSADATA m_wsa_data;

        if (WSAStartup(MAKEWORD(2, 2), &m_wsa_data) != 0) {
            m_log("CLIENT BREAK x2001 \n");
            return false;
        }

        m_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (m_socket == INVALID_SOCKET) {
            m_log("CLIENT BREAK x2002 \n");
            WSACleanup();
            return false;
        }

        sockaddr_in soc_cfg;
        soc_cfg.sin_family = AF_INET;
        soc_cfg.sin_port = htons(port);
        inet_pton(AF_INET, ip.c_str(), &soc_cfg.sin_addr);
      
        if (connect(m_socket, reinterpret_cast<sockaddr*>(&soc_cfg), sizeof(soc_cfg)) == SOCKET_ERROR) {
            m_log("CLIENT BREAK x2003 \n");
            closesocket(m_socket);
            WSACleanup();
            return false;
        }


        u_long mode = 1;  // 1: non-blocking, 0: blocking
        if (ioctlsocket(m_socket, FIONBIO, &mode) != 0) {
            m_log("CLIENT BREAK x2004 \n");
            closesocket(m_socket);
            WSACleanup();
            return false;
        }

        return true;
    }

    void m_log(std::string str) {
        if (m_log_send_func)
            m_log_send_func(str.c_str(), str.size());
    }
};


