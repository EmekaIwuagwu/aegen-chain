add_library(aegen_consensus
    pbft.cpp
    leader.cpp
    validator.cpp
)
target_include_directories(aegen_consensus PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
)
target_link_libraries(aegen_consensus PUBLIC aegen_core aegen_exec aegen_wallet)
