#include "server.h"

extern std::map <uint32_t, std::weak_ptr<Session>> g_sessions;
extern std::set <std::shared_ptr<Proxy>> g_proxies;

ProxyClientServer::ProxyClientServer()
{

}

ProxyClientServer::~ProxyClientServer()
{
    if (m_working)
        terminate();
}

bool ProxyClientServer::init(const std::set <uint16_t>& ports)
{
    if (ports.empty())
        return false;
    if (m_working)
        return true;
    m_working = true;

    for (auto& port: ports) {
        boost::system::error_code ec;
        boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::address_v4::loopback(), port);
        m_acceptors.push_back(boost::asio::ip::tcp::acceptor(m_io));
        auto& acceptor = m_acceptors.back();
        acceptor.open(endpoint.protocol(), ec);
        if (!ec) {
            acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true), ec);
            if (!ec) {
                acceptor.bind(endpoint, ec);
                if (!ec) {
                    acceptor.listen(2147483647, ec);
                }
            }
        }
        if (ec) {
#ifdef PROXY_DEBUG
            std::clog << "ProxyClientServer init error: " << ec.message() << std::endl;
#endif
            terminate();
            return false;
        }
        acceptor.set_option(boost::asio::ip::tcp::no_delay(true));
        accept(acceptor, port);
    }

    m_thread = std::thread([&] {
        m_io.run();
    });
    return true;
}

void ProxyClientServer::terminate()
{
    if (!m_working)
        return;
    m_working = false;
    boost::asio::post(m_io, [&] {
        for (auto& session_pair: g_sessions) {
            if (auto session = session_pair.second.lock()) {
                session->terminate();
            }
        }
        for (auto& proxy: g_proxies) {
            proxy->terminate();
        }
        for (auto& acceptor: m_acceptors) {
            boost::system::error_code ec;
            acceptor.close(ec);
        }
    });
    try {
        m_thread.join();
    }
    catch (const std::system_error&) { }
}

void ProxyClientServer::addProxy(const std::string& host, const std::string& ip, uint16_t port, int priority)
{
    if (!m_working)
        return;
    auto proxy = std::make_shared<Proxy>(m_io, host, ip, port, priority);
    proxy->start();
}

void ProxyClientServer::accept(boost::asio::ip::tcp::acceptor& acceptor, uint16_t port)
{
    acceptor.async_accept(
            [&, port](boost::system::error_code ec, boost::asio::ip::tcp::socket socket) {
                if (ec) {
                    return;
                }
#ifdef PROXY_DEBUG
                const boost::asio::ip::tcp::endpoint endpoint = socket.remote_endpoint(ec);
                if (!ec) {
                    std::clog << "[ProxyClientServer] new connection: " << endpoint.address().to_v4().to_string() << " port: " << port << std::endl;;
                }
#endif
                boost::system::error_code ecc;
                socket.set_option(boost::asio::socket_base::send_buffer_size(65536), ecc);
                socket.set_option(boost::asio::socket_base::receive_buffer_size(65536), ecc);
#ifdef PROXY_DEBUG
                if (ecc) {
                    std::clog << "[ProxyClientServer] sock error " << ecc.message() << std::endl;
                }
#endif

                auto session = std::make_shared<Session>(m_io, std::move(socket), port);
                session->start(m_maxConnections);
                accept(acceptor, port);
            });
}