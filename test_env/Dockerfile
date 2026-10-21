FROM alpine AS tools

RUN apk add --no-cache iproute2 iputils tcpdump

CMD ["tail", "-f", "/dev/null"]

FROM tools AS attacker

RUN apk add --no-cache build-base linux-headers

WORKDIR /opt/ft_malcolm
COPY Makefile ./
COPY includes/ ./includes/
COPY src/ ./src/
COPY libft/ ./libft/
RUN make re

COPY test_env/attacker_entrypoint.sh /usr/local/bin/ft-malcolm-entrypoint
ENTRYPOINT ["/bin/sh", "/usr/local/bin/ft-malcolm-entrypoint"]
CMD ["tail", "-f", "/dev/null"]
