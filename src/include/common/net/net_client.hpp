#pragma once
/**
 * Stores all common headers regarding clients
 *
 * Copyright (C) 2021 Arthur Mendes
 */

#include <string>
#include <cstdint>

namespace familyline::net {

    
/**
 * Client information, as returned by the server
 */
struct CClientInfo {
    uint64_t id;
    std::string name;
    bool ready = false;
};

struct CurrentClientInfo {
    CClientInfo info;
    std::string token;
};


/**
 * All possible network errors
 */
enum class NetResult {
    OK,

    /// A connection error was found
    ConnectionError,

    /// The server password was wrong (not implemented)
    WrongPassword,

    /// The client failed to login into the server
    LoginFailure,

    /// The client timed out while connecting to the server
    ConnectionTimeout,

    /// Unknown server error
    ServerError,

    /// Tried to log off, but the client was already logged off
    AlreadyLoggedOff,

    /// Tried to call /connect to start the game, but not all clients
    /// were connected
    NotAllClientsConnected,

    /// Timed out while waiting for other clients to send a StartRequest packet
    /// to the server.
    NotAllClientsStarted,

    /// The server was expected to respond, but it disconnected
    UnexpectedDisconnect
};

    
}

