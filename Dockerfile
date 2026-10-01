FROM ubuntu:22.04

# Set non-interactive installation mode
ENV DEBIAN_FRONTEND=noninteractive

# Install necessary dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    g++ \
    libboost-all-dev \
    libgmp-dev \
    zlib1g-dev \
    libpthread-stubs0-dev \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

# Create a directory for the application
WORKDIR /app

# Copy the source files
COPY main.cpp server.cpp client.cpp rsa.cpp ./
COPY *.h ./

# Create an empty resource.rs file (since resource.rc is Windows-specific)
RUN touch resource.rs

# Modify main.cpp to handle Windows-specific code
RUN sed -i 's/#include <windows.h>/#ifdef _WIN32\n#include <windows.h>\n#endif/g' main.cpp && \
    sed -i 's/#include <tlhelp32.h>/#ifdef _WIN32\n#include <tlhelp32.h>\n#endif/g' main.cpp && \
    sed -i 's/#include <Shlwapi.h>/#ifdef _WIN32\n#include <Shlwapi.h>\n#endif/g' main.cpp && \
    sed -i 's/#include <shellapi.h>/#ifdef _WIN32\n#include <shellapi.h>\n#endif/g' main.cpp

# Compile the application
RUN g++ main.cpp server.cpp client.cpp rsa.cpp -o launcher \
    -DLINUX -DDEBUG -std=c++17 -Os -s -static-libstdc++ \
    -lz -lgmp -lboost_system -lpthread

# Set the entry point to the compiled executable
CMD ["bash"] 