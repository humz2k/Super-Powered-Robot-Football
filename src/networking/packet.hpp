/** @file packet.hpp
 *
 * This header file defines the structures and classes needed for handling
 * network packets in a multiplayer game. The code ensures efficient
 * synchronization of player states between the server and clients, handles the
 * initial handshake during connection establishment, and provides mechanisms
 * for transmitting and receiving client inputs.
 *
 */

#ifndef _SPRF_NETWORKING_PACKET_
#define _SPRF_NETWORKING_PACKET_

#include "raylib-cpp.hpp"
#include <enet/enet.h>
#include <vector>

namespace SPRF {

enum packet_type_t {
    PACKET_PING = 0,
    PACKET_PING_RESPONSE,
    PACKET_USER_ACTION,
    PACKET_USER_COMMAND,
    PACKET_GAME_STATE,
    PACKET_GAME_EVENT,
    PACKET_SERVER_HANDSHAKE
};

struct packet_header {
    packet_type_t packet_type;
    packet_header(packet_type_t packet_type_) : packet_type(packet_type_) {}
    packet_header() {}
};

ENetPacket*
construct_packet(packet_type_t type, void* data, size_t size,
                 enet_uint32 flags = (ENET_PACKET_FLAG_UNSEQUENCED)) {
    packet_header header(type);
    size_t total_size = sizeof(packet_header) + size;
    void* raw = malloc(total_size);
    memcpy(raw, &header, sizeof(packet_header));
    memcpy(((char*)raw) + sizeof(packet_header), data, size);
    ENetPacket* out = enet_packet_create(raw, total_size, flags);
    free(raw);
    return out;
}

struct ping_packet {
    enet_uint32 ping;
    ping_packet(enet_uint32 ping_) : ping(ping_) {}
    ping_packet() {}

    ENetPacket* serialize() {
        return construct_packet(PACKET_PING, this, sizeof(*this));
    }
};

struct ping_response_packet {
    enet_uint32 ping_return;
    ping_response_packet(enet_uint32 ping_return_)
        : ping_return(ping_return_) {}
    ping_response_packet() {}
    ping_response_packet(void* data, size_t datalen) {
        assert(datalen == sizeof(packet_header) + sizeof(*this));
        memcpy(this, (((char*)data) + sizeof(packet_header)), sizeof(*this));
    }

    ENetPacket* serialize() {
        return construct_packet(PACKET_PING_RESPONSE, this, sizeof(*this));
    }
};

struct player_state_data {
    enet_uint32 id;
    float position_data[3];
    float velocity_data[3];
    float rotation_data[3];
    float health_data;

    player_state_data(enet_uint32 id_) : id(id_) {
        memset(position_data, 0, sizeof(position_data));
        memset(velocity_data, 0, sizeof(velocity_data));
        memset(rotation_data, 0, sizeof(rotation_data));
        health_data = 100;
    }

    player_state_data() {}

    void print() {
        TraceLog(LOG_INFO, "Player %u: %g %g %g | %g %g %g | %g %g %g | %g", id,
                 position_data[0], position_data[1], position_data[2],
                 velocity_data[0], velocity_data[1], velocity_data[2],
                 rotation_data[0], rotation_data[1], rotation_data[2],
                 health_data);
    }

    raylib::Vector3 position() {
        return raylib::Vector3(position_data[0], position_data[1],
                               position_data[2]);
    }

    raylib::Vector3 position(raylib::Vector3 pos) {
        position_data[0] = pos.x;
        position_data[1] = pos.y;
        position_data[2] = pos.z;
        return raylib::Vector3(position_data[0], position_data[1],
                               position_data[2]);
    }

    raylib::Vector3 rotation() {
        return raylib::Vector3(rotation_data[0], rotation_data[1],
                               rotation_data[2]);
    }

    raylib::Vector3 rotation(raylib::Vector3 rot) {
        rotation_data[0] = rot.x;
        rotation_data[1] = rot.y;
        rotation_data[2] = rot.z;
        return raylib::Vector3(rotation_data[0], rotation_data[1],
                               rotation_data[2]);
    }

    raylib::Vector3 velocity() {
        return raylib::Vector3(velocity_data[0], velocity_data[1],
                               velocity_data[2]);
    }

    raylib::Vector3 velocity(raylib::Vector3 vel) {
        velocity_data[0] = vel.x;
        velocity_data[1] = vel.y;
        velocity_data[2] = vel.z;
        return raylib::Vector3(velocity_data[0], velocity_data[1],
                               velocity_data[2]);
    }
};

struct game_state_packet {
    enet_uint32 timestamp;
    std::vector<player_state_data> states;

    game_state_packet(enet_uint32 timestamp_,
                      std::vector<player_state_data> states_)
        : timestamp(timestamp_), states(states_) {}
    game_state_packet() {}

    game_state_packet(void* raw, size_t datalen) {
        memcpy(&timestamp, ((char*)raw) + sizeof(packet_header),
               sizeof(enet_uint32));
        size_t state_size =
            datalen - sizeof(packet_header) - sizeof(enet_uint32);
        int n_states = state_size / sizeof(player_state_data);
        states.resize(n_states);
        memcpy(states.data(),
               ((char*)raw) + sizeof(packet_header) + sizeof(enet_uint32),
               state_size);
    }

    ENetPacket* serialize() {
        size_t size =
            sizeof(enet_uint32) + sizeof(player_state_data) * states.size();
        void* data = malloc(size);
        memcpy(data, &timestamp, sizeof(enet_uint32));
        memcpy(((char*)data) + sizeof(enet_uint32), states.data(),
               states.size() * sizeof(player_state_data));
        ENetPacket* out = construct_packet(PACKET_GAME_STATE, data, size);
        free(data);
        return out;
    }
};

struct HandshakePacket {
    enet_uint32 id;
    enet_uint32 tickrate;
    enet_uint32 current_time;

    HandshakePacket(enet_uint32 id_, enet_uint32 tickrate_,
                    enet_uint32 current_time_)
        : id(id_), tickrate(tickrate_), current_time(current_time_) {}

    HandshakePacket() {}
};

struct user_action_packet_serialized {
    enet_uint32 ping;
    enet_uint32 raw;
    float rotation[3];
};

struct user_action_packet {
    enet_uint32 ping_send;
    raylib::Vector3 rotation;
    bool forward;
    bool backward;
    bool left;
    bool right;
    bool jump;

    user_action_packet(bool forward_, bool backward_, bool left_, bool right_,
                       bool jump_, raylib::Vector3 rotation_)
        : ping_send(enet_time_get()), rotation(rotation_), forward(forward_),
          backward(backward_), left(left_), right(right_), jump(jump_) {}

    user_action_packet(void* rawptr, size_t datalen) {
        assert(datalen = sizeof(packet_header) +
                         sizeof(user_action_packet_serialized));
        user_action_packet_serialized raw;
        memcpy(&raw, ((char*)rawptr) + sizeof(packet_header),
               sizeof(user_action_packet_serialized));
        ping_send = raw.ping;
        forward = raw.raw & (1 << 0);
        backward = raw.raw & (1 << 1);
        left = raw.raw & (1 << 2);
        right = raw.raw & (1 << 3);
        jump = raw.raw & (1 << 4);
        rotation.x = raw.rotation[0];
        rotation.y = raw.rotation[1];
        rotation.z = raw.rotation[2];
    }

    ENetPacket* serialize() {
        user_action_packet_serialized out;
        out.ping = ping_send;
        out.raw = 0 | (forward << 0) | (backward << 1) | (left << 2) |
                  (right << 3) | (jump << 4);
        out.rotation[0] = rotation.x;
        out.rotation[1] = rotation.y;
        out.rotation[2] = rotation.z;
        return construct_packet(PACKET_USER_ACTION, &out,
                                sizeof(user_action_packet_serialized));
    }

    void print() {
        TraceLog(LOG_INFO, "Packet: %u | %s %s %s %s | %g %g %g", ping_send,
                 forward ? "+forward" : "", backward ? "+backward" : "",
                 left ? "+left" : "", right ? "+right" : "",
                 jump ? "+jump" : "", rotation.x, rotation.y, rotation.z);
    }
};

} // namespace SPRF

#endif // _SPRF_NETWORKING_PACKET_