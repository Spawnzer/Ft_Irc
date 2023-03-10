/* Local Includes */
#include "Channel.hpp"
#include "Client.hpp"
#include "defines.h"

class Client;

Channel::Channel(const std::string& name, Client* owner)
  : _name(name), _owner(owner), _timeStart(std::time(nullptr)) {
	/* Set default channel modes */
	_modes = 0;
	setModes(TOPIC_SET_OP | NO_MSG_IN);
}

/*******************************/
/*        Mode Management      */
/*******************************/

/* Set channel wide mode flags */
void Channel::setModes(char modes, bool removeMode) {
	if (removeMode) {
		_modes &= ~(modes);
		return;
	}
	_modes |= modes;
}

/* Set member mode flags for a specified client */
void Channel::setMemberModes(Client* client, char modes, bool removeMode) {
	MemberMap::iterator it = _members.find(client);

	/* Check if user is a member of channel*/
	if (it == _members.end( )) {
		/* Check if a user is a member of notMembers */
		it = _notMembers.find(client);

		/* If user is not known to the Channel, add them */
		if (it == _notMembers.end( )) {
			if (removeMode)
				return;
			_notMembers.insert(std::pair< Client*, int >(client, modes));
			it = _notMembers.find(client);
		}
	}

	/* If setmode is to remove flag*/
	if (removeMode) {
		if (modes & PASS_REQ)
			_password.clear( );
		it->second &= ~(modes);
		return;
	}
	it->second |= modes;
}

/* Check member mode flags for specified client */
bool Channel::checkMemberModes(Client* client, char modes) const {
	MemberMap::const_iterator it = _members.find(client);

	/* Return without doing anything if user not found in list */
	if (it == _members.end( )) {
		it = _notMembers.find(client);
		if (it == _notMembers.end( ))
			return (false);
	}
	return (it->second & modes);
}

/* Get a string containing all current channel modes */
std::string Channel::getChannelModes(void) const {
	std::string channelModes;
	channelModes.append(checkModes(SECRET) ? "s" : "");
	channelModes.append(checkModes(TOPIC_SET_OP) ? "t" : "");
	channelModes.append(checkModes(INV_ONLY) ? "i" : "");
	channelModes.append(checkModes(NO_MSG_IN) ? "n" : "");
	channelModes.append(checkModes(PASS_REQ) ? "k" : "");
	return channelModes;
}

/* Return a list of all banned users */
std::vector< std::string > Channel::getBanList(void) const {
	std::vector< std::string > banList;
	MemberMap::const_iterator  ite;

	ite = _members.end( );
	for (MemberMap::const_iterator it = _members.begin( ); it != ite; ++it)
		if (it->second & BAN)
			banList.push_back(it->first->getPrefix( ));

	ite = _notMembers.end( );
	for (MemberMap::const_iterator it = _notMembers.begin( ); it != ite; ++it)
		if (it->second & BAN)
			banList.push_back(it->first->getPrefix( ));

	return banList;
}

/***********************************/
/*    Channel Member Management    */
/***********************************/

/* Check if specified client is a member of channel */
bool Channel::isMember(Client* client) {
	return (_members.find(client) != _members.end( ));
}

/* Add a new member to channel */
void Channel::addMember(Client* client, const std::string& reply, int modes) {
	/* Check if user is on notMembers list and move them */
	MemberMap::iterator it = _notMembers.find(client);
	if (it != _notMembers.end( )) {
		_members.insert(std::pair< Client*, int >(client, modes | it->second));
		_notMembers.erase(it);
	}
	/* Add member and set default member modes */
	else {
		_members.insert(std::pair< Client*, int >(client, modes));
	}
	sendToAll(reply);
	std::cout << getTimestamp( ) << GREEN "New member: " CLEAR << client->getNickname( )
	          << GREEN " joined channel: " CLEAR << this->getName( ) << std::endl;
}

/* Remove a member from channel */
void	Channel::removeMember(Client* client) {
	bool				wasOpe = false;
	MemberMap::iterator it = _members.find(client);


	/* Return is member not found in channel */
	if (it == _members.end( ))
		return;

	/* Check if member was owner */
	if (checkMemberModes(client, C_OP))
		wasOpe = true;

	/* If member is banned keep track of them*/
	if (checkMemberModes(client, BAN))
		_notMembers[it->first] = it->second;

	/* Erase member from channel */
	_members.erase(it);

	/* If member was operator, make sure there is at least one operator in channel */
	if (!_members.empty( ) && wasOpe)
		ensureOperator( );
}

/*
 * @param isMember: Invisible (mode +i) users will only be displayed to member requesting users
 */
std::string Channel::getMemberList(bool isMember) {
	MemberMap::iterator it = _members.begin( );
	std::string         list;

	// fixme either do vector+sort to have owner+ops at the top of list or sort them that
	// way while adding them
	/*loops through the MemberMap and build a space-separated list of every nickname
	 * on this channel, adding a '@' in front of the nick of owner/ops */
	for (; it != _members.end( ); ++it) {
		if (!isMember && it->first->checkGlobalModes(INVIS)){
			continue;
		}
		if (checkMemberModes(it->first,C_OP | OWNER))
			list.append("@" + it->first->getNickname( ));
		else
			list.append(it->first->getNickname( ));
		list.append(" ");
	}
	return list;
}

std::vector< std::string > Channel::getMemberVector(void) {
	MemberMap::iterator        it = _members.begin( );
	std::vector< std::string > list;

	/* loops through the MemberMap and builds a space-separated list of every nickname
	 * on this channel, adding a '@' in front of the nick of owner/ops */
	for (; it != _members.end( ); ++it) {
		if (it->second == C_OP || it->second == OWNER)
			list.push_back("@" + it->first->getNickname( ));
		else if (!it->first->checkGlobalModes(INVIS))
			list.push_back(it->first->getNickname( ));
	}
	return list;
}

size_t Channel::getNbVisibleUsers(void) const {
	MemberMap::const_iterator it             = _members.begin( );
	size_t                    nbVisibleUsers = 0;

	/* Loops through the MemberMap and counts the amount of visible users */
	for (; it != _members.end( ); ++it) {
		if (!it->first->checkGlobalModes(INVIS))
			++nbVisibleUsers;
	}
	return nbVisibleUsers;
}

/* Make sure there is at least one OP in channel*/
void Channel::ensureOperator(void) {
	MemberMap::iterator it = _members.begin( );

	/* Check if there is another OP in channel */
	for (; it != _members.end( ); ++it) {
		if (checkMemberModes(it->first, C_OP))
			return;
	}
	/* If no OP found, assign OP for first member in channel */
	it = _members.begin( );
	setMemberModes(it->first, C_OP);
}

/* Send message to all members of channel other than client */
void Channel::sendToOthers(const std::string& reply, Client* sender) {
	MemberMap::iterator it = _members.begin( );

	for (; it != _members.end( ); ++it) {
		if (it->first != sender)
			it->first->reply(reply);
	}
}

/* Send reply to all members of channel */
void Channel::sendToAll(const std::string& reply) {
	MemberMap::iterator it = _members.begin( );
	for (; it != _members.end( ); ++it)
		it->first->reply(reply);
}