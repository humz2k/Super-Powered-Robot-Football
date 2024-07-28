#ifndef _SPRF_NETWORKING_PACKET_
#define _SPRF_NETWORKING_PACKET_

#include "raylib-cpp.hpp"
#include <enet/enet.h>

#define CLIENT_PACKET 0
#define SERVER_PACKET 1

namespace SPRF {

struct PlayerState {
    enet_uint32 id;
    enet_uint32 tick;
    enet_uint32 timestamp;
    float position[3];
    float rotation[3];
    float health;

    PlayerState(enet_uint32 id_, enet_uint32 tick_, enet_uint32 timestamp_)
        : id(id_), tick(tick_), timestamp(timestamp_) {
        memset(position, 0, sizeof(position));
        memset(rotation, 0, sizeof(rotation));
        health = 100;
    }

    PlayerState() {}

    void print() {
        TraceLog(
            LOG_INFO,
            "Player %u (tick %u, time %f, delta %f): %g %g %g | %g %g %g | %g",
            id, tick, ((float)timestamp) / 1000.0f,
            ((float)timestamp - (float)enet_time_get()) / 1000.0f, position[0],
            position[1], position[2], rotation[0], rotation[1], rotation[2],
            health);
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
    enet_uint32 raw;
};

struct ClientPacket {

    bool forward;
    bool backward;
    bool left;
    bool right;

    ClientPacket(bool forward_, bool backward_, bool left_, bool right_)
        : forward(forward_), backward(backward_), left(left_), right(right_) {}

    ClientPacket(RawClientPacket raw) {
        forward = raw.raw & (1 << 0);
        backward = raw.raw & (1 << 1);
        left = raw.raw & (1 << 2);
        right = raw.raw & (1 << 3);
    }

    RawClientPacket get_raw() {
        RawClientPacket out;
        out.raw =
            0 | (forward << 0) | (backward << 1) | (left << 2) | (right << 3);
        return out;
    }

    void print() {
        TraceLog(LOG_INFO, "Packet: %s %s %s %s", forward ? "+forward" : "",
                 backward ? "+backward" : "", left ? "+left" : "",
                 right ? "+right" : "");
    }
};

} // namespace SPRF

#endif