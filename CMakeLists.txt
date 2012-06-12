cmake_minimum_required (VERSION 2.6.2)
project (test)

set (SRC
    src/test/Basic.cpp
    src/udp_network_Network.cpp
    src/udp_network_Connection.cpp
    src/udp_network_Packet.cpp
)

find_library (BOOST_SYSTEM_LIBRARY file boost_system)
find_library (PTHREAD_LIBRARY file pthread)

add_definitions (-std=c++0x -Wall)
add_executable (test ${SRC})
target_link_libraries (test
    ${PTHREAD_LIBRARY}
    ${BOOST_SYSTEM_LIBRARY})
