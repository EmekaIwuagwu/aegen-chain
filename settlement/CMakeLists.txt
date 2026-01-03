add_library(aegen_settlement
    bridge.cpp
    batch.cpp
    kadena_client.cpp
)

target_include_directories(aegen_settlement PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
)

# Link WinHTTP for HTTPS requests
target_link_libraries(aegen_settlement PUBLIC aegen_core ws2_32 winhttp)
