#ifndef _SPRF_NETWORKING_CLIENT_HPP_
#define _SPRF_NETWORKING_CLIENT_HPP_

#include "engine/engine.hpp"
#include "packet.hpp"
#include <enet/enet.h>
#include <mutex>
#include <string>
#include <thread>

namespace SPRF {

// class PlayerData{

//};

class Client : public Component {
  private:
    std::mutex m_client_mutex;
    std::string m_host;
    enet_uint16 m_port;
    ENetHost* m_client;
    ENetAddress m_address;
    ENetPeer* m_peer;
    int m_tickrate = 128;
    float m_time_per_update;
    float m_last_time;
    bool m_should_quit = false;
    std::thread m_client_thread;

    bool m_forward = false;
    bool m_backward = false;
    bool m_left = false;
    bool m_right = false;

    void reset_inputs() {
        m_forward = false;
        m_backward = false;
        m_left = false;
        m_right = false;
    }

    void update_inputs() {
        std::lock_guard<std::mutex> guard(m_client_mutex);
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
    }

    void connect() {
        m_client = enet_host_create(NULL, 1, 1, 0, 0);
        if (!m_client) {
            TraceLog(LOG_ERROR, "An error occurred while trying to create an "
                                "ENet client host!");
            exit(EXIT_FAILURE);
        }
        TraceLog(LOG_INFO, "Setting host address and port");
        enet_address_set_host(&m_address, m_host.c_str());
        m_address.port = m_port;

        TraceLog(LOG_INFO, "Creating Peer");
        m_peer = enet_host_connect(m_client, &m_address, 1, 0);
        if (m_peer == NULL) {
            TraceLog(LOG_ERROR,
                     "No available peers for initiating an ENet connection!");
            exit(EXIT_FAILURE);
        }

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
        }

        enet_host_flush(m_client);

        if (!connection_succeeded) {
            enet_peer_reset(m_peer);
            TraceLog(LOG_ERROR, "Connection failed.");
        }

        if (enet_host_service(m_client, &event, 5000) > 0 &&
            event.type == ENET_EVENT_TYPE_RECEIVE) {
            HandshakePacket* handshake = (HandshakePacket*)event.packet->data;
            TraceLog(LOG_INFO,
                     "I am player %u, server tickrate = %u, current_time = %u",
                     handshake->id, handshake->tickrate,
                     handshake->current_time);
            enet_time_set(handshake->current_time);
            m_last_time = enet_time_get();
            enet_packet_destroy(event.packet);
        } else {
            enet_peer_reset(m_peer);
            TraceLog(LOG_ERROR, "Server didn't assign an id.");
        }

        // flush (shouldn't be necessary but why not)
        enet_host_flush(m_client);
    }

    void disconnect() {
        enet_peer_disconnect(m_peer, 0);
        enet_host_flush(m_client);
        ENetEvent event;
        while (enet_host_service(m_client, &event, 3000) > 0) {
            switch (event.type) {
            case ENET_EVENT_TYPE_RECEIVE:
                break;
            case ENET_EVENT_TYPE_DISCONNECT:
                TraceLog(LOG_INFO, "Disconnection succeeded.");
                break;
            default:
                TraceLog(LOG_INFO, "unknown event recieved");
                break;
            }
            enet_packet_destroy(event.packet);
        }
        TraceLog(LOG_INFO, "destroying enet client");
        enet_host_destroy(m_client);
    }

    bool should_quit() {
        std::lock_guard<std::mutex> guard(m_client_mutex);
        return m_should_quit;
    }

    void quit() {
        std::lock_guard<std::mutex> guard(m_client_mutex);
        m_should_quit = true;
    }

    void send_input_packet() {
        std::lock_guard<std::mutex> guard(m_client_mutex);
        ClientPacket send_packet(m_forward, m_backward, m_left, m_right);
        RawClientPacket raw_packet = send_packet.get_raw();
        ENetPacket* packet = enet_packet_create(
            &raw_packet, sizeof(RawClientPacket), ENET_PACKET_FLAG_RELIABLE);
        if (enet_peer_send(m_peer, 0, packet) != 0)
            TraceLog(LOG_ERROR, "Packet send failed?");
        enet_host_flush(m_client);
        reset_inputs();
    }

    void handle_recieve(ENetEvent* event) {
        PlayerState* player_states = (PlayerState*)event->packet->data;
        int n_players = (event->packet->dataLength) / sizeof(PlayerState);
        if (player_states[0].timestamp > enet_time_get()) {
            enet_time_set(player_states[0].timestamp);
        }
        for (int i = 0; i < n_players; i++) {
            player_states[i].print();
        }
    }

    void recv_packet() {
        std::lock_guard<std::mutex> guard(m_client_mutex);
        ENetEvent event;
        if (enet_host_service(m_client, &event, 5) > 0) {
            switch (event.type) {
            case ENET_EVENT_TYPE_RECEIVE:
                handle_recieve(&event);
                enet_packet_destroy(event.packet);
                break;
            default:
                TraceLog(LOG_INFO, "Unknown Event Recieved");
                break;
            }
        }
    }

    void run_client() {
        while (!should_quit()) {
            float time = enet_time_get();
            if ((time - m_last_time) > m_time_per_update) {
                m_last_time = time;
                send_input_packet();
                recv_packet();
            }
        }
    }

  public:
    Client(std::string host, enet_uint16 port) : m_host(host), m_port(port) {
        connect();
        enet_time_set(0);
        m_last_time = enet_time_get();
        m_time_per_update = 1000.0f / (float)m_tickrate;
        m_client_thread = std::thread(&Client::run_client, this);
    }

    ~Client() {
        quit();
        m_client_thread.join();
        disconnect();
    }

    void init() {}

    void update() {
        update_inputs();
        // ClientPacket
        // packet(IsKeyPressed(KEY_W),IsKeyPressed(KEY_S),IsKeyPressed(KEY_A),IsKeyPressed(KEY_D));
    }

    void destroy() {}
};

} // namespace SPRF

#endif