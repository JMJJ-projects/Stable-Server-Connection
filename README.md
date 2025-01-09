# 📦 Stable Server-Client Connection and Data Structure Implementation

> *An application that establishes a stable Client-Server connection implementing a custom protocol for managing and sharing "comptines" with support for file uploads and multi-threaded connections.*

## 🌟 Highlights

- Multithreaded server for handling multiple client connections simultaneously.
- Allows clients to view, edit, upload and download "comptine" files on the server
- Utilizes data structures to manage and handle file data efficiently
- Supports concurrent client connections and file management
- Real-time updates with a catalog of available comptines.
- Robust error handling and clean client-server interaction.

## ℹ️ Overview

**Wikicomptine** is a C-based program designed to establish a stable server-client connection. The project focuses on allowing clients to interact with "comptine" files hosted on a central server. Clients can edit and add to these files, with all changes reflected in real time. The server supports multi-threading to handle multiple client requests concurrently.

The program leverages efficient data structures to handle data manipulation, ensuring that operations like adding or editing files are performed in a performant manner. The server can handle multiple client connections simultaneously, making it a scalable solution for collaborative environments.

### ✍️ Author

Developed by [John-Michael JENY JEYARAJ](https://github.com/johnmichaeljeyaraj)

## ⬇️ Installation

1. Download the source code or clone the repository.
2. Compile the server and client using the provided Makefile:

```bash
make
```

## 🚀 Usage

To start the server and client interactions:

For the Server:

1. Compile the server source code.
2. Run the server executable.
```bash
./wcp_srv
```
3. The server will listen for incoming client connections on a specified port.

For the Client:

1. Compile the client source code.
2. Run the client executable (in a separate terminal or machine to the server).
```
./wcp_clt
```
3. Follow the prompts to interact with the server, view the catalog, and select comptines.

Clients can then interact with the server, viewing and editing "comptine" files.


## 💭 Feedback and Contributing

I welcome feedback, bug reports, and feature requests! Please open an issue or start a discussion in the [Discussions tab](https://github.com/JMJJ-projects/Stable-Server-Connection/discussions).
