#include "shared.h"
#include "Server.h"
#include "Engine.h"

Server::Server(Engine *pEHandle) {
	pEngine = pEHandle;
	m_endpoint.clear_access_channels(websocketpp::log::alevel::all);
	m_endpoint.set_access_channels(websocketpp::log::alevel::access_core);
	m_endpoint.set_access_channels(websocketpp::log::alevel::app);

	m_endpoint.init_asio();

	using websocketpp::lib::placeholders::_1;
	using websocketpp::lib::placeholders::_2;
	using websocketpp::lib::bind;
	m_endpoint.set_open_handler(bind(&Server::on_open, this, _1));
	m_endpoint.set_close_handler(bind(&Server::on_close, this, _1));
	m_endpoint.set_message_handler(bind(&Server::on_message, this, _1, _2));
	m_endpoint.set_http_handler(bind(&Server::on_http, this, _1));
}

void Server::run(uint16_t port) {
	std::stringstream ss;
	ss << "Running engine server on port " << port;
	m_endpoint.get_alog().write(websocketpp::log::alevel::app, ss.str());

	m_endpoint.listen(port);
	m_endpoint.start_accept();

	try {
		m_endpoint.run();
	} catch (websocketpp::exception const & e) {
		std::cout << e.what() << std::endl;
	}
}

void Server::on_message(connection_hdl hdl, message_ptr msg) {
	std::cout 
		<< "on_message called with hdl: " << hdl.lock().get()
		<< " and message: " << msg->get_payload()
		<< std::endl;
	try {
		Message message;
		message.hdl = std::reinterpret_pointer_cast<void>(m_endpoint.get_con_from_hdl(hdl));
		message.msg = msg->get_payload();
		pEngine->queueMessage(message);
		
		//m_endpoint.send(hdl, "OK", msg->get_opcode());
	} catch (websocketpp::exception const & e) {
		std::cout << "Echo failed because: "
			<< "(" << e.what() << ")" << std::endl;
	}
}

void Server::on_http(connection_hdl hdl) {
	auto con = m_endpoint.get_con_from_hdl(hdl);
	con->set_status(websocketpp::http::status_code::ok);
	con->append_header("access-control-allow-origin", "*");
	con->append_header("content-type", "application/json; charset=UTF-8");
	con->set_body("{\"error\":\"resource unavailable\"}");
}

void Server::on_open(connection_hdl hdl) {
	m_connections.insert(hdl);
}

void Server::on_close(connection_hdl hdl) {
	m_connections.erase(hdl);
}

void Server::reply(const Message& msg) {
	auto ptr = std::reinterpret_pointer_cast<websocketpp::connection<websocketpp::config::asio>>(msg.hdl);
	ptr->send(msg.msg);
}