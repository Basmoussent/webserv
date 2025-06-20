NAME = webserv

CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98 -Iinc -g3

SRCS = main.cpp \
       srcs/handler/Request.cpp \
       srcs/handler/Handler.cpp \
       srcs/parsingConfig/ConfigParser.cpp \
       srcs/parsingConfig/ConfigUtils.cpp \
       srcs/parsingConfig/ConfigValid.cpp \
       srcs/network/SocketHandler.cpp \
       srcs/handler/Server.cpp \
       srcs/network/PollManager.cpp

BUILD_DIR = build
OBJS = $(SRCS:%.cpp=$(BUILD_DIR)/%.o)
OBJS := $(OBJS:%.cc=$(BUILD_DIR)/%.o)

all: $(NAME)

$(BUILD_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: %.cc
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(NAME) $(OBJS)

clean:
	rm -rf $(BUILD_DIR)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re
