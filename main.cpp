/****************************************************************************************
* ALI SAHBAZ   
* 
*
* Date          : 22.02.2024
* By            : Ali Şahbaz
* e-mail        : ali_sahbaz@outlook.com 
*/  
#include <iostream>
#include <memory>
#include <vector>
#include "mini_tdd.hpp"
  
#include "tcp_client_api.h"
#include "tcp_server_api.h"

#ifdef _WIN32
  #include <windows.h>  
#else
  #warning "OS is not known"
#endif 
        

static tcp_client_api * _g_client = nullptr;
static tcp_server_api * _g_server = nullptr;


TEST_CASE(client_connection_1)
{ 
      /** open fake server from other app like -> hercules/docklight **/

      tcp_client_api client("192.168.1.36", 84);

      ASSERT(client() == true); 
}

TEST_CASE(client_connection_2)
{
    /** open fake server from other app like -> hercules/docklight **/

    tcp_client_api client;

    auto logger_f = [](const char* str, int size) { 
        std::string str_formatted(str, size);
        std::cerr << str_formatted;
    };

    bool _test_stop = false;

    auto receiver_f = [&_test_stop](const char* str, int size) {
        std::string str_formatted(str, size);
        std::cerr << "client received: " << str_formatted << "\n";

        if (str_formatted.find("test_done") != std::string::npos) {
            /** if server send "test_done" message, than other tests going **/
            _test_stop = true;
        }
    };

    client.logger_bind(logger_f);

    client.bind(receiver_f);

    client.try_connection("192.168.1.36", 84);

    if (client()) {   /*** connection true ***/
    
        _g_client = &client;

        auto timer_callback = [](HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime) {
            _g_client->transmit("client sender test \n");
        };

        UINT_PTR timerID = SetTimer(NULL, 0, 1000, timer_callback);

        if (timerID == 0) {
            std::cerr << "timer callback error" << std::endl;
            return;
        }

        MSG msg = {0}; 
         
        while (GetMessage(&msg, NULL, 0, 0)) { 
            TranslateMessage(&msg); 
            DispatchMessage(&msg);
            client.run();
            if ( _test_stop) { 
                client.break_connection();
                break;  
            }
        } 

        KillTimer(NULL, timerID);
    } 

    ASSERT(client() == false);
}


TEST_CASE(server_connection_1)
{
    /** open fake clients from other app like -> hercules/docklight **/

    tcp_server_api server;

    auto logger_f = [](const char* str, int size) {
        std::string str_formatted(str, size);
        std::cerr << str_formatted;
    };

    bool _test_stop = false;

    auto receiver_f = [&_test_stop](const char* str, int size) {
        std::string str_formatted(str, size);
        std::cerr << "server received: " << str_formatted << "\n";

        if (str_formatted.find("test_done") != std::string::npos) {
            /** if server send "test_done" message, than other tests going **/
            _test_stop = true;
        }
    };

    server.logger_bind(logger_f);

    server.bind(receiver_f);

    server.open(83);

    if (server()) {   /*** connection true ***/

        _g_server = &server;

        auto timer_callback = [](HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime) {
            _g_server->transmit("server sender test \n");
        };

        UINT_PTR timerID = SetTimer(NULL, 0, 1000, timer_callback);

        if (timerID == 0) {
            std::cerr << "timer callback error" << std::endl;
            return;
        }

        MSG msg = { 0 };  

        while (GetMessage(&msg, NULL, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            server.run();
            if (_test_stop) {
                server.stop();
                break;
            }
        }

        KillTimer(NULL, timerID);
    }

    ASSERT(server() == false);
}




TEST_CASE(server_connection_2)
{
    /** open fake clients from other app like -> hercules/docklight **/

    tcp_server_api server;

    auto logger_f = [](const char* str, int size) {
        std::string str_formatted(str, size);
        std::cerr << str_formatted;
    };

    bool _test_stop = false;

    auto receiver_f = [&_test_stop](int i, const char* str, int size) {
        std::string str_formatted(str, size);
        std::cerr << i << "- server received: " << str_formatted << "\n";

        if (str_formatted.find("test_done") != std::string::npos) {
            /** if server send "test_done" message, than other tests going **/
            _test_stop = true;
        }
    };

    server.logger_bind(logger_f);

    server.bind_x(receiver_f);

    server.open(83);
     
    if (server()) {   /*** connection true ***/
          
        _g_server = &server;

        auto timer_callback = [](HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime) {
            _g_server->transmit(0 , "hi client 1   \n");
            _g_server->transmit(1,  "hi client 2   \n");
        };

        UINT_PTR timerID = SetTimer(NULL, 0, 1000, timer_callback);

        if (timerID == 0) {
            std::cerr << "timer callback error" << std::endl;
            return;
        }

        MSG msg = { 0 };

        while (GetMessage(&msg, NULL, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            server.run();
            if (_test_stop) {
                server.stop();
                break;
            }
        }

        KillTimer(NULL, timerID);
    }

    ASSERT(server() == false);
}


int main() { 
       
    std::cout << " tcp server/client test is starting ... \n" ;
     
    /** SERVICE TEST *************/
  
    RUN_TEST(server_connection_2);
    RUN_TEST(server_connection_1);

    RUN_TEST(client_connection_1);
    RUN_TEST(client_connection_2);
     
    /**************/
     
    return 0;
}


 

