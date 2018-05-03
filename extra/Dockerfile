# Adapted from git@github.com:sol-prog/Clang-in-Docker.git

# Use the latest stable Ubuntu version
FROM ubuntu:16.04

# Copy over relevant files
COPY . /HttpClient-cpp/

# Install libraries and programs
RUN apt update && apt install -y \
	xz-utils \
	build-essential \
	curl \
	clang++-5.0 \
	make \
	#libboost-all-dev \
	libasio-dev \
	traceroute \
	wget \
	python3 \
	python3-matplotlib \
	git \
	gist \
	yorick \
	vim-gtk \
	openssl \
	libssl-dev

# Start from a Bash prompt
CMD [ "/bin/bash" ]
