Chatter
=======

A chat client and server

author
------
Aaron Davis

compiling
---------
Type 'make' in the project directory.

running the program
-------------------
Run the chat_coordinator with:

./chat_coordinator

By default the coordinator runs on port 32000, unless this port is already in use,
in which case the OS will select a free port and it will be printed to the screen

Run the chat client with one of the following patterns:

./chat_client

./chat_client [server port]

./chat_client [server ip] [server port]

If the port or the server ip are not specified they default to localhost and 32000

You should never have to run ./chat_server, as this is the session server which is called from
the chat coordinator. However, be sure that a compiled version of chat_server is in the same
directory as the chat_coordinator. This should be the case if you have run make.

known issues
------------
This version of the chat spec space delimits between commands (such as Submit, etc.)
and the message and message_length. It also sends integers over the wire as chars, and converts
to and from integer format in the code. I realize both of these things could have been done
differently, but it was not clear in the spec which method was preferred. The client and server
work together as expected, but might not work with other implementations of the spec that do
not space delimit or send integers as chars over the wire.
