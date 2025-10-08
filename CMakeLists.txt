cmake_minimum_required(VERSION 3.15)
project(WinVLCBridge)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# VLC 路径配置（可以通过命令行参数覆盖）
if(NOT DEFINED VLC_PATH)
    set(VLC_PATH "${CMAKE_CURRENT_SOURCE_DIR}/vlc-3.0.21")
endif()

# 查找 VLC 头文件和库
find_path(VLC_INCLUDE_DIR 
    NAMES vlc/vlc.h
    PATHS ${VLC_PATH}/sdk/include
    REQUIRED
)

find_library(VLC_LIBRARY
    NAMES libvlc.lib vlc.lib libvlc
    PATHS ${VLC_PATH}/sdk/lib
    REQUIRED
)

find_library(VLCCORE_LIBRARY
    NAMES libvlccore.lib vlccore.lib libvlccore
    PATHS ${VLC_PATH}/sdk/lib
    REQUIRED
)

message(STATUS "VLC Include Dir: ${VLC_INCLUDE_DIR}")
message(STATUS "VLC Library: ${VLC_LIBRARY}")
message(STATUS "VLC Core Library: ${VLCCORE_LIBRARY}")

# 源文件
set(SOURCES
    WinVLCBridge.cpp
)

set(HEADERS
    WinVLCBridge.h
)

# 创建动态链接库
add_library(${PROJECT_NAME} SHARED ${SOURCES} ${HEADERS})

# 定义导出宏
target_compile_definitions(${PROJECT_NAME} PRIVATE WINVLCBRIDGE_EXPORTS)

# 包含目录
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${VLC_INCLUDE_DIR}
)

# 链接库
target_link_libraries(${PROJECT_NAME} PRIVATE
    ${VLC_LIBRARY}
    ${VLCCORE_LIBRARY}
    gdiplus
)

# Windows 特定设置
if(WIN32)
    # 设置 DLL 输出名称
    set_target_properties(${PROJECT_NAME} PROPERTIES
        OUTPUT_NAME "WinVLCBridge"
        PREFIX ""
    )
    
    # 在构建后复制 VLC DLL 到输出目录
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${VLC_PATH}/libvlc.dll"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${VLC_PATH}/libvlccore.dll"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
        COMMENT "Copying VLC DLLs to output directory"
    )
    
    # 复制 plugins 目录
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${VLC_PATH}/plugins"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/plugins"
        COMMENT "Copying VLC plugins to output directory"
    )
endif()

# 安装规则
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install(FILES ${HEADERS} DESTINATION include)

