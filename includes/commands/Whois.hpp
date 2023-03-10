#ifndef WHOIS_HPP
#define WHOIS_HPP

#pragma once

/* System Includes */
#include <string>

/* Local Includes */
#include "../Command.hpp"

class Whois : public Command
{
  public:
	/* QoL typedefs */
	typedef std::map< std::string, Channel* > ChannelList;

	/* Constructors & Destructor */
	Whois(Server* server);
	~Whois( );

	/* Public Member Functions */
	bool validate(const Message& msg);
	void execute(const Message& msg);

  private:
	/* Private member attributes */
	Client* _client;
	Client* _target;
};

#endif