#!/bin/sh
# Attacker container entrypoint script

# Custom setup commands go here
echo "Initializing attack setup..."


apk update
apk add tcpdump
apk add git
apk add make
apk add gcc
apk add build-base
apk add linux-headers
cd
git clone https://github.com/qlentz/ft_malcolm.git
cd ./ft_malcolm && make

# Keep the container running
tail -f /dev/null
