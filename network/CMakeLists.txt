add_library(aegen_network
    peer.cpp
    gossip.cpp
    rpc_server.cpp
)

target_include_directories(aegen_network PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(aegen_network PUBLIC aegen_core aegen_rpc ws2_32)
