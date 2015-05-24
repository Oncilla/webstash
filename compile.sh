#!/bin/sh
g++ --std=c++11 webstash.cpp webstash.h -o webstash
g++ --std=c++11 webstash-server.cpp webstash-server.h -o webstash-server
