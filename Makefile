NAME 	= ft_malcolm

SRCS	= src/main.c

OBJS	= $(SRCS:.c=.o)

LIBFT	= ./libft

CC		= gcc

CFLAGS	= -Wall -Wextra -Werror -I. -I includes/

RM		= rm -f

.c.o:
			${CC} ${CFLAGS} -c $< -o ${<:.c=.o}

all:		lib $(NAME)

$(NAME):	$(OBJS)
			$(CC) $(CFLAGS) -L $(LIBFT) -o $@ $^ -lft

lib:
			@make -sC $(LIBFT)

env_up:
			docker compose -f test_env/docker-compose.yml up -d

env_down:
			docker compose -f test_env/docker-compose.yml down

clean:		
			$(RM) $(OBJS)
			@make -C $(LIBFT) clean


fclean:		clean
			@make -C $(LIBFT) fclean
			$(RM) $(NAME)

re:			fclean $(NAME)


.PHONY: all clean fclean re env_up env_down
