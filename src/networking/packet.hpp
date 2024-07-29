#ifndef _SPRF_NETWORKING_PACKET_
#define _SPRF_NETWORKING_PACKET_

#include "raylib-cpp.hpp"
#include <enet/enet.h>

#define CLIENT_PACKET 0
#define SERVER_PACKET 1

namespace SPRF {

struct PlayerState {
    enet_uint32 id;
    float position[3];
    float velocity[3];
    float rotation[3];
    float health;

    PlayerState(enet_uint32 id_) : id(id_) {
        memset(position, 0, sizeof(position));
        memset(velocity, 0, sizeof(velocity));
        memset(rotation, 0, sizeof(rotation));
        health = 100;
    }

    PlayerState() {}

    void print() {
        TraceLog(LOG_INFO, "Player %u: %g %g %g | %g %g %g | %g %g %g | %g", id,
                 position[0], position[1], position[2], velocity[0], velocity[1], velocity[2], rotation[0],
                 rotation[1], rotation[2], health);
    }

    raylib::Vector3 pos(){
        return raylib::Vector3(position[0],position[1],position[2]);
    }

    raylib::Vector3 rot(){
        return raylib::Vector3(rotation[0],rotation[1],rotation[2]);
    }

    raylib::Vector3 vel(){
        return raylib::Vector3(velocity[0],velocity[1],velocity[2]);
    }
};

struct PlayerStateHeader {
    enet_uint32 tick;
    enet_uint32 timestamp;
    enet_uint32 ping_return;
    enet_uint32 n_states;
};

struct PlayerStateHeaderRaw {
    PlayerStateHeader raw;
    char pad[(sizeof(PlayerState) - sizeof(PlayerStateHeader)) / sizeof(char)];
};

struct PlayerStatePacket {
    bool allocated = false;
    void* raw;
    size_t size;

    PlayerStatePacket(enet_uint32 tick, enet_uint32 timestamp,
                      enet_uint32 ping_return, std::vector<PlayerState>& states)
        : allocated(true) {
        raw = malloc(sizeof(PlayerState) * (states.size() + 1));
        size = sizeof(PlayerState) * (states.size() + 1);
        PlayerStateHeaderRaw* header = (PlayerStateHeaderRaw*)raw;
        header->raw.tick = tick;
        header->raw.timestamp = timestamp;
        header->raw.ping_return = ping_return;
        header->raw.n_states = states.size();
        PlayerState* raw_states = &(((PlayerState*)raw)[1]);
        for (int i = 0; i < states.size(); i++) {
            raw_states[i] = states[i];
        }
    }

    PlayerStatePacket(void* raw_) : allocated(false), raw(raw_) {
        size = sizeof(PlayerState) * (header().n_states + 1);
    }

    PlayerStateHeader header() { return ((PlayerStateHeaderRaw*)raw)->raw; }

    std::vector<PlayerState> states() {
        PlayerState* raw_states = &(((PlayerState*)raw)[1]);
        std::vector<PlayerState> out;
        out.resize(header().n_states);
        for (int i = 0; i < header().n_states; i++) {
            out[i] = raw_states[i];
        }
        return out;
    }

    ~PlayerStatePacket() {
        if (allocated)
            free(raw);
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

struct RawClientPacket {
    enet_uint32 ping;
    enet_uint32 raw;
    float rotation[3];
};

struct ClientPacket {

    enet_uint32 ping_send;
    raylib::Vector3 rotation;
    bool forward;
    bool backward;
    bool left;
    bool right;

    ClientPacket(bool forward_, bool backward_, bool left_, bool right_, raylib::Vector3 rotation_)
        : ping_send(enet_time_get()), rotation(rotation_), forward(forward_), backward(backward_),
          left(left_), right(right_) {}

    ClientPacket(RawClientPacket raw) {
        ping_send = raw.ping;
        forward = raw.raw & (1 << 0);
        backward = raw.raw & (1 << 1);
        left = raw.raw & (1 << 2);
        right = raw.raw & (1 << 3);
        rotation.x = raw.rotation[0];
        rotation.y = raw.rotation[1];
        rotation.z = raw.rotation[2];
    }

    RawClientPacket get_raw() {
        RawClientPacket out;
        out.ping = ping_send;
        out.raw =
            0 | (forward << 0) | (backward << 1) | (left << 2) | (right << 3);
        out.rotation[0] = rotation.x;
        out.rotation[1] = rotation.y;
        out.rotation[2] = rotation.z;
        return out;
    }

    void print() {
        TraceLog(LOG_INFO, "Packet: %u | %s %s %s %s | %g %g %g", ping_send,
                 forward ? "+forward" : "", backward ? "+backward" : "",
                 left ? "+left" : "", right ? "+right" : "",rotation.x,rotation.y,rotation.z);
    }
};

} // namespace SPRF

#endif