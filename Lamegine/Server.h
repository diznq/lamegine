#pragma once
#pragma warning( disable: 4267 )
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

#include <set>
#include <string>

class Engine;
class Server {
public:

	typedef websocketpp::connection_hdl connection_hdl;
	typedef websocketpp::server<websocketpp::config::asio> server;
	typedef server::message_ptr message_ptr;

	Server(Engine *pEngine);
	void run(uint16_t port);
	void on_message(connection_hdl hdl, message_ptr msg);
	void on_http(connection_hdl hdl);
	void on_open(connection_hdl hdl);
	void on_close(connection_hdl hdl);

	void reply(const Message& msg);
private:
	typedef std::set<connection_hdl, std::owner_less<connection_hdl>> con_list;
	server m_endpoint;
	con_list m_connections;
	server::timer_ptr m_timer;
	Engine *pEngine;
};