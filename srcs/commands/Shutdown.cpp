/* Local Includes */
#include "commands/Shutdown.hpp"

Shutdown::Shutdown(Server* server) : Command("shutdown", server) {

}

Shutdown::~Shutdown() {

}

void	Shutdown::execute(const Message& msg) {
	_server->stopServer();
}