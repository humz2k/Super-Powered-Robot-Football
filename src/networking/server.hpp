/** @file server.hpp
 *
 * This header file defines a Server class that manages the server-side logic
 * for a multiplayer game. The class handles network communications, manages
 * connected clients, processes game state updates, and runs the game
 * simulation. It ensures a smooth and responsive game experience by
 * synchronizing the states of multiple clients and maintaining the game world
 * through a dedicated simulation. The server setup includes initializing the
 * ENet library, creating the ENet server host, and managing player connections
 * and disconnections. The server and simulation run in separate threads,
 * allowing for concurrent processing of network events and game state updates.
 *
 */

#ifndef _SPRF_SERVER_SERVER_HPP_
#define _SPRF_SERVER_SERVER_HPP_

#include "engine/engine.hpp"
#include "packet.hpp"
#include "physics/simulation.hpp"
//#include "raylib-cpp.hpp"
#include "scripting/scripting.hpp"
#include "server_params.hpp"
#include <cassert>
#include <enet/enet.h>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>

namespace SPRF {

/**
 * @brief Class representing the game server.
 *
 * This class manages the server-side logic for the multiplayer game, including
 * handling connections, receiving and sending packets, and running the game
 * simulation.
 */
class Server {
  private:
    // ScriptingManager m_scripting;

    /** @brief Server configuration parameters */
    ServerConfig config;
    /** @brief Mutex to protect server state */
    std::mutex server_mutex;
    /** @brief Flag to indicate if the server should quit */
    bool m_server_should_quit = false;
    /** @brief Thread running the server loop */
    std::thread server_thread;

    /** @brief Host address for the server */
    std::string m_host;
    /** @brief Port number for the server */
    enet_uint16 m_port;
    /** @brief Number of peers (clients) that can connect to the server */
    size_t m_peer_count;
    /** @brief Number of channels for communication */
    size_t m_channel_count;
    /** @brief Incoming bandwidth limit */
    enet_uint32 m_iband;
    /** @brief Outgoing bandwidth limit */
    enet_uint32 m_oband;

    /** @brief ENet address for the server */
    ENetAddress m_address;
    /** @brief ENet server host */
    ENetHost* m_enet_server;

    /** @brief List of player state data */
    std::vector<player_state_data> m_player_states;

    ball_state_data m_ball_state;

    /** @brief Simulation tick rate */
    enet_uint32 m_tickrate;
    /** @brief Current simulation tick */
    enet_uint32 m_tick = 0;
    /** @brief Next available player ID */
    enet_uint32 m_next_id = 0;

    enet_uint32 m_last_packet_send = 0;

    /** @brief The game simulation */
    Simulation m_simulation;

    /**
     * @brief Handles incoming packets from clients.
     *
     * This method processes client packets, updates player inputs, and sends
     * the updated state back to the client.
     *
     * @param event Pointer to the ENet event containing the packet data.
     */
    void handle_recieve(ENetEvent* event) {
        packet_header header = *(packet_header*)(event->packet->data);
        if (header.packet_type == PACKET_USER_ACTION) {
            user_action_packet client_packet(event->packet->data,
                                             event->packet->dataLength);
            PlayerBody* body = (PlayerBody*)event->peer->data;
            body->update_inputs(client_packet);
            if (enet_peer_send(event->peer, 0,
                               ping_response_packet(client_packet.ping_send)
                                   .serialize()) != 0) {
                TraceLog(LOG_ERROR, "packet send failed");
            }
        }
        enet_host_flush(m_enet_server);
    }

    /**
     * @brief Handles new client connections.
     *
     * This method initializes a new player in the simulation, assigns them an
     * ID, and sends a handshake packet to the client.
     *
     * @param event Pointer to the ENet event containing the connection data.
     */
    void handle_connect(ENetEvent* event) {
        TraceLog(LOG_INFO, "Peer Connected");
        m_player_states.push_back(player_state_data(m_next_id));
        auto player = m_simulation.create_player(m_next_id);
        player->enable();
        event->peer->data = player;
        HandshakePacket out(m_next_id, m_tickrate, enet_time_get(),
                            m_simulation.params().ball_radius);
        m_next_id++;
        ENetPacket* packet = enet_packet_create(&out, sizeof(HandshakePacket),
                                                ENET_PACKET_FLAG_RELIABLE);
        if (enet_peer_send(event->peer, 0, packet) != 0) {
            TraceLog(LOG_ERROR, "packet send failed");
        }
        enet_host_flush(m_enet_server);
    }

    /**
     * @brief Handles client disconnections.
     *
     * This method disables the player in the simulation and removes their state
     * data from the server.
     *
     * @param event Pointer to the ENet event containing the disconnection data.
     */
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

    /**
     * @brief Processes incoming ENet events.
     *
     * This method handles different types of ENet events, such as connect,
     * receive, and disconnect.
     *
     */
    void get_event() {
        ENetEvent event;
        if (enet_host_service(m_enet_server, &event, (1000 / m_tickrate)) > 0) {
            m_simulation.update(&m_tick, m_player_states, m_ball_state);
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
        if ((enet_time_get() - m_last_packet_send) >= (1000 / m_tickrate)) {
            game_state_packet packet(enet_time_get(), m_ball_state,
                                     m_player_states);
            enet_host_broadcast(m_enet_server, 0, packet.serialize());
            enet_host_flush(m_enet_server);
            m_last_packet_send = enet_time_get();
        }
    }

    /**
     * @brief Checks if the server should quit.
     *
     * @return bool True if the server should quit, false otherwise.
     */
    bool should_quit() {
        std::lock_guard<std::mutex> guard(server_mutex);
        return m_server_should_quit;
    }

    /**
     * @brief Runs the server loop.
     *
     * This method continuously processes incoming events until the server is
     * signaled to quit.
     */
    void run() {
        scripting.run_file("assets/server/scripts/on_load.lua");
        while (!should_quit()) {
            get_event();
        }
    }

    void register_scripts(){

    }

  public:
    /**
     * @brief Signals the server to quit.
     *
     * Locks `server_mutex`.
     */
    void quit() {
        TraceLog(LOG_INFO, "closing server...");
        std::lock_guard<std::mutex> guard(server_mutex);
        TraceLog(LOG_INFO, "closing server");
        m_server_should_quit = true;
    }

    /**
     * @brief Waits for the server and simulation threads to finish.
     */
    void join() {
        TraceLog(LOG_INFO, "joining server thread");
        server_thread.join();
        TraceLog(LOG_INFO, "quitting sim thread");
        m_simulation.quit();
        TraceLog(LOG_INFO, "joining simulation thread");
        m_simulation.join();
        TraceLog(LOG_INFO, "done");
    }

    /**
     * @brief Construct a new Server object.
     *
     * Initializes the server with the given configuration file, sets up the
     * ENet server host, and launches the server and simulation threads.
     *
     * @param server_config The path to the server configuration file.
     */
    Server(std::string server_config)
        : config(server_config), m_host(config.host), m_port(config.port),
          m_peer_count(config.peer_count),
          m_channel_count(config.channel_count), m_iband(config.iband),
          m_oband(config.oband), m_tickrate(config.tickrate),
          m_simulation(m_tickrate, server_config) {
        enet_address_set_host(&m_address, m_host.c_str());
        m_address.port = m_port;
        TraceLog(LOG_INFO,
                 "setting server.address.host = %s, server.address.port = %u",
                 m_host.c_str(), m_port);
        TraceLog(LOG_INFO,
                 "creating host with peer_count %lu, channel_count %lu, iband "
                 "%u, oband %u",
                 m_peer_count, m_channel_count, m_iband, m_oband);

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
        register_scripts();
    }

    /**
     * @brief Construct a new Server object.
     *
     * Initializes the server with the given configuration file, sets up the
     * ENet server host, and launches the server and simulation threads.
     *
     * @param server_config The path to the server configuration file.
     */
    Server(std::string server_config, std::string host, enet_uint16 port)
        : config(server_config), m_host(host), m_port(port),
          m_peer_count(config.peer_count),
          m_channel_count(config.channel_count), m_iband(config.iband),
          m_oband(config.oband), m_tickrate(config.tickrate),
          m_simulation(m_tickrate, server_config) {
        enet_address_set_host(&m_address, m_host.c_str());
        m_address.port = m_port;
        TraceLog(LOG_INFO,
                 "setting server.address.host = %s, server.address.port = %u",
                 m_host.c_str(), m_port);
        TraceLog(LOG_INFO,
                 "creating host with peer_count %lu, channel_count %lu, iband "
                 "%u, oband %u",
                 m_peer_count, m_channel_count, m_iband, m_oband);

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
        register_scripts();
    }

    /**
     * @brief Destructor for the Server class.
     *
     * Cleans up the ENet server host and deinitializes ENet.
     */
    ~Server() {
        enet_host_destroy(m_enet_server);
        TraceLog(LOG_INFO, "ENet server host destroyed");
    }
};

}; // namespace SPRF

#endif // _SPRF_SERVER_SERVER_HPP_