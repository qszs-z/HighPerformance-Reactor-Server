cmake_minimum_required(VERSION 3.10)
project(MyReactor)

# Setting C++ Standard (Force C++17 or C++14 for Modern C++)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ========================================================
# 1. 查找系统线程库 (Modern CMake 标准写法)
# 这行代码会自动检测当前系统需要什么参数才能支持线程
# 在 Linux 下它会自动找到 -pthread
# ========================================================
find_package(Threads REQUIRED)
# 1. 关键：告诉编译器去哪里找头文件 (.h)
# 这样你在 .cpp 里写 #include "Socket.h" 就不用写长长的路径了
include_directories(include)

# 2. 编译服务端
add_executable(TcpServer 
    "src/main.cpp" 
    src/InetAddress.cpp 
    src/Socket.cpp
    src/Epoll.cpp
    src/Channel.cpp
    src/EventLoop.cpp
    src/TcpServer.cpp
    src/Acceptor.cpp
    src/Connection.cpp
    src/Buffer.cpp
    src/EchoServer.cpp 
    src/ThreadPool.cpp
    src/Timestamp.cpp;
)

# 【关键】链接线程库到 Server
# Threads::Threads 是 CMake 的抽象目标，不用管它底层是 -lpthread 还是啥
target_link_libraries(TcpServer PRIVATE Threads::Threads)

# ========================================================
# 4. 编译客户端
# ========================================================
add_executable(TcpClient 
    src/TcpClient.cpp  
)
