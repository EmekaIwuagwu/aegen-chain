cmake_minimum_required(VERSION 3.20)
project(aegen-blockchain CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
    ${CMAKE_SOURCE_DIR}/wallet
    ${CMAKE_SOURCE_DIR}/tokens
    ${CMAKE_SOURCE_DIR}/consensus
    ${CMAKE_SOURCE_DIR}/db
    ${CMAKE_SOURCE_DIR}/exec
    ${CMAKE_SOURCE_DIR}/network
    ${CMAKE_SOURCE_DIR}/rpc
    ${CMAKE_SOURCE_DIR}/settlement
    ${CMAKE_SOURCE_DIR}/proofs
    ${CMAKE_SOURCE_DIR}/da
    ${CMAKE_SOURCE_DIR}/util
)

# Core subsystems
add_subdirectory(core)
add_subdirectory(wallet)
add_subdirectory(tokens)
add_subdirectory(db)
add_subdirectory(exec)
add_subdirectory(consensus)
add_subdirectory(rpc)
add_subdirectory(network)
add_subdirectory(settlement)
add_subdirectory(proofs)
add_subdirectory(da)
add_subdirectory(tests)

# Main Executable
add_executable(aegen-node main.cpp)
# Link all modules
target_link_libraries(aegen-node PRIVATE 
    aegen_network
    aegen_rpc
    aegen_consensus
    aegen_exec 
    aegen_tokens 
    aegen_core 
    aegen_db 
    aegen_wallet
    aegen_settlement # Added settlement linkage
)
