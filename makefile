NAME = webserv
CC = c++
CFLAGS = -Wall -Wextra -Werror -std=c++98

SRCS = main.cpp \
       srcs/parsingConfig/ConfigParser.cpp \
       srcs/parsingConfig/ConfigUtils.cpp \
       srcs/parsingConfig/ConfigValid.cpp \
       srcs/network/SocketHandler.cpp \
       srcs/network/PollManager.cpp 
       #srcs/network/Connection.cpp

OBJS = $(SRCS:%.cpp=obj/%.o)

all: $(NAME)

obj/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(NAME): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(NAME)

clean:
	rm -rf obj

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re
