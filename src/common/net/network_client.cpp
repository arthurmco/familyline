#include <common/logger.hpp>
#include <common/net/game_packet_server.hpp>
#include <common/net/network_client.hpp>

using namespace familyline::net;

/**
 * Returns a future object that will be available whenever the client finishes loading
 *
 * If an error occurred, it will return the error code.
 * If not, it will return true if finished, or false if canceled.
 */
std::future<tl::expected<bool, NetResult>> NetworkClient::waitLoading()
{
    waiting_for_load_ = true;

    return std::async(std::launch::async, [this]() -> tl::expected<bool, NetResult> {
        auto& log     = LoggerService::getLogger();
        bool finished = false;
        while (!loaded_) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            std::lock_guard<std::mutex> lg(recv_queue_mtx_);

            if (recv_queue_.empty()) {
                continue;
            }

            auto& pkt = recv_queue_.front();

            if (auto p = std::get_if<Packet::NLoadingResponse>(&pkt.message); p) {
                log->write("network-client", LogType::Info, "client %lu loaded %d percent", id_, p->percent);

                if (p->percent >= 100) {
                    puts("YEAH");
                    finished = true;
                    loaded_  = true;
                }
            } else {
                log->write(
                    "network-client", LogType::Warning,
                    "received unexpected packet (id %llu, from %llu, to %llu, timestamp %llu..., "
                    "type %d )",
                    pkt.id, pkt.source_client, pkt.dest_client, pkt.timestamp, pkt.message.index());
            }

            recv_queue_.pop();
        }

        waiting_for_load_ = false;
        return tl::expected<bool, NetResult>{finished};
    });
}

/**
 * Returns a future object that will be available whenever the client is ready to
 * start the game
 */
std::future<tl::expected<bool, NetResult>> NetworkClient::waitReadyToStart()
{
    waiting_for_ready_ = true;
    assert(loaded_);

    return std::async(std::launch::async, [this]() -> tl::expected<bool, NetResult> {
        bool finished = false;
        while (!game_ready_) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            std::lock_guard<std::mutex> lg(recv_queue_mtx_);
            if (recv_queue_.empty()) {
                continue;
            }

            auto& pkt = recv_queue_.front();

            printf("received message index %zu", pkt.message.index());
            if (auto p = std::get_if<Packet::NGameStartResponse>(&pkt.message); p) {
                finished    = true;
                game_ready_ = true;
            }

            recv_queue_.pop();
        }

        waiting_for_ready_ = false;
        return tl::expected<bool, NetResult>{finished};
    });
}

/**
 * Update the data, send and receive required messages
 */
void NetworkClient::update()
{
    send_queue_mtx_.lock();
    while (!send_queue_.empty()) {
        auto timestamp = duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch());

        auto pkt = send_queue_.front();
        send_queue_.pop();

        pkt.source_client = id_;
        pkt.timestamp     = timestamp;

        fn_out_(std::move(pkt));
    }
    send_queue_mtx_.unlock();

    recv_queue_mtx_.lock();
    Packet pkt;
    while (fn_in_(pkt)) {
        auto& log = LoggerService::getLogger();

        recv_queue_.push(pkt);
    }
    recv_queue_mtx_.unlock();
}
