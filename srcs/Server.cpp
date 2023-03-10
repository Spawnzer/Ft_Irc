/* Local Includes */
#include "Server.hpp"
#include "Client.hpp"
#include "defines.h"
#include "replies.h"

/* Command Includes */
#include "commands/Away.hpp"
#include "commands/Invite.hpp"
#include "commands/Join.hpp"
#include "commands/Kick.hpp"
#include "commands/List.hpp"
#include "commands/Mode.hpp"
#include "commands/Names.hpp"
#include "commands/Nick.hpp"
#include "commands/Notice.hpp"
#include "commands/Part.hpp"
#include "commands/Pass.hpp"
#include "commands/Ping.hpp"
#include "commands/Pong.hpp"
#include "commands/Privmsg.hpp"
#include "commands/Quit.hpp"
#include "commands/Shutdown.hpp"
#include "commands/Topic.hpp"
#include "commands/User.hpp"
#include "commands/Who.hpp"
#include "commands/Whois.hpp"


/*****************************/
/* Constructor & Destructor */
/*****************************/

Server::Server(const std::string& servername, const int port, const std::string& password) :
	_servername(servername), _password(password), _timeStart(std::time(nullptr)), _port(port) {
	/* Attempt to initialize server */
	try
	{
		/* Setup server connection */
		initializeConnection();
		std::cout << getTimestamp() << GREEN "Server initialization successful" CLEAR << std::endl;
		std::cout << "\t\t\t\tport: " << port << std::endl << "\t\t\t\tpass: " << password << std::endl;
		
		/* Initialize commands map */
		initializeCommands();
		std::cout << getTimestamp() << GREEN "Command initialization successful" CLEAR<< std::endl;
	}
	catch(const std::exception& e)
	{
		std::cerr << e.what() << '\n';
		std::cerr << RED "Failure to initialize server, program exiting" CLEAR << std::endl;
		shutdown(_socket, SHUT_RDWR);
		exit (1);
	}

	/* Start Server Loop */
	std::cout << "_______________________________________________________" << std::endl << std::endl;
	std::cout << getTimestamp() <<  "Server status - " << GREEN "ONLINE" CLEAR << std::endl;
	std::cout << "_______________________________________________________" << std::endl << std::endl;
	runServer();
}

Server::~Server() {
	/* Delete Commands*/
	std::map<std::string, Command *>::iterator it = _commands.begin();
	for (; it != _commands.end(); ++it)
		delete (it->second);
	_commands.clear();

	/* Delete Clients */
	for (size_t i = 0; i < _clients.size(); i++)
		delete (_clients[i]);
	_clients.clear();
	
	/* Delete channels */
	std::map<std::string, Channel *>::iterator it_ch = _channels.begin();
	for (; it_ch != _channels.end(); ++it_ch)
		delete (it_ch->second);
	_channels.clear();

	/* Close server socket */
	shutdown(_socket, SHUT_RDWR);
}


/***********************************/
/*      Server Initialization      */
/***********************************/

/* Open server socket and configure for listening */
void		Server::initializeConnection(void) {
	int	yes = 1;

	/* Create socket */
	if ((_socket  = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		throw std::runtime_error("Unable to create socket");

	/* Set socket as non-blocking */
	fcntl(_socket, F_SETFL, O_NONBLOCK);

	/* Set socket options to reuse addresses */
	if (setsockopt(_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) < 0)
		throw std::runtime_error("Unable to set socket options");

	/* Setup socket address struct */
	_address.sin_port = htons(_port);
	_address.sin_family = AF_INET;
	_address.sin_addr.s_addr = INADDR_ANY;

	/* Bind Socket */
	if (bind(_socket, (struct sockaddr *)&_address, sizeof(_address)) < 0)
		throw std::runtime_error("Unable to bind socket");
	
	/* Set socket to passive listening */
	if (listen(_socket, MAX_CONNECTIONS) < 0)
		throw std::runtime_error("Unable to listen on socket");
	
	/* Get server hostname & IP */
	char hostname[1024];
	gethostname(hostname, sizeof(hostname));
	_hostname = hostname;
	struct hostent *host = gethostbyname(hostname);
	char ip[INET_ADDRSTRLEN + 1];
	inet_ntop(AF_INET, host->h_addr_list[0], ip, sizeof(ip));
	_ip = ip;
	
	/* Set Server Status */
	g_status = ONLINE;

	/* Set up pollFDs */
	pollfd pfd = {.fd = _socket, .events = POLLIN, .revents = 0};
	_pfds.push_back(pfd);
}

/* Build Server Commands */
void		Server::initializeCommands(void) {
	_commands["away"] = new Away(this);
	_commands["invite"] = new Invite(this);
	_commands["join"] = new Join(this);
	_commands["kick"] = new Kick(this);
	_commands["list"] = new List(this);
	_commands["mode"] = new Mode(this);
	_commands["names"] = new Names(this);
	_commands["nick"] = new Nick(this);
	_commands["notice"] = new Notice(this);
	_commands["part"] = new Part(this);
	_commands["pass"] = new Pass(this);
	_commands["ping"] = new Ping(this);
	_commands["pong"] = new Pong(this);
	_commands["privmsg"] = new Privmsg(this);
	_commands["quit"] = new Quit(this);
	_commands["shutdown"] = new Shutdown(this);
	_commands["topic"] = new Topic(this);
	_commands["user"] = new User(this);
	_commands["who"] = new Who(this);
	_commands["whois"] = new Whois(this);
}


/****************************************/
/*      Server Operation Functions      */
/****************************************/

/* Manage Connection Requests from New Clients */
void		Server::handleConnections() {
	/* Setup variables to manage connection */
	std::cout << getTimestamp() << RED "Incoming connection request" CLEAR<< std::endl;
	int	new_fd;
	int	addressLen;
	struct sockaddr_in clientAddress;
	addressLen = sizeof(clientAddress);

	/* Attempt to connect to client and get client address info*/
	if ((new_fd = accept(_socket, (struct sockaddr *)&clientAddress, (socklen_t *)&addressLen)) < 0)
		throw std::runtime_error("Failure to accept incoming connection due to socket error");
	
	/* Set socket option to ensure that we dont attempt to send on a socket that has been disconnected */
	int yes = 1;
	if (setsockopt(new_fd, SOL_SOCKET, SO_NOSIGPIPE, &yes, sizeof(int)) < 0)
		throw std::runtime_error("Unable to set socket options");

	/* Add client to _clients map and populate address variables */
	_clients.push_back(new Client(new_fd));
	_clients.back()->setAddress(clientAddress);
	_clients.back()->setHostname(inet_ntoa(clientAddress.sin_addr));

	/* Create new pollfd object and add to _pfds list */
	pollfd pfd = {.fd = new_fd, .events = POLLIN, .revents = 0};
	_pfds.push_back(pfd);

	/* Print new client data */
	std::cout << getTimestamp() << GREEN "New client connected successfully" CLEAR << std::endl;
	std::cout << "\t\t\t\taddress: " << inet_ntoa(clientAddress.sin_addr) <<  std::endl;
}

/* Read incoming data from client socket & perform actions */
void		Server::handleMessages(Client* client)
{	
	std::string	rawMessage;
	std::string	cmd;
	/* Client reads entire input string coming from their socket */
	if (client->read() <= 0)
	{
		/* Handle forcefully disconnected clients */
		std::cout << getTimestamp() << RED "Removing disconnected client: " CLEAR << client->getUsername() << std::endl;
		std::map<std::string, Channel*>::iterator it = _channels.begin();
		for (; it != _channels.end(); ++it)
		{
			if (it->second->isMember(client))
				it->second->sendToOthers(CMD_QUIT(client->getNickname(), client->getUsername(), client->getAddress()), client);
		}
		removeClient(client);
	}
	else
	{
		rawMessage = client->retrieveMessage();
		/* While there are valid commands (messages) stored in the client's input string */
		while (rawMessage.empty() == false)
		{
			/* Build message from rawMessage */
			Message	msg(client, rawMessage);
			cmd = msg.getCommand();

			/* If message is cap, do nothing */
			if (cmd != "cap")
			{
				/* If password has not been verified and command is not pass, terminate connection */
				if (!client->getPassStatus() && cmd != "pass")
				{
					client->reply("ERROR :Closing Link: localhost (Bad Password)\n");
					removeClient(client);
					break;
				}
				/* If client is not registered and tries to run a non-registration command send error */
				else if (!client->getRegistration() && cmd != "pass" && cmd != "nick" && cmd != "user")
					client->reply(ERR_NOTREGISTERED(_hostname));
				else
				{
					try {
						executeCommand(msg);
					}
					catch (Pass::passException& e) {
						client->reply("ERROR :Closing Link: localhost (Bad Password)\n");
						removeClient(client);
						break;
					}
					catch (...) {
						std::cerr << getTimestamp() << YELLOW "Caught unknown exception" CLEAR << std::endl;
					}
				}
			}

			/* Retrieve next command */
			rawMessage = client->retrieveMessage();	
		}
	}
}

/* Execute a command from client */
void		Server::executeCommand(const Message & msg) {
	/* Attempt to execute command */
	try
	{
		_commands.at(msg.getCommand())->execute(msg);
	}
	/* Error message if command is invalid or not supported */
	catch(std::out_of_range &e)
	{
		msg._client->reply(ERR_UNKNOWNCOMMAND(_hostname, msg._client->getNickname(), msg.getCommand()));
	}
}

/* Main server loop */
void		Server::runServer(void) {
	while (g_status == ONLINE)
	{
		/* Poll all open sockets for activity */
		if (poll(_pfds.data(), _pfds.size(), 10) < 0) {
			if (g_status == OFFLINE)	// If server is terminated through SIGINT, poll will fail
				return ;
			throw std::runtime_error("Error when attempting to poll");
		}
			
		/* Iterate through sockets to check for events */
		for (size_t i = 0; i < _pfds.size(); i++)
		{
			/* If there is an event is on the server socket, check for new connection */
			if (i == 0 && (_pfds[i].revents & POLLIN))
					handleConnections();
			else if (i > 0)
			{
				Client* client = _clients[i - 1];
				/* If there is an event on a client socket, get input */
				if (_pfds[i].revents & POLLIN)
					handleMessages(client);
				/* If there is no event, check if ping interval has passed and send PING to */
				else
				{
					if (client->getRegistration() && !client->getPingStatus() && 
						((std::time(nullptr) - client->getLastActivityTime()) > PING_INTERVAL))
					{
						client->reply(CMD_PING(_hostname, std::to_string(std::time(nullptr))));
						client->setPingStatus(true);
					}
				}
			}
		}
	}
}

/* Stop server and send shutdown message to all clients */
void		Server::stopServer(void) {
	g_status = OFFLINE;
	std::vector<Client*>::iterator it = _clients.begin();
	for (; it != _clients.end(); ++it)
	{
	Client* client = *it;
	client->reply(ERR_SHUTDOWN(client->getUsername(), client->getAddress()));
	}
}

/*******************************/
/*      Client Management      */
/*******************************/

/* Remove a client from the server */
void		Server::removeClient(Client* client) {

	/* Remove client from all channels */
	std::map<std::string, Channel *>::iterator it = _channels.begin();
	while (it != _channels.end())
	{
		/* Remove ban from user to prevent stale memory pointer from remaining in _notMembers */
		it->second->setMemberModes(client, BAN, true);
		it->second->removeMember(client);
		if (it->second->getIsEmpty())
            destroyChannel(it++->first);
		else
			++it;
	}

	/* Remove client socket from pollFD vector */
	std::vector<pollfd>::iterator it2 = _pfds.begin();
	for (; it2 != _pfds.end(); ++it2)
	{
		if (it2->fd == client->getSocket())
		{
			_pfds.erase(it2);
			break;
		}
	}

	/* Shutdown socket & delete client */
	std::vector<Client *>::iterator it3 = find(_clients.begin(), _clients.end(), client);
	if (it3 != _clients.end())
	{
		shutdown(client->getSocket(), SHUT_RDWR);
		_clients.erase(it3);
	}
}

/* Check if specified nickname is already in use on server */
bool		Server::doesNickExist(const std::string nick) const {
	std::vector<Client *>::const_iterator	it = _clients.begin();
	std::vector<Client *>::const_iterator	ite = _clients.end();

	/* Iterate through clients list and attempt to find specified nickname */
	while (it != ite)
	{
		if ((*it)->getNickname() == nick)
			return (true);
		++it;
	}
	return (false);
}

/* Find and return client pointer using given nickname */
Client*		Server::getClientPtr(const std::string &client) {
	std::vector<Client*>::iterator it = _clients.begin();
	/* Iterate through clients list and attempt to return pointer for given nickname */
	for (; it != _clients.end(); ++it)
	{
		if ((*it)->getNickname() == client)
			break;
	}
	if (it == _clients.end())
		return (nullptr);
	return *it;
}


/********************************/
/*      Channel Management      */
/********************************/

/* Create a new channel with given channel name */
void		Server::createChannel(const std::string& channel, Client* owner) {
	/* Check if channel already exists */
	if (!_channels.empty() && (_channels.find(channel) != _channels.end()))
		return;
	std::cout << getTimestamp() << GREEN "New channel created: " CLEAR << channel << std::endl << std::endl;
	_channels[channel] = new Channel(channel, owner);
}

/* Destroy channel with given channel name */
void		Server::destroyChannel(const std::string& channel) {
	/* Find correct channel in map */
	std::map<std::string, Channel *>::iterator it = _channels.find(channel);
	if (it == _channels.end())
		return;
	std::cout << getTimestamp() << YELLOW "Deleting channel: " << it->first << CLEAR << std::endl; 
	delete it->second;
	_channels.erase(it);
}

/* Check if a specified channel name already exists */
bool		Server::doesChannelExist(const std::string& channel) const {
	if (_channels.find(channel) != _channels.end())
		return (true);
	return (false);
}

/* Check if password provided for given channel is correct */
bool		Server::channelCheckPass(const std::string& channel, const std::string& pass) {
	std::map<std::string, Channel *>::iterator it = _channels.find(channel);
	/* Check if channel exists */
	if (it == _channels.end())
		return (false);
	/* Check if password match */
	if (it->second->getPassword() == pass)
		return (true);
	return (false);
}

/* Find and return channel pointer using given channel name */
Channel*	Server::getChannelPtr(const std::string& channel) {
	std::map<std::string, Channel *>::iterator it = _channels.find(channel);
	if (it == _channels.end())
		return (nullptr);
	return(it->second);
}


/********************************/
/*      Utility Functions       */
/********************************/

/* Return a formatted timestamp for current time */
const std::string	getTimestamp() {
	std::time_t time = std::time(nullptr);
	char output[50];
	int len;
	len = strftime(output, 50, "[%a %b %d %Y %X] : ", std::localtime(&time));
	output[len] = 0;
	return (std::string(output));
}
