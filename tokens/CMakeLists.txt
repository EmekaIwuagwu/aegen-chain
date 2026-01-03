add_library(aegen_tokens
    token_manager.cpp
    token_registry.cpp
    token_transfer.cpp
)

target_include_directories(aegen_tokens PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
)
target_link_libraries(aegen_tokens PUBLIC aegen_core)
