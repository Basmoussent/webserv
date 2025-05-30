NAME = webserv
CC = c++
CFLAGS = -Wall -Wextra -Werror -std=c++98

SRC_DIR = srcs
OBJ_DIR = obj

SRCS =	./main.cpp \
		./$(SRC_DIR)/ConfigParser.cpp \
		./$(SRC_DIR)/ConfigValid.cpp \
		./$(SRC_DIR)/ConfigUtils.cpp

OBJS = $(SRCS:.cpp=.o)
OBJS := $(OBJS:./%=$(OBJ_DIR)/%)

all: $(NAME)

$(NAME): $(OBJ_DIR) $(OBJS)
	$(CC) $(CFLAGS) -o $(NAME) $(OBJS)

$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

clean:
	rm -rf $(OBJ_DIR)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re
