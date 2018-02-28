# Adapted from git@github.com:sol-prog/Clang-in-Docker.git

# Use the latest stable Ubuntu version
FROM ubuntu:16.04

# Install libraries and programs
RUN apt update && apt install -y \
	xz-utils \
	build-essential \
	curl \
	clang++-5.0 \
	make \
	libboost-all-dev \
	traceroute \
	wget \
	python3 \
	python3-matplotlib \
	git \
	gist \
	vim-gtk

# Start from a Bash prompt
CMD [ "/bin/bash" ]
