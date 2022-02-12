cmake_minimum_required(VERSION 3.22)
project (thjson)

# this shit copied and modified from
# https://gauriatiq.medium.com/c-native-addon-independent-of-node-js-version-using-napi-node-addon-api-and-cmake-53315582cbd1
# if it doesnt work blame them

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${CMAKE_JS_INC})
add_library(${PROJECT_NAME} SHARED replay_decode.cpp threplay.cpp ${CMAKE_JS_SRC})

target_compile_options(${PROJECT_NAME} PUBLIC -fno-exceptions)
target_compile_definitions(${PROJECT_NAME} PUBLIC NAPI_DISABLE_CPP_EXCEPTIONS)

# This line will give our library file a .node extension without any "lib" prefix
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "" SUFFIX ".node")

# Essential library files to link to a node addon,
# you should add this line in every CMake.js based project.
target_link_libraries(${PROJECT_NAME} ${CMAKE_JS_LIB})

# Include N-API wrappers
# no goddamn idea what this is doing but it works i guess?
execute_process(COMMAND node -p "require('node-addon-api').include"
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE NODE_ADDON_API_DIR
        )
string(REPLACE "\n" "" NODE_ADDON_API_DIR ${NODE_ADDON_API_DIR})
string(REPLACE "\"" "" NODE_ADDON_API_DIR ${NODE_ADDON_API_DIR})
target_include_directories(${PROJECT_NAME} PRIVATE ${NODE_ADDON_API_DIR})

add_definitions(-DNAPI_VERSION=3)