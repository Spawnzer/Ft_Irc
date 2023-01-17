#ifndef JOIN_HPP
#define JOIN_HPP

#pragma once

/* System Includes */
#include <string>
#include <vector>

/* Local Includes */
#include "Command.hpp"

class Join : public Command
{
    public:
        /* Constructors & Destructor */
        Join(Server* server);
        ~Join();

        /* Public Member Functions */
        bool                validate(const Message& msg);
        void                execute(const Message& msg);

        bool                checkInvalidChars(const std::string& string);

    private:
        std::vector<std::string>    _channels;
        std::vector<std::string>    _passwords;

};

#endif