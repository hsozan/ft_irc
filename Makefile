NAME		= Ircserv

CC			= g++ -std=c++98
FLAGS		= -Wall -Wextra -Werror

SRC			=	main.cpp src/Server.cpp src/Client.cpp src/Channel.cpp src/SrvUtils.cpp src/ChUtils.cpp src/Commands/CommandParser.cpp \
				src/Commands/PrivMsg.cpp src/Commands/Quit.cpp src/Commands/User.cpp src/Commands/Join.cpp \
				src/Commands/Part.cpp src/Commands/Mode.cpp src/Commands/Kick.cpp src/Commands/Notice.cpp \
				src/Commands/Pass.cpp src/Commands/Nick.cpp src/Commands/Cap.cpp src/Bot.cpp \

OBJ			= $(SRC:src/%.cpp=obj/%.o)
#OBJCOM		= $(SRC:src/Commands/%.cpp=obj/%.o)

all		: $(NAME)

$(NAME)	: obj $(OBJ)
	@$(CC) $(FLAGS) obj/*.o obj/Commands/*.o main.cpp -o $(NAME)
	@echo "\033[32mIRCserv \033[0m\033[1;32mCompilation OK\033[0m"

obj		:
	mkdir -p obj
	mkdir -p obj/Commands

obj/%.o	: src/%.cpp
	$(CC) $(FLAGS) -c $< -o $@

clean	:
	@rm -rf log.log
	@rm -rf error.log
	@rm -rf obj
	@echo "\033[31mIRCserv objects files removed\033[0m"

fclean	: clean
	@rm -rf $(NAME)
	@echo "\033[31mIRCserv removed\033[0m"

re		: fclean all