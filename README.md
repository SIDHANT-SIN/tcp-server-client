# Simple HTTP Server in C for Unix

This repo has a basic HTTP server written in C for Unix systems. It also includes a simple TCP server and client to show how socket communication works.

An HTTP server listens for requests from web browsers or other clients, then responds by sending web pages, files, or data. It uses the HTTP protocol, which is the foundation of how the web works, allowing you to access websites by requesting and receiving content over the network.

---

## Contents

- `tcp_server.c` — A simple TCP server that waits for client connections and allows basic message exchange via terminal.  
- `tcp_client.c` — Connects to the TCP server and lets you send messages from your terminal.  
- `httpserv.c` — Implements a minimal HTTP/1.1 server that handles GET requests and serves files to browsers or tools like curl.

---

## How to Use

### Run TCP Server and Client

1. Compile the programs:

```bash
   gcc tcp_server.c -o tcp_server
   gcc tcp_client.c -o tcp_client
```

2. Start the server :

```bash
  ./tcp_server
```
3. In another terminal, run the client:

```bash
  ./tcp_client
```

---
### Run HTTP Serve

1. Compile the server:

```bash
  gcc httpserv.c -o httpserv
```
2. Start it:

```bash
 ./httpserv
```
3. Access it from browser:

```bash
 curl http://localhost:8080
```
