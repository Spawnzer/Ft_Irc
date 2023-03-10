#--------------------------------#
#    Name and file information   #
#--------------------------------#
NAME			:= ircserv


CPP_FILES		:=	main.cpp \
					Channel.cpp \
					Client.cpp \
					Message.cpp \
					Server.cpp \
					commands/Nick.cpp \
					commands/User.cpp \
					commands/Quit.cpp \
					commands/Join.cpp \
					commands/Pass.cpp \
					commands/Mode.cpp \
					commands/Away.cpp \
					commands/Privmsg.cpp \
					commands/List.cpp \
					commands/Topic.cpp \
					commands/Ping.cpp \
					commands/Pong.cpp \
					commands/Part.cpp \
					commands/Invite.cpp \
					commands/Notice.cpp \
					commands/Whois.cpp	\
					commands/Who.cpp	\
					commands/Kick.cpp \
					commands/Names.cpp \
					commands/Shutdown.cpp



INC_FILES		:=	defines.h \
					Channel.hpp \
					Client.hpp \
					Message.hpp \
					Server.hpp \
					replies.h \
					commands/Nick.hpp \
					commands/User.hpp \
					commands/Quit.hpp \
                    commands/Pass.hpp \
					commands/Join.hpp \
                    commands/Mode.hpp \
                    commands/Away.hpp \
                    commands/Privmsg.hpp \
                    commands/List.hpp \
                    commands/Topic.hpp \
                    commands/Ping.hpp \
                    commands/Pong.hpp \
                    commands/Part.hpp \
					commands/Invite.hpp \
                    commands/Notice.hpp \
                    commands/Whois.hpp	\
                    commands/Who.hpp \
                    commands/Kick.hpp \
                    commands/Names.hpp \
					commands/Shutdown.hpp


#---------------------------------------------------------#
#   Directory information and object directory building   #
#---------------------------------------------------------#

BOTS_DIR		:= ./bots

INC_DIR			:= ./includes
INCS			= $(addprefix $(INC_DIR)/, $(INC_FILES))

SRC_DIR			= ./srcs
SRCS			= $(addprefix $(SRC_DIR)/, $(CPP_FILES))

OBJ_DIR			= ./obj
OBJS			= $(addprefix $(OBJ_DIR)/, $(CPP_FILES:.cpp=.o))

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp $(INCS)
	@mkdir -p $(@D)
	@echo Compiling $@
	@$(CC) $(CFLAGS) -o $@ -c $<

#--------------------------------#
#  Compiler settings and flags   #
#--------------------------------#
CC				= c++
RM				= rm -rf
CFLAGS			= -Wall -Wextra -Werror -Wshadow -Wno-shadow -std=c++98 -I$(INC_DIR) -g

#--------------------------------#
#   Makefile rules and targets   #
#--------------------------------#

all:			$(NAME)
				@echo Compiled executable $(NAME).

bots:			
				@echo Launching Bots..
				@make -s run -C $(BOTS_DIR)

$(NAME):		$(OBJS)
				@$(CC) $(CFLAGS) -o $(NAME) $(OBJS)

clean:			
				@$(RM) $(OBJ_DIR)
				@echo Clean complete.
clean_bots:			
				@make -s fclean -C $(BOTS_DIR)

fclean:			clean clean_bots
				@$(RM) $(NAME)
				@echo Full clean complete.

re:				fclean $(NAME)

.PHONY:			all bots clean clean_bots fclean re 