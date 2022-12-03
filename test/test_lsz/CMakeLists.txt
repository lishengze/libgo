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
ENABLE_LANGUAGE(ASM)

# set(CMAKE_CXX_FLAGS "-fPIC -g -Wall -m64 ${CMAKE_CXX_FLAGS}")
# set(CMAKE_CXX_FLAGS_DEBUG "-g ${CMAKE_CXX_FLAGS}")
# set(CMAKE_CXX_FLAGS_RELEASE "-g -O3 ${CMAKE_CXX_FLAGS}")

set(CMAKE_CXX_FLAGS "-std=c++11 -D__const__=  -fPIC -fno-strict-aliasing -Wall ${CMAKE_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-g -O3 -DNDEBUG")

set(BOOST_ROOT /usr/local/boost_1_68)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

execute_process(COMMAND "chmod" "+x" "/home/lsz/Code/Git/study/AnnotatedCode/libgo/third_party/select_asm.sh")
execute_process(COMMAND "chmod" "+x" "/home/lsz/Code/Git/study/AnnotatedCode/libgo/third_party/boost.context/bootstrap.sh")
execute_process(COMMAND "chmod" "+x" "/home/lsz/Code/Git/study/AnnotatedCode/libgo/third_party/boost.context/tools/build/src/engine/build.sh")

set(CMAKE_ASM_SOURCE_FILE_EXTENSIONS S)
message("--> select asm source file, please wait about 5 seconds ...")
execute_process(COMMAND "/home/lsz/Code/Git/study/AnnotatedCode/libgo/third_party/select_asm.sh" "/home/lsz/Code/Git/study/AnnotatedCode/libgo" "jump" OUTPUT_VARIABLE jump_asm_file)
execute_process(COMMAND "/home/lsz/Code/Git/study/AnnotatedCode/libgo/third_party/select_asm.sh" "/home/lsz/Code/Git/study/AnnotatedCode/libgo" "make" OUTPUT_VARIABLE make_asm_file)

aux_source_directory(../../libgo CO_SRC_LIST)
aux_source_directory(../../libgo/common CO_SRC_LIST)
aux_source_directory(../../libgo/context CO_SRC_LIST)
aux_source_directory(../../libgo/task CO_SRC_LIST)
aux_source_directory(../../libgo/scheduler CO_SRC_LIST)
aux_source_directory(../../libgo/sync CO_SRC_LIST)
aux_source_directory(../../libgo/timer CO_SRC_LIST)
aux_source_directory(../../libgo/cls CO_SRC_LIST)
aux_source_directory(../../libgo/defer CO_SRC_LIST)
aux_source_directory(../../libgo/pool CO_SRC_LIST)
aux_source_directory(../../libgo/debug CO_SRC_LIST)
aux_source_directory(../../libgo/netio/unix CO_SRC_LIST)
aux_source_directory(. CO_SRC_LIST)


list(APPEND CO_SRC_LIST ${jump_asm_file})
list(APPEND CO_SRC_LIST ${make_asm_file})

message("\n---- jump_asm_file: " ${jump_asm_file})
message("\n---- make_asm_file: " ${make_asm_file})
message("\n---- make_asm_file: " ${CO_SRC_LIST})

ADD_EXECUTABLE(${BIN_NAME} ${CO_SRC_LIST})

INCLUDE_DIRECTORIES(../../libgo)

# aux_source_directory(../../libgo GO_MAIN)
# aux_source_directory(../../libgo/common GO_COMMON)
# aux_source_directory(../../libgo/context GO_CONTEXT)
# aux_source_directory(../../libgo/task GO_TASK)
# aux_source_directory(../../libgo/scheduler GO_SCHEDULER)
# aux_source_directory(../../libgo/sync GO_SYNC)
# aux_source_directory(../../libgo/timer GO_TIMER)
# aux_source_directory(../../libgo/cls GO_CLS)
# aux_source_directory(../../libgo/defer GO_DEFER)
# aux_source_directory(../../libgo/pool GO_POOL)
# aux_source_directory(../../libgo/debug GO_DEBUG)
# aux_source_directory(../../libgo/netio/unix GO_NETIO)

# SET(ALL_SRC ${MAIN_SRC} ${GO_MAIN} ${GO_COMMON} ${GO_CONTEXT} ${GO_TASK}
#             ${GO_SCHEDULER} ${GO_SYNC} ${GO_TIMER} ${GO_CLS} 
#             ${GO_DEFER} ${GO_POOL} ${GO_DEBUG} ${GO_NETIO})

# # generate bin
# ADD_EXECUTABLE(${BIN_NAME} ${ALL_SRC})


# target_link_libraries(${BIN_NAME} ${LINK_LIBS} -ldl)
TARGET_LINK_LIBRARIES(${BIN_NAME} ${Boost_LIBRARIES})
TARGET_LINK_LIBRARIES(${BIN_NAME} Threads::Threads)
TARGET_LINK_LIBRARIES(${BIN_NAME} curl pthread dl)

# TARGET_LINK_LIBRARIES(${BIN_NAME} c)
# TARGET_LINK_LIBRARIES(${BIN_NAME} static_hook)
# TARGET_LINK_LIBRARIES(${BIN_NAME} libgo)


