#include "ImplementationLogger.hpp"
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <WinSock2.h>
#include <iostream>
#include <sstream>
#pragma comment(lib, "ws2_32.lib")

#define WEB_SERVER_PORT (4848)

namespace HA {

	Logger::Logger(bool WebServer)
		: WebServer(WebServer)
	{
		if (WebServer) {
			WSADATA wsaData;
			WSAStartup(MAKEWORD(2, 2), &wsaData);
			SocketFd = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
			sockaddr_in endpoint{};
			endpoint.sin_addr.s_addr = inet_addr("127.0.0.1");
			endpoint.sin_family = AF_INET;
			endpoint.sin_port = htons(WEB_SERVER_PORT);
			if (::connect(SocketFd, (sockaddr*)&endpoint, sizeof(endpoint)) < 0) {
				std::cout << "Connection Failed to webserver";
			}
		}
	}

	Logger::~Logger()
	{
		if (WebServer) {
			::closesocket(SocketFd);
		}
	}

	void Logger::Print(const char* message, bool bold, const char* color)
	{
		if (WebServer) {
			std::stringstream ss;
			if (bold) ss << "<b>";
			ss << "<span style='color:" << color << ">" << message << "</span>";
			if (bold) ss << "</b><br/>";
			std::string finalMessage = ss.str();
			::send(SocketFd, finalMessage.c_str(), finalMessage.size(), 0);
		}
		else {
			std::cout << message;
		}
	}

}