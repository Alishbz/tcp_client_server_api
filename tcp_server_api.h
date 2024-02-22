/****************************************************************************************
* ALI SAHBAZ
*
*
* Server TCP Port Maneger
*
* Date          : 22.02.2024
* By            : Ali Sahbaz
* e-mail        : ali_sahbaz@outlook.com
*/
#pragma once
#include <iostream>
#include <functional>
#include <vector>

#ifdef _WIN32
#include <WinSock2.h>
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



class tcp_server_api {

public:

    tcp_server_api(ULONG ip, int port) : m_port(0), my_ip(0), m_conn_state(false), m_rcv_callback(nullptr), m_rcv_by_index_callback(nullptr), m_log_send_func(nullptr), m_socket(NULL)
    {
        if (m_ll_connect(ip, port)) {
            m_conn_state = true;
            m_port = port;
            my_ip = ip;
            m_log("SERVER OPENED \n");
        }
    }

    tcp_server_api(int port) : m_port(0), my_ip(0), m_conn_state(false), m_rcv_callback(nullptr), m_rcv_by_index_callback(nullptr), m_log_send_func(nullptr), m_socket(NULL)
    {
        if (m_ll_connect(0, port)) {
            m_conn_state = true;
            m_port = port;
            my_ip = 0;
            m_log("SERVER OPENED \n");
        }
    }

    tcp_server_api() : m_port(0), my_ip(0), m_conn_state(false), m_rcv_callback(nullptr), m_rcv_by_index_callback(nullptr), m_log_send_func(nullptr), m_socket(NULL)
    {
    }

    ~tcp_server_api() {
        if (m_conn_state == true) {
            closesocket(m_socket);
            WSACleanup();
            m_conn_state == false;
            m_log("SERVER DELETED \n");
        }
    }

    bool open(ULONG ip, int port)
    {
        if (m_conn_state == false &&
            m_ll_connect(ip, port)) {
            m_conn_state = true;
            m_port = port;
            my_ip = ip;
            m_log("SERVER OPENED \n");
            return true;
        }

        return false;
    }

    int client_count() const {
        return client_pool.size();
    }

    bool open(int port)
    {
        if (m_conn_state == false &&
            m_ll_connect(0, port)) {
            m_conn_state = true;
            m_port = port;
            my_ip = 0;
            m_log("SERVER OPENED \n");
            return true;
        }

        return false;
    }

    template <typename Function>
    void logger_bind(Function&& function) {
        m_log_send_func = std::bind(std::forward<Function>(function), std::placeholders::_1, std::placeholders::_2);
    }

    /** bind -> receive all clients , no index ****************************/

    template <typename Function, typename Class>
    void bind(Function&& function, Class* handler) {
        m_rcv_callback = std::bind(std::forward<Function>(function), handler, std::placeholders::_1, std::placeholders::_2);
    }

    template <typename Function>
    void bind(Function&& function) {
        m_rcv_callback = std::bind(std::forward<Function>(function), std::placeholders::_1, std::placeholders::_2);
    }

    /** bind -> receive with clients index ********************************/

    template <typename Function, typename Class>
    void bind_x(Function&& function, Class* handler) {
        m_rcv_by_index_callback = std::bind(std::forward<Function>(function), handler, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    }

    template <typename Function>
    void bind_x(Function&& function) {
        m_rcv_by_index_callback = std::bind(std::forward<Function>(function), std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    }

    bool stop()
    {
        if (m_conn_state == true)
        {
            closesocket(m_socket);
            WSACleanup();
            m_conn_state = false;
            m_log("SERVER DELETED \n");
            return true;
        }

        return false;
    }

    /** transmiting to all clients **/
    void transmit(const char* data, int size)
    {
        m_ll_transmit(SELECT_ALL_CLIENTS, data, size);
    }

    /** transmiting to specific client with index in vector table **/
    void transmit(int client_index, const char* data, int size)
    {
        m_ll_transmit(client_index, data, size);
    }

    /** transmiting to all clients **/
    void transmit(std::string str)
    {
        const char* cstr = str.c_str();

        int length = static_cast<int>(str.length());
         
        m_ll_transmit(SELECT_ALL_CLIENTS, cstr, length);
    }


    /** transmiting to specific client with index in vector table **/
    void transmit(int client_index, std::string str)
    {
        const char* cstr = str.c_str();

        int length = static_cast<int>(str.length());

        m_ll_transmit(client_index, cstr, length);
      
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

            int i = 0;

            for (auto& client : client_pool) {

                i++;

                int bytesRead = recv(client, buffer, sizeof(buffer), 0);

                if (bytesRead > 0 && bytesRead < MAX_RECEIVE_SIZE - 1) {

                    if (bytesRead < MAX_RECEIVE_SIZE - 1)
                        buffer[bytesRead++] = '\0';  // END with NULL

                    if (m_rcv_by_index_callback) {
                        m_rcv_by_index_callback(i, (const char*)buffer, bytesRead);
                    }
                    else if (m_rcv_callback) {
                        m_rcv_callback((const char*)buffer, bytesRead);
                    }
                }
                else if (bytesRead == 0) {
                    m_log("SERVER BREAK x5005 \n");
                    stop();
                }
            }

            m_ll_check_is_there_new_client();
        }
    }


private:

    /** common callback for all clients **/
    std::function<void(const char*, int)> m_rcv_callback;

    /** there is index for which client receivec data , priorty is higher then -> "m_rcv_callback" **/
    std::function<void(int, const char*, int)> m_rcv_by_index_callback;

    std::function<void(const char*, int)> m_log_send_func;  // optional

    SOCKET m_socket;

    int m_port;

    ULONG my_ip;

    bool m_conn_state;  /** connection state **/

    std::vector<SOCKET>   client_pool;

    bool m_ll_connect(ULONG ip, int port)
    {
        WSADATA m_wsa_data;

        if (WSAStartup(MAKEWORD(2, 2), &m_wsa_data) != 0) {
            m_log("SERVER BREAK x6001 \n");
            return false;
        }

        m_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (m_socket == INVALID_SOCKET) {
            m_log("SERVER BREAK x6002 \n");
            WSACleanup();
            return false;
        }

        sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(port);
        if (ip == 0) {
            serverAddr.sin_addr.s_addr = INADDR_ANY;
        }
        else {
            serverAddr.sin_addr.s_addr = ip;
        }

        if (::bind(m_socket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR) {
            m_log("SERVER BREAK x6003 \n");
            closesocket(m_socket);
            WSACleanup();
            return 1;
        }

        if (listen(m_socket, SOMAXCONN) == SOCKET_ERROR) {
            m_log("SERVER BREAK x6004 \n");
            closesocket(m_socket);
            WSACleanup();
            return false;
        }

        u_long mode = 1;  // 1: non-blocking, 0: blocking
        if (ioctlsocket(m_socket, FIONBIO, &mode) != 0) {
            m_log("SERVER BREAK x6005 \n");
            closesocket(m_socket);
            WSACleanup();
            return false;
        }

        return true;
    }

     

    void m_ll_check_is_there_new_client(void)
    {
        const int MAX_CLIENT_SUPPORT = SELECT_ALL_CLIENTS;

        while (client_pool.size() < MAX_CLIENT_SUPPORT) {
            SOCKET clientSocket = accept(m_socket, NULL, NULL);
            if (clientSocket == INVALID_SOCKET) {
                return;
            }
            client_pool.push_back(clientSocket);
            send(clientSocket, (const char*)"asos - connection done \n ", 24, 0);
        }
    }


    void m_log(std::string str) {
        if (m_log_send_func)
            m_log_send_func(str.c_str(), str.size());
    }

    const int SELECT_ALL_CLIENTS = 10;

    /** client_index: SELECT_ALL_CLIENTS  ***/
    void m_ll_transmit(int client_index, const char* cstr, int length)
    {  
        if (m_conn_state == true) {

            int i = 0;

            for (auto& client : client_pool) {

                if (client_index =! SELECT_ALL_CLIENTS && i == client_index)
                {
                    int status = send(client, cstr, length, 0);

                    if (status == SOCKET_ERROR) {

                        int error = WSAGetLastError();

                        if (error > 10000 - 1) //  WSAENOTSOCK 
                        {
                            m_log("SERVER'S client connection down x5003 \n");

                            auto it = std::find(client_pool.begin(), client_pool.end(), client);
                            if (it != client_pool.end()) {
                                client_pool.erase(it);
                            }
                            closesocket(client);

                            // stop();
                        }
                    }
                }

                i++;
            }
        }
    }
};


