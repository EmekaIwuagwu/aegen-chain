add_library(aegen_util
    uint256.cpp
)

target_include_directories(aegen_util PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
)
