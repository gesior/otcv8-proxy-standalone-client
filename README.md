# Proxy Standalone Client - Docker Build

This repository contains Docker configuration to compile the proxy standalone client on Linux Ubuntu 22.04.

## Requirements

- Docker
- Docker Compose

## Building the Application

1. Make sure Docker and Docker Compose are installed on your system.
2. Run the following command in the project root directory:

```bash
docker-compose up --build
```

3. After successful compilation, the binary will be available in the `build` directory as `launcher`.

## Notes

- This Docker setup creates a Linux build of the application.
- Windows-specific resources (resource.rc, ico files) are handled differently in the Linux build.
- The `-DLINUX` flag is added to the compilation to allow for platform-specific code adaptations if needed. 