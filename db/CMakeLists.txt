add_library(aegen_db
    rocksdb_wrapper.cpp
    state_manager.cpp
)

target_include_directories(aegen_db PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
)
