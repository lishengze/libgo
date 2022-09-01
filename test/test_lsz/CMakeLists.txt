# copyright [2018] [wxblockchain]
# author: daniel.bian
# description: galaxy project main CMakeLists.txt
# update-date:
#   2019-01-04  init version
##############################################################################################
CMAKE_MINIMUM_REQUIRED(VERSION 3.12)
PROJECT(go_test)

SET("BIN_NAME" go_test)
set(CMAKE_CXX_STANDARD 11)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON) #Generate compile_commands.json

set(CMAKE_CXX_FLAGS "-fPIC -g -Wall -m64 ${CMAKE_CXX_FLAGS}")
# set(CMAKE_CXX_FLAGS_DEBUG "-g ${CMAKE_CXX_FLAGS}")
# set(CMAKE_CXX_FLAGS_RELEASE "-g -O3 ${CMAKE_CXX_FLAGS}")

set(BOOST_ROOT /usr/local/boost_1_68)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

aux_source_directory(../../libgo GO_MAIN)
aux_source_directory(../../libgo/common GO_COMMON)
aux_source_directory(../../libgo/context GO_CONTEXT)
aux_source_directory(../../libgo/task GO_TASK)
aux_source_directory(../../libgo/scheduler GO_SCHEDULER)
aux_source_directory(../../libgo/sync GO_SYNC)
aux_source_directory(../../libgo/timer GO_TIMER)
aux_source_directory(../../libgo/cls GO_CLS)
aux_source_directory(../../libgo/defer GO_DEFER)
aux_source_directory(../../libgo/pool GO_POOL)
aux_source_directory(../../libgo/debug GO_DEBUG)

AUX_SOURCE_DIRECTORY(. MAIN_SRC)

INCLUDE_DIRECTORIES("/usr/local/include/libgo")

SET(ALL_SRC ${MAIN_SRC} ${GO_MAIN} ${GO_COMMON} ${GO_CONTEXT} ${GO_TASK}
${GO_SCHEDULER} ${GO_SYNC} ${GO_TIMER} ${GO_CLS} ${GO_DEFER} ${GO_POOL} ${GO_DEBUG})

# generate bin
ADD_EXECUTABLE(${BIN_NAME} ${ALL_SRC})

TARGET_LINK_LIBRARIES(${BIN_NAME} ${Boost_LIBRARIES})
TARGET_LINK_LIBRARIES(${BIN_NAME} Threads::Threads)
TARGET_LINK_LIBRARIES(${BIN_NAME} curl pthread dl)

# TARGET_LINK_LIBRARIES(${BIN_NAME} c)
# TARGET_LINK_LIBRARIES(${BIN_NAME} static_hook)
# TARGET_LINK_LIBRARIES(${BIN_NAME} libgo)


