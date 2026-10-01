#pragma once

#include "client.h"

class ProxyClientServer {
public:
    ProxyClientServer();

    ~ProxyClientServer();

    bool init(const std::set <uint16_t>& ports);

    void terminate();

    void setMaxActiveProxyConnections(uint16_t value)
    {
        m_maxConnections = value;
    }

    void addProxy(const std::string& host, const std::string& ip, uint16_t port, int priority = 0);

private:
    void accept(boost::asio::ip::tcp::acceptor& acceptor, uint16_t port);

    bool m_working = false;
    std::thread m_thread;
    boost::asio::io_context m_io;
    std::list <boost::asio::ip::tcp::acceptor> m_acceptors;
    uint16_t m_maxConnections = 2;
};
