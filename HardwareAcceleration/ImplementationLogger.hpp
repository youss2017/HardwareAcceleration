#pragma once

namespace HA {

	class Logger {
		
	public:
		Logger(bool WebServer);
		~Logger();

		void Print(const char* message, bool bold = false, const char* color = "black");

	public:
		const bool WebServer;

	private:
		unsigned long long SocketFd;
	};

}
