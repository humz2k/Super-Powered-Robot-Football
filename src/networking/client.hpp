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

class NetworkEntity : public Component{
    public:
        raylib::Vector3 position;
        raylib::Vector3 rotation;
        raylib::Vector3 velocity;
        float health = 100;
        bool active = false;
        NetworkEntity(){}
};

/**
 * @brief Class to handle networked data of a player.
 *
 * This class manages the interpolation and buffering of networked player data
 * to provide smooth position and rotation updates.
 */
class PlayerNetworkedData {
  private:
    /** @brief Buffer pointer for the current index */
    int m_buffer_ptr = 0;
    /** @brief Time buffer for storing update times */
    float m_time_buffer[2];
    /** @brief Position buffer */
    raylib::Vector3 m_position_buffer[2];
    /** @brief Rotation buffer */
    raylib::Vector3 m_rotation_buffer[2];
    /** @brief Velocity vector */
    raylib::Vector3 m_velocity;

    bool m_active = true;

    /**
     * @brief Get the index of the last buffer.
     * @return int Index of the last buffer.
     */
    int last_buffer() {
        if (m_buffer_ptr == 0) {
            return 1;
        }
        return 0;
    }

    /**
     * @brief Get the index of the latest buffer.
     * @return int Index of the latest buffer.
     */
    int latest_buffer() { return m_buffer_ptr; }

    /**
     * @brief Get the time of the last buffer.
     * @return float Time of the last buffer.
     */
    float last_time() { return m_time_buffer[last_buffer()]; }

    /**
     * @brief Get the time of the latest buffer.
     * @return float Time of the latest buffer.
     */
    float latest_time() { return m_time_buffer[latest_buffer()]; }

    /**
     * @brief Get the position from the last buffer.
     * @return raylib::Vector3 Position from the last buffer.
     */
    raylib::Vector3 last_position() { return m_position_buffer[last_buffer()]; }

    /**
     * @brief Get the position from the latest buffer.
     * @return raylib::Vector3 Position from the latest buffer.
     */
    raylib::Vector3 latest_position() {
        return m_position_buffer[latest_buffer()];
    }

  public:
    /**
     * @brief Constructor for PlayerNetworkedData.
     *
     * Initializes the buffers to zero.
     */
    PlayerNetworkedData() {
        memset(m_time_buffer, 0, sizeof(m_time_buffer));
        memset(m_position_buffer, 0, sizeof(m_position_buffer));
        memset(m_rotation_buffer, 0, sizeof(m_rotation_buffer));
    }

    /**
     * @brief Update the buffer with new data.
     *
     * @param time Update time.
     * @param position New position.
     * @param velocity New velocity.
     * @param rotation New rotation.
     */
    void update_buffer(float time, raylib::Vector3 position,
                       raylib::Vector3 velocity, raylib::Vector3 rotation) {
        m_time_buffer[m_buffer_ptr] = time;
        m_position_buffer[m_buffer_ptr] = position;
        m_rotation_buffer[m_buffer_ptr] = rotation;
        m_velocity = velocity;
        m_buffer_ptr++;
        if (m_buffer_ptr >= 2) {
            m_buffer_ptr = 0;
        }
    }

    /**
     * @brief Get the interpolated position based on the current time.
     * @return raylib::Vector3 Interpolated position.
     */
    raylib::Vector3 position() {
        float current_time = enet_time_get();
        float lerp_amount =
            (current_time - last_time()) / (latest_time() - last_time());
        return Vector3Lerp(last_position(), latest_position(), lerp_amount);
    }

    /**
     * @brief Get the latest rotation.
     * @return raylib::Vector3 Latest rotation.
     */
    raylib::Vector3 rotation() { return m_rotation_buffer[latest_buffer()]; }

    /**
     * @brief Get the latest velocity.
     * @return raylib::Vector3 Latest velocity.
     */
    raylib::Vector3 velocity() { return m_velocity; }

    bool active(){
        return m_active;
    }

    bool active(bool new_active){
        m_active = new_active;
        return m_active;
    }
};

/**
 * @brief Class to manage the client-side networking.
 *
 * This class handles connecting to the server, sending and receiving data
 * packets, and updating player states.
 */
class Client : public Component {
  private:
    /** @brief Mutex to protect client state */
    std::mutex m_client_mutex;
    /** @brief Mutex to protect player data */
    std::mutex m_players_mutex;
    /** @brief Server host address */
    std::string m_host;
    /** @brief Server port */
    enet_uint16 m_port;
    /** @brief ENet client host */
    ENetHost* m_client;
    /** @brief ENet address */
    ENetAddress m_address;
    /** @brief ENet peer */
    ENetPeer* m_peer;
    /** @brief Tickrate of the client */
    int m_tickrate = 128;
    /** @brief Time per update */
    float m_time_per_update;
    /** @brief Time of the last update */
    float m_last_time;
    /** @brief Last tick received from the server */
    enet_uint32 m_last_tick = 0;
    /** @brief Flag indicating if the client should quit */
    bool m_should_quit = false;
    /** @brief Thread for running the client */
    std::thread m_client_thread;

    /** @brief Flag for moving forward */
    bool m_forward = false;
    /** @brief Flag for moving backward */
    bool m_backward = false;
    /** @brief Flag for moving left */
    bool m_left = false;
    /** @brief Flag for moving right */
    bool m_right = false;
    /** @brief Flag for jumping */
    bool m_jump = false;

    /** @brief Flag indicating if the client is connected */
    bool m_connected = false;

    /** @brief Array to store ping measurements */
    enet_uint32 m_pings[N_PING_AVERAGE] = {500, 500, 500, 500, 500};
    /** @brief Index for the current ping measurement */
    int m_current_ping = 0;

    int m_uprate = 64;

    /** @brief Map to store networked data of players */
    std::unordered_map<enet_uint32, PlayerNetworkedData> m_player_data;

    /** @brief Unique ID of the client player */
    enet_uint32 m_id = -1;

    std::unordered_map<enet_uint32, Entity*> m_entities;

    std::function<void(Entity*)> m_init_player;

    /**
     * @brief Add a ping measurement to the ping array.
     *
     * @param measurement The ping measurement to add.
     */
    void add_ping_measurement(enet_uint32 measurement) {
        m_pings[m_current_ping] = measurement;
        m_current_ping++;
        if (m_current_ping >= N_PING_AVERAGE) {
            m_current_ping = 0;
        }
    }

    /**
     * @brief Reset the input flags.
     */
    void reset_inputs() {
        m_forward = false;
        m_backward = false;
        m_left = false;
        m_right = false;
        m_jump = false;
    }

    /**
     * @brief Update the input flags based on the current keyboard state.
     *
     * Locks `m_client_mutex`.
     */
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
        if (IsKeyDown(KEY_SPACE) || (GetMouseWheelMove() != 0.0f)) {
            m_jump = true;
        }
    }

    /**
     * @brief Connect to the server.
     *
     * Creates the ENet host and establishes a connection to the server.
     *
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
            // this->entity()->scene()->close();
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
                m_last_time = enet_time_get();
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

    /**
     * @brief Check if the client should quit.
     *
     * Locks `m_client_mutex`.
     *
     * @return bool True if the client should quit, false otherwise.
     */
    bool should_quit() {
        std::lock_guard<std::mutex> guard(m_client_mutex);
        return m_should_quit;
    }

    /**
     * @brief Set the flag to quit the client.
     *
     * Locks `m_client_mutex`.
     */
    void quit() {
        std::lock_guard<std::mutex> guard(m_client_mutex);
        m_should_quit = true;
    }

    /**
     * @brief Send the input packet to the server.
     *
     * Collects the current input states and sends them to the server as a
     * packet.
     *
     * Locks `m_client_mutex`.
     */
    void send_input_packet() {
        std::lock_guard<std::mutex> guard(m_client_mutex);
        ClientPacket send_packet(
            m_forward, m_backward, m_left, m_right, m_jump,
            this->entity()->get_child(0)->get_component<Transform>()->rotation);
        ClientPacketRaw raw_packet = send_packet.get_raw();
        ENetPacket* packet = enet_packet_create(
            &raw_packet, sizeof(ClientPacketRaw), ENET_PACKET_FLAG_UNSEQUENCED);
        if (enet_peer_send(m_peer, 0, packet) != 0){
            enet_packet_destroy(packet);
            TraceLog(LOG_ERROR, "Packet send failed?");
        }
        enet_host_flush(m_client);
        reset_inputs();
    }

    /**
     * @brief Handle receiving a packet from the server.
     *
     * Processes the incoming packet and updates player states.
     *
     * Locks `m_players_mutex` when updating states.
     *
     * @param event Pointer to the ENetEvent containing the received packet.
     */
    void handle_recieve(ENetEvent* event) {
        PlayerStatePacket player_states(event->packet->data);
        auto header = player_states.header();

        if (header.tick <= m_last_tick) {
            TraceLog(LOG_INFO,"out of order packet...");
            return;
        }

        auto current_time = enet_time_get();
        auto this_ping = current_time - header.ping_return;
        add_ping_measurement(this_ping);

        auto send_time = current_time - (this_ping / 2);

        // TraceLog(LOG_INFO,"tick = %u",header.tick);
        std::lock_guard<std::mutex> guard(m_players_mutex);
        for (auto& i : m_player_data){
            i.second.active(false);
        }
        auto states = player_states.states();
        for (auto& i : states) {
            m_player_data[i.id].update_buffer(send_time, i.position(),
                                              i.velocity(), i.rotation());
            m_player_data[i.id].active(true);
        }
    }

    /**
     * @brief Receive a packet from the server.
     *
     * Locks `m_client_mutex`.
     */
    void recv_packet() {
        std::lock_guard<std::mutex> guard(m_client_mutex);
        ENetEvent event;
        if (enet_host_service(m_client, &event, 1000/(m_uprate)) > 0) {
            switch (event.type) {
            case ENET_EVENT_TYPE_RECEIVE:
                handle_recieve(&event);
                enet_packet_destroy(event.packet);
                break;
            default:
                TraceLog(LOG_INFO, "Unknown Event Recieved");
                break;
            }
        } else {
            TraceLog(LOG_INFO,"packet expected but failed...");
        }
    }

    /**
     * @brief Main client loop.
     *
     * Continuously sends input packets and receives updates from the server.
     */
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
    /**
     * @brief Constructor for Client.
     *
     * Initializes the client and connects to the server.
     *
     * @param host The server host address.
     * @param port The server port.
     */
    Client(std::string host, enet_uint16 port, std::function<void(Entity*)> init_player_) : m_host(host), m_port(port), m_init_player(init_player_) {
        if (!connect()) {
            TraceLog(LOG_ERROR, "Connection failed...");
            TraceLog(LOG_INFO, "destroying enet client");
            enet_host_destroy(m_client);
            return;
        }
        enet_time_set(0);
        m_last_time = enet_time_get();
        m_time_per_update = 1000.0f / (float)m_tickrate;
        m_client_thread = std::thread(&Client::run_client, this);
    }

    /**
     * @brief Destructor for Client.
     *
     * Disconnects from the server and cleans up resources.
     */
    ~Client() {
        if (!m_connected)
            return;
        quit();
        m_client_thread.join();
        disconnect();
    }

    /**
     * @brief Get the average ping.
     *
     * @return float The average ping.
     */
    float ping() {
        float total = 0;
        for (int i = 0; i < N_PING_AVERAGE; i++) {
            total += m_pings[i];
        }
        return total / (float)N_PING_AVERAGE;
    }

    /**
     * @brief Initialize the client.
     *
     * Closes the scene if not connected.
     */
    void init() {
        if (!m_connected) {
            this->entity()->scene()->close();
            return;
        }
    }

    /**
     * @brief Update the client state.
     *
     * Updates inputs, player states, and game information.
     */
    void update() {
        if (!m_connected)
            return;

        // update inputs
        update_inputs();

        // store information in game info
        game_info.ping = ping();
        game_info.rotation =
            this->entity()->get_child(0)->get_component<Transform>()->rotation;

        // update position of the client
        std::lock_guard<std::mutex> guard(m_players_mutex);
        this->entity()->get_component<Transform>()->position =
            m_player_data[m_id].position();
        this->entity()->get_component<Transform>()->position.y +=
            (PLAYER_HEIGHT / 2.0f) * 0.95;

        // update more game info (because we locked mutex)
        game_info.position =
            this->entity()->get_component<Transform>()->position;
        game_info.velocity = m_player_data[m_id].velocity();

        // loop through player data
        for (auto& i : m_player_data){
            // if this data is for us, then continue
            if (i.first == m_id){
                continue;
            }
            // if this data does not have an associated entity
            if (m_entities[i.first] == NULL){
                // create a new entity and assign it a NetworkEntity component
                auto new_entity = this->entity()->scene()->create_entity();
                new_entity->add_component<NetworkEntity>();
                m_init_player(new_entity);
                new_entity->init();
                // store in m_entities
                m_entities[i.first] = new_entity;
            }
            // grab the associated NetworkEntity of this player data
            auto network_entity = m_entities[i.first]->get_component<NetworkEntity>();
            // update its values
            network_entity->position = i.second.position();
            network_entity->rotation = i.second.rotation();
            network_entity->velocity = i.second.velocity();
            network_entity->active = i.second.active();
        }
    }

    /**
     * @brief Draw 2D elements for the client.
     */
    void draw2D() {
        if (!m_connected)
            return;
    }

    /**
     * @brief Draw debug information for the client.
     *
     * Draws player capsules and other debug information.
     */
    void draw_debug() {
        if (!m_connected)
            return;
        std::lock_guard<std::mutex> guard(m_players_mutex);
        for (auto& i : m_player_data) {
            if (i.first == m_id)
                continue;
            auto pos = i.second.position();
            DrawCapsuleWires(pos - raylib::Vector3(0, PLAYER_HEIGHT / 2.0f, 0),
                             pos + raylib::Vector3(0, PLAYER_HEIGHT / 2.0f, 0),
                             PLAYER_RADIUS, 10, 10, GREEN);
            DrawSphereWires(pos - raylib::Vector3(0, PLAYER_FOOT_OFFSET, 0),
                            PLAYER_FOOT_RADIUS, 10, 10, GREEN);
        }
        DrawGrid(100, 1);
    }

    /**
     * @brief Destroy the client.
     */
    void destroy() {}
};

} // namespace SPRF

#endif // _SPRF_NETWORKING_CLIENT_HPP_