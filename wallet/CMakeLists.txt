add_library(aegen_wallet
    keypair.cpp
    address.cpp
    signer.cpp
    mnemonic.cpp
)

target_include_directories(aegen_wallet PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR} # To allow including core/types.h
)
