add_library(aegen_exec
    execution_engine.cpp
    vm.cpp
)

target_include_directories(aegen_exec PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
)
target_link_libraries(aegen_exec PUBLIC aegen_core aegen_db aegen_tokens)
