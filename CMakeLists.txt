cmake_minimum_required(VERSION 2.8) #需要根据自己的cmake修改最低依赖版本 

# 设置交叉编译
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm)
# 这里指定的路径必须是绝对路径
set(CMAKE_C_COMPILER /usr/local/arm-linux/bin/arm-linux-gcc)
set(CMAKE_CXX_COMPILER /usr/local/arm-linux/bin/arm-linux-g++)

project(lvgl)

set(CMAKE_C_STANDARD 99)#C99 # lvgl officially support C99 and above
set(CMAKE_CXX_STANDARD 17)#C17
set(CMAKE_CXX_STANDARD_REQUIRED ON)


set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)


add_subdirectory(lvgl)
target_include_directories(lvgl PUBLIC ${PROJECT_SOURCE_DIR})
#查找指定目录下的源文件保存在变量 SRCS 中
aux_source_directory(./mysrc/src SRCS)

#指定自定义的头文件路径
include_directories(${PROJECT_SOURCE_DIR}/mysrc/inc)





add_executable(main main.c carnum.c income.c mouse_cursor_icon.c car_serve.c ${SRCS} )



target_link_libraries(main lvgl lvgl::examples lvgl::demos lvgl::thorvg ${SDL2_LIBRARIES} ${SDL2_IMAGE_LIBRARIES} ${Libdrm_LIBRARIES} m pthread)
add_custom_target (run COMMAND ${EXECUTABLE_OUTPUT_PATH}/main DEPENDS main)