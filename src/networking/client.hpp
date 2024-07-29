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

class PlayerNetworkedData{
    private:
        int m_buffer_ptr = 0;
        float m_time_buffer[2];
        raylib::Vector3 m_position_buffer[2];
        raylib::Vector3 m_rotation_buffer[2];

        int last_buffer(){
            if (m_buffer_ptr == 0){
                return 1;
            }
            return 0;
        }

        int latest_buffer(){
            return m_buffer_ptr;
        }

        float last_time(){
            return m_time_buffer[last_buffer()];
        }

        float latest_time(){
            return m_time_buffer[latest_buffer()];
        }

        raylib::Vector3 last_position(){
            return m_position_buffer[last_buffer()];
        }

        raylib::Vector3 latest_position(){
            return m_position_buffer[latest_buffer()];
        }

    public:
        PlayerNetworkedData(){
            memset(m_time_buffer,0,sizeof(m_time_buffer));
            memset(m_position_buffer,0,sizeof(m_position_buffer));
            memset(m_rotation_buffer,0,sizeof(m_rotation_buffer));
        }

        void update_buffer(float time, raylib::Vector3 position, raylib::Vector3 rotation){
            m_time_buffer[m_buffer_ptr] = time;
            m_position_buffer[m_buffer_ptr] = position;
            m_rotation_buffer[m_buffer_ptr] = rotation;
            m_buffer_ptr++;
            if (m_buffer_ptr >= 2){
                m_buffer_ptr = 0;
            }
        }

        raylib::Vector3 position(){
            float current_time = enet_time_get();
            float lerp_amount = (current_time - last_time())/(latest_time() - last_time());
            return Vector3Lerp(last_position(),latest_position(),lerp_amount);
        }

        raylib::Vector3 rotation(){
            return m_position_buffer[latest_buffer()];
        }
};

class Client : public Component {
  private:
    std::mutex m_client_mutex;
    std::mutex m_players_mutex;
    std::string m_host;
    enet_uint16 m_port;
    ENetHost* m_client;
    ENetAddress m_address;
    ENetPeer* m_peer;
    int m_tickrate = 128;
    float m_time_per_update;
    float m_last_time;
    enet_uint32 m_last_tick = 0;
    bool m_should_quit = false;
    std::thread m_client_thread;

    bool m_forward = false;
    bool m_backward = false;
    bool m_left = false;
    bool m_right = false;

    bool m_connected = false;

    enet_uint32 m_pings[N_PING_AVERAGE] = {500, 500, 500, 500, 500};
    int m_current_ping = 0;
    std::unordered_map<enet_uint32,PlayerNetworkedData> m_player_data;

    enet_uint32 m_id = -1;

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

    // returns 0 if connection failed, 1 if succeeded
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
        std::lock_guard<std::mutex> guard(m_client_mutex);
        return m_should_quit;
    }

    void quit() {
        std::lock_guard<std::mutex> guard(m_client_mutex);
        m_should_quit = true;
    }

    void send_input_packet() {
        std::lock_guard<std::mutex> guard(m_client_mutex);
        ClientPacket send_packet(m_forward, m_backward, m_left, m_right, this->entity()->get_child(0)->get_component<Transform>()->rotation);
        RawClientPacket raw_packet = send_packet.get_raw();
        ENetPacket* packet = enet_packet_create(
            &raw_packet, sizeof(RawClientPacket), ENET_PACKET_FLAG_RELIABLE);
        if (enet_peer_send(m_peer, 0, packet) != 0)
            TraceLog(LOG_ERROR, "Packet send failed?");
        enet_host_flush(m_client);
        reset_inputs();
    }

    void handle_recieve(ENetEvent* event) {
        PlayerStatePacket player_states(event->packet->data);
        auto header = player_states.header();
        auto current_time = enet_time_get();
        auto this_ping = current_time - header.ping_return;
        add_ping_measurement(this_ping);

        if (header.tick <= m_last_tick){
            return;
        }
        auto send_time = current_time - (this_ping/2);

        //TraceLog(LOG_INFO,"tick = %u",header.tick);
        std::lock_guard<std::mutex> guard(m_players_mutex);
        auto states = player_states.states();
        for (auto& i : states) {
            m_player_data[i.id].update_buffer(send_time,i.pos(),i.rot());
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
        if (!connect()) {
            TraceLog(LOG_ERROR, "Connection failed...");
            return;
        }
        enet_time_set(0);
        m_last_time = enet_time_get();
        m_time_per_update = 1000.0f / (float)m_tickrate;
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
        }
    }

    void update() {
        if (!m_connected)
            return;
        update_inputs();
        std::lock_guard<std::mutex> guard(m_players_mutex);
        this->entity()->get_component<Transform>()->position = m_player_data[m_id].position();
        // game_info.draw_debug()
        //  ClientPacket
        //  packet(IsKeyPressed(KEY_W),IsKeyPressed(KEY_S),IsKeyPressed(KEY_A),IsKeyPressed(KEY_D));
    }

    void draw2D() {
        if (!m_connected)return;
        game_info.draw_debug_var("ping", (int)ping(), 100, 100, GREEN);
    }

    void draw3D(raylib::Matrix transform){
        if (!m_connected)return;
        //std::lock_guard<std::mutex> guard(m_players_mutex);
        //this->entity()->get_component<Transform>()->position = m_player_data[m_id].position();
        //this->entity()->get_component<Transform>()->rotation = m_player_data[m_id].rotation();
    }

    void draw_debug(){
        if (!m_connected)return;
        std::lock_guard<std::mutex> guard(m_players_mutex);
        for (auto& i : m_player_data){
            if (i.first == m_id)continue;
            auto pos = i.second.position();
            DrawCapsuleWires(pos - raylib::Vector3(0,PLAYER_HEIGHT/2.0f,0),pos + raylib::Vector3(0,PLAYER_HEIGHT/2.0f,0),PLAYER_RADIUS,10,10,GREEN);
            DrawSphereWires(pos - raylib::Vector3(0,PLAYER_FOOT_OFFSET,0),PLAYER_FOOT_RADIUS,10,10,GREEN);
        }
        DrawGrid(100,1);
    }

    void destroy() {}
};

} // namespace SPRF

#endif