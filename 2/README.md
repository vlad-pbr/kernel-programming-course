# Assigment 2

Write a C program which acts as a download server. It should be able to receive keyboard input for commands and download files "simultaneously" - using sockets and polling. Program can not spawn additional processes. The following commands should be supported:

- leave: closes server and quits
- show: shows download progress in the following format: [download id] [download url] [bytes downloaded]
- stop [id]: stops download by download id
- start [url] [path]: queries server by url for file download and stores file to path

Makefile commands `run`, `command` and `send` are provided as examples of how the program can be used.
