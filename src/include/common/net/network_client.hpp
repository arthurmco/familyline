#pragma once

/**
 * Network client class
 *
 * Contains network lower-level for each client (you usually call them players,
 * *expletive* if you are losing to them, or noobs if you are losing and they
 * are your allies)
 *
 * They are used to create the NetworkPlayer class, which will handle high level
 * things
 *
 * Copyright (C) 2021 Arthur Mendes
 */

#include <common/net/net_common.hpp>
#include <common/net/net_client.hpp>
#include <cstdint>
#include <functional>
#include <future>
#include <queue>
#include <mutex>
#include <tl/expected.hpp>

namespace familyline::net
{

/**
 * The network client
 *
 * The network client is where you will send and receive packets (depending on its configuration)
 * You cannot use the GamePacketServer directly, so you will need to use this
 *
 * Also, this class uses the GamePacketServer, so it should have a lifetime equal or less than
 * it.
 *
 */
class NetworkClient
{
    using FnClientOut = std::function<void(Packet&&)>;
    using FnClientIn  = std::function<bool(Packet&)>;

private:
    // the client will call this function when it needs to send a packet
    FnClientOut fn_out_;

    // the client will call this function when it needs to receive a packet
    // you return true if you have a packet, false if you do not
    FnClientIn fn_in_;

    // the client ID
    uint64_t id_;

    std::queue<Packet> send_queue_; 
    std::mutex send_queue_mtx_;
    std::queue<Packet> recv_queue_; 
    std::mutex recv_queue_mtx_;

    std::atomic<bool> loaded_ = false;
    std::atomic<bool> game_ready_ = false;

    std::atomic<bool> waiting_for_load_ = false;
    std::atomic<bool> waiting_for_ready_ = false;
    
public:
    NetworkClient(uint64_t id, FnClientOut&& fn_out, FnClientIn&& fn_in)
        : id_(id), fn_out_(fn_out), fn_in_(fn_in)
    {}

    NetworkClient(NetworkClient&& c) {
        this->fn_out_ = c.fn_out_;
        this->fn_in_ = c.fn_in_;
        this->id_ = c.id_;
        this->send_queue_ = std::move(c.send_queue_);
        this->recv_queue_ = std::move(c.recv_queue_);
    }

    /// The move assignment operator and move constructor are needed, because, without them
    /// the copy assignment would be used in some places, and the default move constr would try
    /// to copy the mutex. This does not work for obvious reasons: it would be like copying
    /// a ticket.
    NetworkClient& operator=(NetworkClient&& c) {
        this->fn_out_ = c.fn_out_;
        this->fn_in_ = c.fn_in_;
        this->id_ = c.id_;
        this->send_queue_ = std::move(c.send_queue_);
        this->recv_queue_ = std::move(c.recv_queue_);
        
        return *this;
    }

    
    /**
     * Returns a future object that will be available whenever the client finishes loading
     *
     * If an error occurred, it will return the error code.
     * If not, it will return true if finished, or false if canceled.
     */
    std::future<tl::expected<bool, NetResult>> waitLoading();

    /**
     * Returns a future object that will be available whenever the client is ready to
     * start the game
     *
     * The value will be true if finished, or false if canceled.
     */
    std::future<tl::expected<bool, NetResult>> waitReadyToStart();

    uint64_t id() const { return id_; }
    
    /**
     * Stops waiting for the game to load
     */
    void stopWaitLoading();
    
    /**
     * Update the data, send and receive required messages
     */
    void update();
   
};

}  // namespace familyline::net
