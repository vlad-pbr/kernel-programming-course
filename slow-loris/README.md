# Final project: Slow loris attack

From CloudFlare:

> Slowloris is an application layer attack which operates by utilizing partial HTTP requests. The attack functions by opening connections to a targeted Web server and then keeping those connections open as long as it can.
> Slowloris is not a category of attack but is instead a specific attack tool designed to allow a single machine to take down a server without using a lot of bandwidth. Unlike bandwidth-consuming reflection-based DDoS attacks such as NTP amplification, this type of attack uses a low amount of bandwidth, and instead aims to use up server resources with requests that seem slower than normal but otherwise mimic regular traffic. It falls in the category of attacks known as “low and slow” attacks. The targeted server will only have so many threads available to handle concurrent connections. Each server thread will attempt to stay alive while waiting for the slow request to complete, which never occurs. When the server’s maximum possible connections has been exceeded, each additional connection will not be answered and denial-of-service will occur.

This is a user-space program written in C which implements the slow loris attack. The program receives the following command line arguments:

```sh
./a.out <ip> <port> <num_connections> <interval>
```

Upon launch, the program spawns `num_connections` amount of worker child processes. A worker process is in charge of:
- establishing a TCP connection with `ip` on `port`
- sending an additional custom header every `interval`
- if connection is reset by remote host, reestablishing connection
- on each action taken, sending a message through message queue to main process

After spawning the workers, the main process waits for messages to arrive to the message queue and prints them out. Upon receiving an interrupt, the main process terminates the workers and quits.

Makefile command `run` runs the program with default command line arguments `127.0.0.1 80 500 5`. Additional `ARGS` argument can be supplied to the run command like so: `make run ARGS="<ip> <port> <num_connections> <interval>"`.
Makefile command `watch` displays current TCP connections. This is useful assuming you run the attack on a local HTTP server. Apache2 with default configuration is especially weak to this attack.
Makefile command `request` performs a single request to the local HTTP server and then displays the time it took to receive the response.

Using default Apache2 configuration and default command line arguments, `make request` command reports an average of 110 seconds per request.
