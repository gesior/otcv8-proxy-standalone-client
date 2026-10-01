# OTCv8 Proxy Standalone Client

This is a standalone client for the OTCv8 proxy. It allows you to use the OTCv8 proxy system 
with the official (RL) Tibia client.

It works with clients 7.x - 13.40.
On 14.x they changed something in connection to the server protocol that breaks 
the 'standalone proxy' client and 'alpha proxy' server - probably packet size is counted in XTEA blocks, not bytes.

## Compilation

You can compile it using CLion, Visual Studio, or CMake.

- **Visual Studio instructions:** [doc/compilation-visual-studio.md](doc/compilation-visual-studio.md)
- **CLion instructions:** [doc/compilation-clion.md](doc/compilation-clion.md)

## Configuration

Proxy configuration can be found in the `proxy.config` file.

Example configuration for an OTS with 2 HAProxy servers listening on port 6600:
```
My Server Name
.\Tibia.exe
7171 7172
proxy-1.example.com proxy-1.example.com 6600
proxy-2.example.com proxy-2.example.com 6600
```

More details can be found in the [configuration documentation](doc/configuration.md).

## How does it work?

When you launch `launcher.exe`, it starts listening on the configured ports (`7171 7172` from `proxy.config`) on 
`localhost` and forwards TCP connections to the configured proxy server IPs and ports 
(e.g., `proxy-1.example.com 6600`).

It then launches the configured binary, e.g., `.\Tibia.exe`.

To make the client use the proxy, you need to edit the domains or IPs in the Tibia client 
so that it connects to `127.0.0.1`.

If you need domains that are 17 and 19 characters long pointing to `127.0.0.1`, you can use the following:
```
log1.otupdate.ovh
log123.otupdate.ovh
```
