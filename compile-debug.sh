#!/ usr / bin / env bash
set - e

          os = "$(uname -s)"

#Ensure a compiler is available
                   if command -
                   v g++ >
               / dev / null;
then CXX = g++ elif command - v clang++ > / dev / null;
then CXX =
    clang++ else echo "No C++ compiler found, attempting to install..." if[["$os" == "Darwin"]];
then if command - v brew > / dev / null;
then brew install llvm CXX = clang++ else echo
                             "Homebrew not found. Please install Xcode or a compiler manually." >
                             &2 exit 1 fi else if command - v apt - get > / dev / null;
then sudo apt - get update sudo apt - get install - y g++ CXX =
    g++ elif command - v yum > / dev / null;
then sudo yum install - y gcc - c++ CXX =
    g++ else echo
    "Unsupported Linux distribution. Please install g++ manually." > &2 exit 1 fi fi fi

                                                                         if command -
                                                                         v pkg - config >
    / dev / null&& pkg - config-- exists libgit2;
then : else if[["$os" == "Darwin"]];
then if command - v brew > / dev / null&& brew ls-- versions libgit2 > / dev / null;
then echo "libgit2 found via brew" else echo "libgit2 not found, attempting to install..."./
    install_deps.sh fi else echo "libgit2 not found, attempting to install..."./
    install_deps.sh fi fi

        PKG_CFLAGS =
    "$(pkg-config --cflags libgit2 2>/dev/null || echo '') $(pkg-config --cflags yaml-cpp "
    "2>/dev/null || echo '')" PKG_LIBS =
        "$(pkg-config --libs libgit2 2>/dev/null || echo '-lgit2') $(pkg-config --libs yaml-cpp "
        "2>/dev/null || echo '-lyaml-cpp')"

        ${CXX} -
        std = c++ 20 - O0 - g - fsanitize =
                  address ${
                      PKG_CFLAGS} autogitpull.cpp git_utils.cpp tui.cpp logger.cpp resource_utils
                      .cpp system_utils.cpp time_utils.cpp config_utils.cpp debug_utils.cpp ${
                          PKG_LIBS} -
                  fsanitize = address - o autogitpull_debug
