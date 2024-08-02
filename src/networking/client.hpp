/** @file client.hpp
 *
 * PlayerNetworkedData for lerping recieved ticks. Client for client side
 * networking.
 *
 */

#ifndef _SPRF_NETWORKING_CLIENT_HPP_
#define _SPRF_NETWORKING_CLIENT_HPP_

#include "engine/engine.hpp"
#include "packet.hpp"
#include "player_stats.hpp"
#include <enet/enet.h>
#include <functional>
#include <mutex>
#include <string>
#include <thread>

#define N_PING_AVERAGE (5)

namespace SPRF {

class NetworkEntity : public Component {
  public:
    raylib::Vector3 position;
    raylib::Vector3 rotation;
    raylib::Vector3 velocity;
    float health = 100;
    bool active = false;
    NetworkEntity() {}
};

class Client : public Component {
  private:
    std::mutex m_quit_mutex;
    std::string m_host;
    enet_uint16 m_port;
    ENetHost* m_client;
    ENetAddress m_address;
    ENetPeer* m_peer;
    int m_tickrate = 100;
    bool m_should_quit = false;
    std::thread m_client_thread;

    // inputs
    bool m_forward = false;
    bool m_backward = false;
    bool m_left = false;
    bool m_right = false;
    bool m_jump = false;

    bool m_connected = false;

    // debug information
    size_t sends = 0;
    float send_delta = 0;
    size_t recieves = 0;
    size_t loops = 0;
    float recieve_delta = 0;
    float last_recieve = 0;

    /** @brief Array to store ping measurements */
    enet_uint32 m_pings[N_PING_AVERAGE] = {500, 500, 500, 500, 500};
    /** @brief Index for the current ping measurement */
    int m_current_ping = 0;

    game_state_packet m_last_game_state;

    enet_uint32 m_id = -1;

    std::function<void(Entity*)> m_init_player;

    void add_ping_measurement(enet_uint32 measurement) {
        m_pings[m_current_ping] = measurement;
        m_current_ping++;
        if (m_current_ping >= N_PING_AVERAGE) {
            m_current_ping = 0;
        }
    }

    void reset_inputs() {
        m_forward = false;
        m_backward = false;
        m_left = false;
        m_right = false;
        m_jump = false;
    }

    void update_inputs() {
        if (IsKeyDown(KEY_W)) {
            m_forward = true;
        }
        if (IsKeyDown(KEY_S)) {
            m_backward = true;
        }
        if (IsKeyDown(KEY_A)) {
            m_left = true;
        }
        if (IsKeyDown(KEY_D)) {
            m_right = true;
        }
        if (IsKeyDown(KEY_SPACE) || (GetMouseWheelMove() != 0.0f)) {
            m_jump = true;
        }
    }

    /**
     * @brief Connect to the server.
     * @return int 0 if connection failed, 1 if succeeded.
     */
    int connect() {
        game->loading_screen.draw(0, "Creating ENet host...");

        m_client = enet_host_create(NULL, 1, 1, 0, 0);
        if (!m_client) {
            TraceLog(LOG_ERROR, "An error occurred while trying to create an "
                                "ENet client host!");
            exit(EXIT_FAILURE);
        }

        game->loading_screen.draw(0.1, "Setting host address and port...");

        TraceLog(LOG_INFO, "Setting host address and port");
        enet_address_set_host(&m_address, m_host.c_str());
        m_address.port = m_port;

        game->loading_screen.draw(0.2, "Creating peer...");

        TraceLog(LOG_INFO, "Creating Peer");
        m_peer = enet_host_connect(m_client, &m_address, 1, 0);
        if (m_peer == NULL) {
            TraceLog(LOG_ERROR,
                     "No available peers for initiating an ENet connection!");
            exit(EXIT_FAILURE);
        }

        game->loading_screen.draw(0.3, "Waiting for server response...");

        TraceLog(LOG_INFO, "Waiting for server response");

        bool connection_succeeded = false;

        ENetEvent event;
        for (int i = 0; i < 10; i++) {
            if (enet_host_service(m_client, &event, 500) > 0 &&
                event.type == ENET_EVENT_TYPE_CONNECT) {
                TraceLog(LOG_INFO, "Connection succeeded.");
                connection_succeeded = true;
                break;
            }
            game->loading_screen.draw(0.3 + 0.05 * (float)i,
                                      "Waiting for server response...");
        }

        enet_host_flush(m_client);

        if (!connection_succeeded) {
            enet_peer_reset(m_peer);
            TraceLog(LOG_ERROR, "Connection failed.");
            return 0;
        }

        game->loading_screen.draw(0.8, "Waiting for handshake...");
        bool handshake_succeeded = false;
        for (int i = 0; i < 10; i++) {
            if (enet_host_service(m_client, &event, 500) > 0 &&
                event.type == ENET_EVENT_TYPE_RECEIVE) {
                HandshakePacket* handshake =
                    (HandshakePacket*)event.packet->data;
                TraceLog(
                    LOG_INFO,
                    "I am player %u, server tickrate = %u, current_time = %u",
                    handshake->id, handshake->tickrate,
                    handshake->current_time);
                enet_time_set(handshake->current_time);
                m_id = handshake->id;
                enet_packet_destroy(event.packet);
                handshake_succeeded = true;
                break;
            }
            game->loading_screen.draw(0.8 + 0.02 * (float)i,
                                      "Waiting for handshake...");
        }
        if (!handshake_succeeded) {
            enet_peer_reset(m_peer);
            TraceLog(LOG_ERROR, "Server didn't assign an id.");
            return 0;
        }
        // flush (shouldn't be necessary but why not)
        enet_host_flush(m_client);
        game->loading_screen.draw(1, "Connected!");
        m_connected = true;
        return 1;
    }

    /**
     * @brief Disconnect from the server.
     *
     * Sends a disconnect packet to the server and waits for a response.
     *
     * If no response is recieved, forcefully close the connection. Otherwise,
     * close gracefully.
     */
    void disconnect() {
        game->loading_screen.draw(0, "Disconnecting Peer...");
        enet_peer_disconnect(m_peer, 0);
        enet_host_flush(m_client);
        ENetEvent event;
        game->loading_screen.draw(0.05, "Waiting for server response...");
        bool disconnect_succeeded = false;
        for (int i = 0; i < 10; i++) {
            if (enet_host_service(m_client, &event, 500) > 0) {
                switch (event.type) {
                case ENET_EVENT_TYPE_RECEIVE:
                    enet_packet_destroy(event.packet);
                    break;
                case ENET_EVENT_TYPE_DISCONNECT:
                    TraceLog(LOG_INFO, "Disconnection succeeded.");
                    disconnect_succeeded = true;
                    break;
                default:
                    TraceLog(LOG_INFO, "unknown event recieved");
                    break;
                }
                if (disconnect_succeeded) {
                    break;
                }
            }
            game->loading_screen.draw(0.05 + 0.1 * (float)i,
                                      "Waiting for server response...");
        }
        game->loading_screen.draw(0.95, "Destroying ENet client...");
        TraceLog(LOG_INFO, "destroying enet client");
        enet_host_destroy(m_client);
        game->loading_screen.draw(0.1, "Disconnected!");
    }

    bool should_quit() {
        std::lock_guard<std::mutex> guard(m_quit_mutex);
        return m_should_quit;
    }

    void quit() {
        std::lock_guard<std::mutex> guard(m_quit_mutex);
        m_should_quit = true;
    }

    void send_input_packet() {
        user_action_packet send_packet(
            m_forward, m_backward, m_left, m_right, m_jump,
            this->entity()->get_child(0)->get_component<Transform>()->rotation);
        ENetPacket* packet = send_packet.serialize();
        if (enet_peer_send(m_peer, 0, packet) != 0) {
            enet_packet_destroy(packet);
            TraceLog(LOG_ERROR, "Packet send failed?");
        }
        enet_host_flush(m_client);
        reset_inputs();
    }

    /** TODO: Lerp of some kind. */
    void handle_recieve(ENetEvent* event) {
        packet_header header = *(packet_header*)(event->packet->data);
        if (header.packet_type == PACKET_PING_RESPONSE) {
            ping_response_packet tmp(event->packet->data,
                                     event->packet->dataLength);
            add_ping_measurement(enet_time_get() - tmp.ping_return);
            return;
        }
        if (header.packet_type == PACKET_GAME_STATE) {
            recieves++;
            recieve_delta += enet_time_get() - last_recieve;
            game_info.recieve_delta = enet_time_get() - last_recieve;
            last_recieve = enet_time_get();
            game_state_packet game_state_update(event->packet->data,
                                                event->packet->dataLength);
            m_last_game_state = game_state_update;

            // need to have some kind of lerping here? I think...
        }
    }

    void recv_packet() {
        ENetEvent event;
        if (enet_host_service(m_client, &event, 2) > 0) {
            switch (event.type) {
            case ENET_EVENT_TYPE_RECEIVE:
                handle_recieve(&event);
                enet_packet_destroy(event.packet);
                break;
            default:
                TraceLog(LOG_ERROR, "Unknown Event Recieved");
                break;
            }
        }
    }

    void run_client() {
        float last_time = enet_time_get();
        last_recieve = enet_time_get();
        while (!should_quit()) {
            float time = enet_time_get();
            if ((time - last_time) >= (1000.0f / (float)m_tickrate)) {
                send_delta += time - last_time;
                game_info.send_delta = time - last_time;
                last_time = time;
                send_input_packet();
                sends++;
            }

            recv_packet();
            loops++;
        }
        TraceLog(LOG_INFO,
                 "sends: %lu, mean send delta: %g, recvs: %lu, mean recieve "
                 "delta: %g, loops: %lu",
                 sends, send_delta / ((float)sends), recieves,
                 recieve_delta / ((float)recieves), loops);
    }

  public:
    /**
     * @brief Constructor for Client.
     *
     * Initializes the client and connects to the server.
     *
     * @param host The server host address.
     * @param port The server port.
     */
    Client(std::string host, enet_uint16 port,
           std::function<void(Entity*)> init_player_)
        : m_host(host), m_port(port), m_init_player(init_player_) {
        if (!connect()) {
            TraceLog(LOG_ERROR, "Connection failed...");
            TraceLog(LOG_INFO, "destroying enet client");
            enet_host_destroy(m_client);
            return;
        }
        enet_time_set(0);
        m_client_thread = std::thread(&Client::run_client, this);
    }

    ~Client() {
        if (!m_connected)
            return;
        quit();
        m_client_thread.join();
        disconnect();
    }

    float ping() {
        float total = 0;
        for (int i = 0; i < N_PING_AVERAGE; i++) {
            total += m_pings[i];
        }
        return total / (float)N_PING_AVERAGE;
    }

    void init() {
        if (!m_connected) {
            this->entity()->scene()->close();
            return;
        }
    }

    void update() {
        if (!m_connected)
            return;

        // update inputs
        update_inputs();

        if (m_last_game_state.states.size() > 0) {
            this->entity()->get_component<Transform>()->position =
                m_last_game_state.states[0].position();
        }

        // store information in game info
        game_info.ping = ping();
        game_info.rotation =
            this->entity()->get_child(0)->get_component<Transform>()->rotation;
    }

    void draw2D() {
        if (!m_connected)
            return;
    }

    void draw_debug() {
        if (!m_connected)
            return;
    }

    void destroy() {}
};

} // namespace SPRF

#endif // _SPRF_NETWORKING_CLIENT_HPP_