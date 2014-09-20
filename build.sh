#!/bin/bash

g++ -D_DARWIN_UNLIMITED_SELECT=1 -o client chatClient.cpp Reader.cpp
g++ -D_DARWIN_UNLIMITED_SELECT=1 -o server chatServer.cpp Reader.cpp
