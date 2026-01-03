add_library(aegen_settlement
    bridge.cpp
    batch.cpp
    kadena_client.cpp
)

target_include_directories(aegen_settlement PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
)

target_link_libraries(aegen_settlement PUBLIC aegen_core)

# Windows-specific HTTP library
if(WIN32)
    target_link_libraries(aegen_settlement PUBLIC ws2_32 winhttp)
endif()

# Linux: For production with real HTTPS, add libcurl
# find_package(CURL)
# if(CURL_FOUND)
#     target_link_libraries(aegen_settlement PUBLIC ${CURL_LIBRARIES})
#     target_include_directories(aegen_settlement PRIVATE ${CURL_INCLUDE_DIRS})
#     target_compile_definitions(aegen_settlement PRIVATE USE_CURL=1)
# endif()
