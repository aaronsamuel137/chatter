#!/bin/bash

g++ -D_DARWIN_UNLIMITED_SELECT=1 -o chat_client chat_client.cpp Reader.cpp
g++ -D_DARWIN_UNLIMITED_SELECT=1 -o chat_coordinator chat_coordinator.cpp Reader.cpp
g++ -D_DARWIN_UNLIMITED_SELECT=1 -o chat_server chat_server.cpp Reader.cpp
