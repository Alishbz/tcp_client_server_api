/****************************************************************************************
* ALI SAHBAZ
*
*
* Date          : 20.02.2024
* By            : Ali Þahbaz
* e-mail        : ali_sahbaz@outlook.com
*/
#pragma once 

#define TEST_CASE(name) void name()
 
#define ASSERT(expression) \
    if (!(expression)) { \
        std::cerr << "Assertion failed: " << #expression << " in " << __FILE__ << " at line " << __LINE__ << std::endl; \
        return; \
    }
 

#define RUN_TEST(test) \
    std::cout << "Running test: " << #test << std::endl; \
    test(); \
    std::cout << #test << " passed!" << std::endl << std::endl;