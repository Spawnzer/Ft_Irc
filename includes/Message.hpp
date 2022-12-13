#ifndef MESSAGE_HPP
# define MESSAGE_HPP

#pragma once

/* System Includes */
#include <string>
#include <vector>

/* Local Includes */

class Client;

/* Class Prototypes */
class Command;

class Message {
	public:
		/* Constructors & Destructor */
		Message(Client* client, std::string raw);
		~Message();

		/* Setters & Getters */
		const 	std::string&				getCommand() const;
		const	std::string&				getPrefix() const;
		const	std::vector<std::string>&	getParams() const;

	private:
		std::vector<std::string>	_params;
		std::string					_prefix;
		std::string					_cmd;	// Represents the command string, in lower case
		Client*						_client;
};

#endif