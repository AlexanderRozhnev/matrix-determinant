cmake_minimum_required(VERSION 3.10)

project(Matrix C)

# Project files
set(SERVER_FILES
        "array_memory.h"    "array_memory.c"
        "server_output.h"   "server_output.c"
        "server.c")

set (CLIENT_FILES
        "array_memory.h"    "array_memory.c"
        "keyboard.h"        "keyboard.c"
        "client.c")

# Target executable
add_executable(server ${SERVER_FILES})

add_executable(client ${CLIENT_FILES})