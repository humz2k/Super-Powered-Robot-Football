#ifndef _SPRF_SERVER_SERVER_HPP_
#define _SPRF_SERVER_SERVER_HPP_

#include "packet.hpp"
#include "raylib-cpp.hpp"
#include "simulation.hpp"
#include <cassert>
#include <enet/enet.h>
#include <mutex>
#include <ode/ode.h>
#include <string>
#include <thread>
#include <unordered_map>

namespace SPRF {

class Server {
  private:
    std::mutex server_mutex;
    bool m_server_should_quit = false;
    std::thread server_thread;

    std::string m_host;
    enet_uint16 m_port;
    size_t m_peer_count;
    size_t m_channel_count;
    enet_uint32 m_iband;
    enet_uint32 m_oband;

    ENetAddress m_address;
    ENetHost* m_enet_server;

    // int m_n_players = 0;
    std::vector<PlayerState> m_player_states;

    enet_uint32 m_tickrate;
    enet_uint32 m_tick = 0;
    enet_uint32 m_next_id = 0;

    Simulation m_simulation;

    void handle_recieve(ENetEvent* event) {
        RawClientPacket* in_packet = (RawClientPacket*)event->packet->data;
        ClientPacket client_packet(*in_packet);
        PlayerBody* body = (PlayerBody*)event->peer->data;
        body->update_inputs(client_packet);
        enet_uint32 timestamp = enet_time_get();
        PlayerStatePacket state_packet(
            m_tick, timestamp, client_packet.ping_send, m_player_states);
        ENetPacket* packet = enet_packet_create(
            state_packet.raw, state_packet.size, ENET_PACKET_FLAG_UNSEQUENCED);
        assert(enet_peer_send(event->peer, 0, packet) == 0);
        enet_host_flush(m_enet_server);
    }

    void handle_connect(ENetEvent* event) {
        TraceLog(LOG_INFO, "Peer Connected");
        m_player_states.push_back(PlayerState(m_next_id));
        auto player = m_simulation.create_player(m_next_id);
        player->enable();
        event->peer->data = player;
        HandshakePacket out(m_next_id, m_tickrate, enet_time_get());
        m_next_id++;
        ENetPacket* packet = enet_packet_create(&out, sizeof(HandshakePacket),
                                                ENET_PACKET_FLAG_UNSEQUENCED);
        assert(enet_peer_send(event->peer, 0, packet) == 0);
    }

    void handle_disconnect(ENetEvent* event) {
        PlayerBody* player = (PlayerBody*)event->peer->data;
        player->disable();
        TraceLog(LOG_INFO, "ID %d disconnected", player->id());
        int to_delete = -1;
        for (int i = 0; i < m_player_states.size(); i++) {
            if (m_player_states[i].id == player->id()) {
                to_delete = i;
                break;
            }
        }
        assert(to_delete >= 0);
        m_player_states.erase(m_player_states.begin() + to_delete);
    }

    void get_event() {
        ENetEvent event;
        if (enet_host_service(m_enet_server, &event, 1000) > 0) {
            m_simulation.update(&m_tick, m_player_states);
            switch (event.type) {
            case ENET_EVENT_TYPE_CONNECT:
                handle_connect(&event);
                break;
            case ENET_EVENT_TYPE_RECEIVE:
                handle_recieve(&event);
                enet_packet_destroy(event.packet);
                break;
            case ENET_EVENT_TYPE_DISCONNECT:
                TraceLog(LOG_INFO, "Peer Disconnected");
                handle_disconnect(&event);
                break;
            default:
                TraceLog(LOG_INFO, "Got Unknown Event");
                break;
            }
        }
        enet_host_flush(m_enet_server);
    }

    bool should_quit() {
        std::lock_guard<std::mutex> guard(server_mutex);
        return m_server_should_quit;
    }

    void run() {
        while (!should_quit()) {
            get_event();
        }
    }

  public:
    void quit() {
        std::lock_guard<std::mutex> guard(server_mutex);
        m_server_should_quit = true;
    }

    void join() {
        server_thread.join();
        m_simulation.quit();
        m_simulation.join();
    }

    Server(std::string host, enet_uint16 port, size_t peer_count = 4,
           size_t channel_count = 2, enet_uint32 iband = 0,
           enet_uint32 oband = 0, enet_uint32 tickrate = 64)
        : m_host(host), m_port(port), m_peer_count(peer_count),
          m_channel_count(channel_count), m_iband(iband), m_oband(oband),
          m_tickrate(tickrate), m_simulation(m_tickrate) {
        if (enet_initialize() != 0) {
            TraceLog(LOG_ERROR, "Initializing Enet failed");
            exit(EXIT_FAILURE);
        }
        TraceLog(LOG_INFO, "Enet initialized");
        enet_address_set_host(&m_address, host.c_str());
        m_address.port = port;
        TraceLog(LOG_INFO,
                 "setting server.address.host = %s, server.address.port = %u",
                 host.c_str(), port);
        TraceLog(LOG_INFO,
                 "creating host with peer_count %lu, channel_count %lu, iband "
                 "%u, oband %u",
                 peer_count, channel_count, iband, oband);

        m_enet_server = enet_host_create(&m_address, m_peer_count,
                                         m_channel_count, m_iband, m_oband);
        if (m_enet_server == NULL) {
            TraceLog(LOG_ERROR, "Create an ENet server host failed");
            exit(EXIT_FAILURE);
        }
        TraceLog(LOG_INFO, "ENet server host created");
        enet_time_set(0);
        server_thread = std::thread(&SPRF::Server::run, this);
        m_simulation.launch();
    }

    ~Server() {
        enet_host_destroy(m_enet_server);
        TraceLog(LOG_INFO, "ENet server host destroyed");
        enet_deinitialize();
        TraceLog(LOG_INFO, "Enet deinitialized");
    }
};

}; // namespace SPRF

#endif