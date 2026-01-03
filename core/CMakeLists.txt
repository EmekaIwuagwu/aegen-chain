add_library(aegen_core
    transaction.cpp
    block.cpp
    account.cpp
    merkle.cpp
    mempool.cpp
    ../util/crypto.cpp 
)

target_include_directories(aegen_core PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR} # to find util/crypto.h
)
target_link_libraries(aegen_core PUBLIC aegen_wallet)
