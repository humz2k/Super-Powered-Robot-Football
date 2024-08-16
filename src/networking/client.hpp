/** @file client.hpp
 *
 * PlayerNetworkedData for lerping recieved ticks. Client for client side
 * networking.
 *
 */

#ifndef _SPRF_NETWORKING_CLIENT_HPP_
#define _SPRF_NETWORKING_CLIENT_HPP_

#include <functional>
#include <list>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>
#include "engine/engine.hpp"
#include "packet.hpp"
#include "physics/player_stats.hpp"
#include <enet/enet.h>

#define N_PING_AVERAGE (5)
#define N_RECV_AVERAGE (20)

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

class SmoothedVariable {
  private:
    //float* m_data = NULL;
    std::vector<float> m_data;
    int m_pointer = 0;
    const int m_samples;

  public:
    SmoothedVariable(int samples, float initial_value = 0.0f)
        : m_samples(samples) {
        m_data.resize(samples);
        //assert((m_data = (float*)malloc(sizeof(float) * samples)));
        for (int i = 0; i < m_samples; i++) {
            m_data[i] = initial_value;
        }
    }

    void update(float sample) {
        m_data[m_pointer] = sample;
        m_pointer = (m_pointer + 1) % m_samples;
    }

    float get() {
        float sum = 0;
        for (int i = 0; i < m_samples; i++) {
            sum += m_data[i];
        }
        return sum / (float)m_samples;
    }

    ~SmoothedVariable() { //free(m_data);
    }
};

template <class T> class UpdateVariable : public DevConsoleCommand {
  private:
    std::string m_var_name;
    T* m_var;
    T convert_variable(std::string& val) {
        std::stringstream convert(val);
        T value;
        convert >> value;
        return value;
    }

  public:
    UpdateVariable(DevConsole& console, std::string var_name, T* var)
        : DevConsoleCommand(console), m_var_name(var_name), m_var(var){};
    void handle(std::vector<std::string>& args) {
        if (args.size() == 0) {
            std::string out = m_var_name + " = " + std::to_string(*m_var);
            TraceLog(LOG_CONSOLE, out.c_str());
            return;
        }
        T val = convert_variable(args[0]);
        *m_var = val;
        std::string out = m_var_name + " = " + std::to_string(*m_var);
        TraceLog(LOG_CONSOLE, out.c_str());
    }
};

template <> class UpdateVariable<bool> : public DevConsoleCommand {
  private:
    std::string m_var_name;
    bool* m_var;

  public:
    UpdateVariable(DevConsole& console, std::string var_name, bool* var)
        : DevConsoleCommand(console), m_var_name(var_name), m_var(var){};
    void handle(std::vector<std::string>& args) {
        if (args.size() == 0) {
            std::string out =
                m_var_name + " = " + ((*m_var) ? "true" : "false");
            TraceLog(LOG_CONSOLE, out.c_str());
            return;
        }
        *m_var = args[0] == "true";
        std::string out = m_var_name + " = " + ((*m_var) ? "true" : "false");
        TraceLog(LOG_CONSOLE, out.c_str());
    }
};

/** NOTE: No fake ping for sends!!! Only recieves... Please fix! */
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

    float m_last_recieve = 0;

    SmoothedVariable m_recv_delta;
    SmoothedVariable m_send_delta;
    SmoothedVariable m_ping;

    float m_interp = 2;
    game_state_packet m_last_game_state;
    std::mutex m_queue_mutex;
    std::list<game_state_packet> m_game_state_queue;
    std::unordered_map<enet_uint32, Entity*> m_entities;

    enet_uint32 m_id = -1;

    std::function<void(Entity*)> m_init_player;

    float m_fake_packet_down_loss_amount = 0.03;
    bool m_fake_packet_down_loss = false;
    float m_fake_packet_up_loss_amount = 0.01;
    bool m_fake_packet_up_loss = false;
    int m_fake_ping_amount = 5;
    bool m_fake_ping = false;
    std::queue<ENetEvent> m_fake_ping_down_packets;

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
        if (m_fake_packet_up_loss) {
            if (randrange(0, 1) <= m_fake_packet_up_loss_amount) {
                reset_inputs();
                return;
            }
        }
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

    void handle_recieve(ENetEvent* event) {
        if (m_fake_packet_down_loss) {
            if (randrange(0, 1) <= m_fake_packet_down_loss_amount)
                return;
        }

        packet_header header = *(packet_header*)(event->packet->data);
        if (header.packet_type == PACKET_PING_RESPONSE) {
            ping_response_packet tmp(event->packet->data,
                                     event->packet->dataLength);
            m_ping.update(enet_time_get() - tmp.ping_return);
            return;
        }
        if (header.packet_type == PACKET_GAME_STATE) {
            m_recv_delta.update(enet_time_get() - m_last_recieve);
            game_info.recieve_delta = m_recv_delta.get();
            m_last_recieve = enet_time_get();
            game_state_packet game_state_update(event->packet->data,
                                                event->packet->dataLength);
            game_state_update.timestamp = enet_time_get();
            m_last_game_state = game_state_update;

            std::lock_guard<std::mutex> guard(m_queue_mutex);
            m_game_state_queue.push_back(game_state_update);
        }
    }

    void recv_packet() {
        if (m_fake_ping) {
            if (m_fake_ping_down_packets.size() > m_fake_ping_amount) {
                ENetEvent event = m_fake_ping_down_packets.front();
                m_fake_ping_down_packets.pop();
                handle_recieve(&event);
                enet_packet_destroy(event.packet);
            }
            ENetEvent event;
            if (enet_host_service(m_client, &event, 2) > 0) {
                switch (event.type) {
                case ENET_EVENT_TYPE_RECEIVE:
                    m_fake_ping_down_packets.push(event);
                    break;
                default:
                    TraceLog(LOG_ERROR, "Unknown Event Recieved");
                    break;
                }
            }
            return;
        }

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
        m_last_recieve = enet_time_get();
        while (!should_quit()) {
            float time = enet_time_get();
            if ((time - last_time) >= (1000.0f / (float)m_tickrate)) {
                // send_delta += time - last_time;
                m_send_delta.update(time - last_time);
                game_info.send_delta = m_send_delta.get();
                last_time = time;
                send_input_packet();
                // sends++;
            }

            recv_packet();
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
    Client(std::string host, enet_uint16 port,
           std::function<void(Entity*)> init_player_, DevConsole* dev_console)
        : m_host(host), m_port(port), m_recv_delta(N_RECV_AVERAGE, 100),
          m_send_delta(N_RECV_AVERAGE, 100), m_ping(N_PING_AVERAGE, 500),
          m_init_player(init_player_) {
        if (!connect()) {
            TraceLog(LOG_ERROR, "Connection failed...");
            TraceLog(LOG_INFO, "destroying enet client");
            enet_host_destroy(m_client);
            return;
        }
        // this->entity()->scene()->
        dev_console->add_command<UpdateVariable<float>>("cl_interp",
                                                        "cl_interp", &m_interp);
        dev_console->add_command<UpdateVariable<int>>(
            "cl_fake_ping_amount", "cl_fake_ping_amount", &m_fake_ping_amount);
        dev_console->add_command<UpdateVariable<float>>(
            "cl_fake_packet_up_loss_amount", "cl_fake_packet_up_loss_amount",
            &m_fake_packet_up_loss_amount);
        dev_console->add_command<UpdateVariable<float>>(
            "cl_fake_packet_down_loss_amount",
            "cl_fake_packet_down_loss_amount", &m_fake_packet_down_loss_amount);
        dev_console->add_command<UpdateVariable<bool>>(
            "cl_fake_ping", "cl_fake_ping", &m_fake_ping);
        dev_console->add_command<UpdateVariable<bool>>("cl_fake_packet_up_loss",
                                                       "cl_fake_packet_up_loss",
                                                       &m_fake_packet_up_loss);
        dev_console->add_command<UpdateVariable<bool>>(
            "cl_fake_packet_down_loss", "cl_fake_packet_down_loss",
            &m_fake_packet_down_loss);
        enet_time_set(0);
        m_client_thread = std::thread(&Client::run_client, this);
    }

    void close() {
        if (!m_connected)
            return;
        quit();
        m_client_thread.join();
        disconnect();
        m_connected = false;
    }

    ~Client() {
        if (!m_connected)
            return;
        quit();
        m_client_thread.join();
        disconnect();
        m_connected = false;
    }

    void init() {
        if (!m_connected) {
            this->entity()->scene()->close();
            return;
        }
    }

    // bool id_exists(std::unordered_map<enet_uint32, player_state_data>& data,
    // enet_uint32 id){
    //     return data.find(id) != data.end();
    // }

    player_state_data interpolate_player_states(player_state_data& previous,
                                                player_state_data& next,
                                                enet_uint32 previous_time,
                                                enet_uint32 next_time,
                                                enet_uint32 client_time) {
        auto p1 = previous.position();
        auto p2 = next.position();
        float lerp_amout = (((float)client_time) - (float)previous_time) /
                           (((float)next_time) - ((float)previous_time));
        player_state_data tmp;
        tmp.position(Vector3Lerp(p1, p2, lerp_amout));
        tmp.velocity(next.velocity());
        tmp.rotation(next.rotation());
        tmp.id = next.id;
        return tmp;
    }

    game_state_packet interpolate_game_states() {
        enet_uint32 client_time =
            enet_time_get() - m_recv_delta.get() * m_interp;
        std::lock_guard<std::mutex> guard(m_queue_mutex);
        game_info.packet_queue_size = m_game_state_queue.size();
        if (m_game_state_queue.size() == 0) {
            TraceLog(LOG_WARNING, "queue is empty?????");
            return game_state_packet();
        } else if (m_game_state_queue.size() == 1) {
            TraceLog(LOG_WARNING, "queue has 1 element???");
            return m_game_state_queue.back();
        }
        if (m_game_state_queue.back().timestamp < client_time) {
            TraceLog(LOG_WARNING, "client_time too far ahead!!!");
            while (m_game_state_queue.size() > 1) {
                m_game_state_queue.pop_front();
            }
            return m_game_state_queue.back();
        }
        while (m_game_state_queue.size() > 1) {
            auto t1_ptr = m_game_state_queue.begin();
            auto t2_ptr = std::next(t1_ptr, 1);
            auto& t1 = *t1_ptr;
            auto& t2 = *t2_ptr;
            if ((t1.timestamp <= client_time) && (t2.timestamp > client_time)) {
                game_state_packet out;
                out.timestamp = client_time;
                if (t1.states.size() < 1) {
                    return t1;
                }
                std::unordered_map<enet_uint32, player_state_data>
                    previous_states, next_states;

                for (auto& i : t1.states) {
                    previous_states[i.id] = i;
                }
                for (auto& i : t2.states) {
                    next_states[i.id] = i;
                }
                for (auto& i : next_states) {
                    enet_uint32 id = i.first;
                    player_state_data& next_data = i.second;
                    if (!KEY_EXISTS(previous_states, id)) {
                        out.states.push_back(next_data);
                        continue;
                    }
                    out.states.push_back(interpolate_player_states(
                        previous_states[id], i.second, t1.timestamp,
                        t2.timestamp, client_time));
                }
                return out;
            } else if ((t1.timestamp > client_time) &&
                       (t2.timestamp > client_time)) {
                TraceLog(LOG_WARNING, "no current packets???");
                return t1;
            } else {
                m_game_state_queue.pop_front();
            }
        }
        TraceLog(LOG_WARNING, "something bad happened");
        return m_game_state_queue.back();
    }

    /** TODO: This only handles one player!!!! Please fix... */
    void update() {
        if (!m_connected)
            return;

        // update inputs
        if (!game_info.dev_console_active)
            update_inputs();
        for (auto& i : m_entities) {
            i.second->get_component<NetworkEntity>()->active = false;
        }
        for (auto& i : interpolate_game_states().states) {
            if (i.id == m_id) {
                this->entity()->get_component<Transform>()->position =
                    i.position();
                game_info.position = i.position();
                game_info.velocity = i.velocity();
                continue;
            }
            if (!KEY_EXISTS(m_entities, i.id)) {
                auto entity = this->entity()->scene()->create_entity();
                entity->add_component<NetworkEntity>();
                m_init_player(entity);
                entity->init();
                m_entities[i.id] = entity;
            }
            auto net_data = m_entities[i.id]->get_component<NetworkEntity>();
            net_data->position = i.position();
            net_data->rotation = i.rotation();
            net_data->velocity = i.velocity();
            net_data->active = true;
        }

        // store information in game info
        game_info.ping = m_ping.get();
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