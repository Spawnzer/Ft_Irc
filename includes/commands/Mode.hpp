#ifndef MODE_HPP
#define MODE_HPP

#pragma once

/* System Includes */
#include <string>

/* Local Includes */
#include "Command.hpp"

class Mode : public Command
{
    public:
        /* Constructors & Destructor */
        Mode();
        ~Mode();

        /* Public Member Functions */
        bool                validate(const Message& msg);
        const std::string&  execute(const Message& msg);

    private:

};

#endif