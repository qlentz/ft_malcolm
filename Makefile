NAME 	= ft_malcolm

SRCS	= src/mac_addr.c src/arp.c src/main.c src/interface.c src/net.c \
		  src/reply.c src/setup.c

OBJS	= $(SRCS:.c=.o)

DEPS	= $(OBJS:.o=.d)

LIBFT	= ./libft

LIBFT_A	= $(LIBFT)/libft.a

CC		= gcc

CFLAGS	= -Wall -Wextra -Werror -I. -I includes/ -I $(LIBFT)

RM		= rm -f

all:		$(NAME)

%.o:		%.c
			$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

$(OBJS):	| $(LIBFT)/Makefile

$(LIBFT)/Makefile:
			git submodule update --init --recursive -- $(LIBFT)

$(NAME):	$(OBJS) $(LIBFT_A)
			$(CC) $(CFLAGS) -o $@ $^

$(LIBFT_A):	$(LIBFT)/Makefile FORCE
			@$(MAKE) -C $(LIBFT)

lib:		$(LIBFT_A)

FORCE:

check:      all
			python3 tests/check_mandatory.py
			python3 tests/check_interfaces.py
			python3 tests/check_build.py

check_kernel: all
			python3 tests/check_interfaces.py --kernel

env_up:
			docker compose -f test_env/docker-compose.yml up --build -d

env_down:
			docker compose -f test_env/docker-compose.yml down

clean:
			$(RM) $(OBJS) $(DEPS)
			@if test -f $(LIBFT)/Makefile; then $(MAKE) -C $(LIBFT) clean; fi


fclean:		clean
			@if test -f $(LIBFT)/Makefile; then $(MAKE) -C $(LIBFT) fclean; fi
			$(RM) $(NAME)

re:
			@$(MAKE) fclean
			@$(MAKE) all

-include $(DEPS)

.PHONY: all lib FORCE check check_kernel clean fclean re env_up env_down
