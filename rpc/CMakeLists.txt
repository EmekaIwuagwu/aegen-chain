add_library(aegen_rpc
    endpoints.cpp
    handler.cpp
)

target_include_directories(aegen_rpc PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
)
target_link_libraries(aegen_rpc PUBLIC aegen_core aegen_network aegen_db aegen_exec aegen_wallet)
