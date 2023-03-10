#include "commands/Join.hpp"

Join::Join(Server* server) : Command("join", server) {}

/* Parse raw message data into vector of channel/password pairs */
bool Join::parse(const Message& msg) {
	/* Error message if not enough information received to execute command */
	if (msg.getMiddle( ).empty( )) {
		_client->reply(ERR_NEEDMOREPARAMS(
		  _server->getHostname( ), _client->getNickname( ), msg.getCommand( )));
		return (false);
	}

	/* Split all requested channels into _targets vector */
	std::string rawChannels = msg.getMiddle( ).at(0);
	size_t      posChannels = rawChannels.find(',');
	std::string rawPasswords;
	size_t      posPasswords = std::string::npos;
	bool        pass         = false;

	/* Check if passwords were also provided */
	if (msg.getMiddle( ).size( ) > 1) {
		pass         = true;
		rawPasswords = msg.getMiddle( ).at(1);
		posPasswords = rawPasswords.find(',');
	}

	/* Iterate through channels and add channel/password pair to vector (_targets) */
	while ((posChannels = rawChannels.find(',')) != std::string::npos) {
		/* If passwords and still remaining passwords add name/pass to vector */
		if (pass && !rawPasswords.empty( )) {
			_targets.push_back(StringPair(rawChannels.substr(0, posChannels),
			                              rawPasswords.substr(0, posPasswords)));
			rawPasswords.erase(0, posPasswords + 1);
			posPasswords = rawPasswords.find(',');
		}
		/* Otherwise add name and empty password to vector */
		else
			_targets.push_back(
			  StringPair(rawChannels.substr(0, posChannels), std::string( )));
		rawChannels.erase(0, posChannels + 1);
	}

	/* Add remaining name/pass to vector */
	if (pass && !rawPasswords.empty( ))
		_targets.push_back(StringPair(rawChannels, rawPasswords));
	else
		_targets.push_back(StringPair(rawChannels, std::string( )));
	return (true);
}

/* Validate channel join requests before attempting to exectute*/
bool Join::validate(StringPair channel) {
	/* QoL variables*/
	std::string name = channel.first;
	std::string pass = channel.second;

	/* Check if first character is valid */
	if (name.size( ) > 0 && name.at(0) != '#') {
		_client->reply(
		  ERR_BADCHANMASK(_server->getHostname( ), _client->getNickname( ), name));
		return (false);
	}

	/* Check if name contains non-printable characters or is longer than 200 chars */
	if (name.size( ) > 200 || !checkInvalidChars(name)) {
		_client->reply(
		  ERR_NOSUCHCHANNEL(_server->getHostname( ), _client->getNickname( ), name));
		return (false);
	}

	/* If channel does exist, check modes and password */
	if (_server->doesChannelExist(name)) {
		/* QoL variable */
		Channel* channelPtr = _server->getChannelPtr(name);

		/* If user is already a member of channel then do nothing */
		if (channelPtr->isMember(_client))
			return (false);

		/* Check if channel is invite only & user is not invited */
		if (channelPtr->checkModes(INV_ONLY)
		    && !channelPtr->checkMemberModes(_client, INVIT)) {
			_client->reply(
			  ERR_INVITEONLYCHAN(_server->getHostname( ), _client->getNickname( ), name));
			return (false);
		}

		/* Check if client is banned from channel */
		if (channelPtr->checkMemberModes(_client, BAN)) {
			_client->reply(
			  ERR_BANNEDFROMCHAN(_server->getHostname( ), _client->getNickname( ), name));
			return (false);
		}

		/* If channel is password protected check password match*/
		if (channelPtr->checkModes(PASS_REQ) && channelPtr->getPassword( ) != pass) {
			_client->reply(
			  ERR_BADCHANNELKEY(_server->getHostname( ), _client->getNickname( ), name));
			return (false);
		}
	}
	return (true);
}

void Join::execute(const Message& msg) {
	_targets.clear( );
	_client = msg._client;

	/* Parse raw message into vector of channel/password pairs */
	if (!parse(msg))
		return;

	/* Iterate through channels list and attempt to validate and exectute for each one */
	ChannelList::iterator ite       = _targets.end( );
	bool                  hasJoined = true;

	for (ChannelList::iterator it = _targets.begin( ); it != ite; ++it) {
		/* QoL variables */
		std::string name = it->first;
		std::string pass = it->second;
		Channel*    channelPtr;

		/* Attempt to validate*/
		if (validate(*it) == false)
			continue;

		/* If channel does not exist, create it */
		if (!_server->doesChannelExist(name)) {
			_server->createChannel(name, _client);
			hasJoined = false;
		}

		/* Get pointer to channel */
		channelPtr = _server->getChannelPtr(name);
		if (channelPtr == nullptr)
			return;

		/* Add new member to channel*/
		channelPtr->addMember(_client, CMD_JOIN(_buildPrefix(msg), name));

		/* If the channel didn't exist, make the client a channel operator */
		if (!hasJoined)
			channelPtr->setMemberModes(_client, C_OP);

		/* Send reply messages */
		_client->reply(RPL_NAMREPLY(_server->getHostname( ),
		                            _client->getNickname( ),
		                            name,
		                            channelPtr->getMemberList(channelPtr->isMember(_client))));
		_client->reply(
		  RPL_ENDOFNAMES(_server->getHostname( ), _client->getNickname( ), name));

		/* Manage topic reply */
		if (hasJoined && channelPtr->getTopic( ).size( ) > 0)
			_client->reply(RPL_TOPIC(_server->getHostname( ),
			                         _client->getNickname( ),
			                         name,
			                         channelPtr->getTopic( )));
		else if (hasJoined)
			_client->reply(
			  RPL_NOTOPIC(_server->getHostname( ), _client->getNickname( ), name));
	}
}

/* Check for unprintable characters in channel name */
bool Join::checkInvalidChars(const std::string& string) {
	std::string::const_iterator it = string.begin( );

	for (; it != string.end( ); ++it) {
		if (!isprint(*it))
			return (false);
	}
	return (true);
}