#!/bin/sh
g++ --std=c++11 webstash.cpp webstash.h -o webstash
g++ --std=c++11 webstash-server.cpp webstash-server.h -o webstash-server
mkdir $HOME/.webstash
mkdir $HOME/.webstash/stash
echo -e "127.0.0.1\n5002\n">$HOME/.webstash/.config
echo -e "5002\n">$HOME/.webstash/.config-server
sudo cp webstash /usr/local/bin/webstash
sudo cp webstash-server /usr/local/bin/webstash-server
rm webstash
rm webstash-server
