cmake_minimum_required(VERSION 3.29)
project(vim-viperos C)

set(SOURCES
        src/main.c
        src/tui.c
        src/filebuf.c

)

set(HEADERS
        include/tui.h
        include/filebuf.h

)

source_group(TREE ${PROJECT_SOURCE_DIR} FILES ${SOURCES} ${HEADERS})

add_executable(vim-viperos ${SOURCES} ${HEADERS})

target_include_directories(vim-viperos PUBLIC include)

set_property(TARGET vim-viperos PROPERTY C_STANDARD 11)

target_link_libraries(vim-viperos PUBLIC m)