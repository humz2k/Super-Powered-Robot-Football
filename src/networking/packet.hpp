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

namespace SPRF {

/**
 * @brief Represents the state of a player in the game.
 *
 * This structure holds essential data about a player's state, such as position,
 * velocity, rotation, and health. It is used to synchronize player states
 * between the server and clients in a multiplayer game.
 */
struct PlayerStateData {
    /** @brief Unique identifier for the player */
    enet_uint32 id;
    /** @brief Player's position in the game world (x, y, z) */
    float position_data[3];
    /** @brief Player's velocity in the game world (x, y, z) */
    float velocity_data[3];
    /** @brief Player's rotation in the game world (pitch, yaw, roll) */
    float rotation_data[3];
    /** @brief Player's health */
    float health_data;

    /**
     * @brief Construct a new PlayerStateData object with a specified ID.
     *
     * Initializes position, velocity, and rotation to zero, and health to 100.
     *
     * @param id_ The unique identifier for the player.
     */
    PlayerStateData(enet_uint32 id_) : id(id_) {
        memset(position_data, 0, sizeof(position_data));
        memset(velocity_data, 0, sizeof(velocity_data));
        memset(rotation_data, 0, sizeof(rotation_data));
        health_data = 100;
    }

    /**
     * @brief Default constructor for PlayerStateData.
     *
     * Allows for creating an uninitialized PlayerStateData.
     */
    PlayerStateData() {}

    /**
     * @brief Print the player's state to the log.
     *
     * This is useful for debugging and logging purposes, to track the state of
     * players.
     */
    void print() {
        TraceLog(LOG_INFO, "Player %u: %g %g %g | %g %g %g | %g %g %g | %g", id,
                 position_data[0], position_data[1], position_data[2],
                 velocity_data[0], velocity_data[1], velocity_data[2],
                 rotation_data[0], rotation_data[1], rotation_data[2],
                 health_data);
    }

    /**
     * @brief Get the player's position as a Vector3.
     *
     * @return raylib::Vector3 The player's position.
     */
    raylib::Vector3 position() {
        return raylib::Vector3(position_data[0], position_data[1],
                               position_data[2]);
    }

    /**
     * @brief Get the player's rotation as a Vector3.
     *
     * @return raylib::Vector3 The player's rotation.
     */
    raylib::Vector3 rotation() {
        return raylib::Vector3(rotation_data[0], rotation_data[1],
                               rotation_data[2]);
    }

    /**
     * @brief Get the player's velocity as a Vector3.
     *
     * @return raylib::Vector3 The player's velocity.
     */
    raylib::Vector3 velocity() {
        return raylib::Vector3(velocity_data[0], velocity_data[1],
                               velocity_data[2]);
    }
};

/**
 * @brief Header for a packet containing multiple player states.
 *
 * This header is included in packets that transmit player states between the
 * server and clients. It contains metadata about the packet, such as the tick,
 * timestamp, and the number of player states included.
 */
struct PlayerStateHeader {
    /** @brief Simulation tick when the packet was created */
    enet_uint32 tick;
    /** @brief Timestamp of the packet creation */
    enet_uint32 timestamp;
    /** @brief Ping value to be returned to the client */
    enet_uint32 ping_return;
    /** @brief Number of player states included in the packet */
    enet_uint32 n_states;
};

/**
 * @brief Padded representation of a player state header.
 *
 * This structure ensures that the header size matches the size of a
 * PlayerStateData, facilitating memory alignment and network transmission
 * (makes it easier to construct a packet to send, but yes we are wasting
 * space).
 */
struct PlayerStateHeaderPadded {
    /** @brief The actual player state header */
    PlayerStateHeader raw;
    /** @brief Padding to match the size of PlayerStateData */
    char pad[(sizeof(PlayerStateData) - sizeof(PlayerStateHeader)) /
             sizeof(char)];
};

/**
 * @brief Packet structure for transmitting multiple player states.
 *
 * This packet is used to bundle multiple player states together and send them
 * over the network. It includes a header with metadata and the actual player
 * state data.
 */
struct PlayerStatePacket {
    /** @brief Indicates whether the memory for the packet was allocated by this
     * object (so we can free it) */
    bool allocated = false;
    /** @brief Raw pointer to the packet data */
    void* raw;
    /** @brief Size of the packet data */
    size_t size;

    /**
     * @brief Construct a new PlayerStatePacket object.
     *
     * Initializes the packet with a header and a list of player states.
     *
     * @param tick Simulation tick when the packet was created.
     * @param timestamp Timestamp of the packet creation.
     * @param ping_return Ping value to be returned to the client.
     * @param states List of player states to include in the packet.
     */
    PlayerStatePacket(enet_uint32 tick, enet_uint32 timestamp,
                      enet_uint32 ping_return,
                      std::vector<PlayerStateData>& states)
        : allocated(true) {
        raw = malloc(sizeof(PlayerStateData) * (states.size() + 1));
        size = sizeof(PlayerStateData) * (states.size() + 1);
        PlayerStateHeaderPadded* header = (PlayerStateHeaderPadded*)raw;
        header->raw.tick = tick;
        header->raw.timestamp = timestamp;
        header->raw.ping_return = ping_return;
        header->raw.n_states = states.size();
        PlayerStateData* raw_states = &(((PlayerStateData*)raw)[1]);
        for (int i = 0; i < states.size(); i++) {
            raw_states[i] = states[i];
        }
    }

    /**
     * @brief Construct a new PlayerStatePacket object from raw data.
     *
     * This infers the size of the data from the header (maybe not super
     * safe...)
     *
     * @param raw_ Raw pointer to the packet data.
     */
    PlayerStatePacket(void* raw_) : allocated(false), raw(raw_) {
        size = sizeof(PlayerStateData) * (header().n_states + 1);
    }

    /**
     * @brief Get the header of the packet.
     *
     * @return PlayerStateHeader The header of the packet.
     */
    PlayerStateHeader header() { return ((PlayerStateHeaderPadded*)raw)->raw; }

    /**
     * @brief Get the list of player states included in the packet.
     *
     * Eh, probably not the best way to do this... Whatever.
     *
     * @return std::vector<PlayerState> The list of player states.
     */
    std::vector<PlayerStateData> states() {
        PlayerStateData* raw_states = &(((PlayerStateData*)raw)[1]);
        std::vector<PlayerStateData> out;
        out.resize(header().n_states);
        for (int i = 0; i < header().n_states; i++) {
            out[i] = raw_states[i];
        }
        return out;
    }

    /**
     * @brief Destructor for PlayerStatePacket.
     *
     * Frees the allocated memory if the packet was dynamically allocated by
     * this object.
     */
    ~PlayerStatePacket() {
        if (allocated)
            free(raw);
    }
};

/**
 * @brief Packet structure for the initial handshake between client and server.
 *
 * This packet is used to establish the initial connection and synchronization
 * between the client and server.
 */
struct HandshakePacket {
    /** @brief Unique identifier for the client */
    enet_uint32 id;
    /** @brief Tick rate of the server */
    enet_uint32 tickrate;
    /** @brief Current server time */
    enet_uint32 current_time;

    /**
     * @brief Construct a new HandshakePacket object.
     *
     * @param id_ Unique identifier for the client.
     * @param tickrate_ Tick rate of the server.
     * @param current_time_ Current server time.
     */
    HandshakePacket(enet_uint32 id_, enet_uint32 tickrate_,
                    enet_uint32 current_time_)
        : id(id_), tickrate(tickrate_), current_time(current_time_) {}

    /**
     * @brief Default constructor for HandshakePacket.
     */
    HandshakePacket() {}
};

/**
 * @brief Raw structure for a client packet.
 *
 * This structure is used for low-level network transmission and reception of
 * client data.
 */
struct ClientPacketRaw {
    /** @brief Ping value sent by the client (this should get returned to the
     * client for ping calculations) */
    enet_uint32 ping;
    /** @brief Encoded input state of the client (bits encode keyboard inputs)
     */
    enet_uint32 raw;
    /** @brief Rotation of the client (pitch, yaw, roll) */
    float rotation[3];
};

/**
 * @brief Packet structure for transmitting client inputs and state.
 *
 * This packet is used to send client inputs and state to the server, including
 * movement directions and rotation.
 */
struct ClientPacket {
    /** @brief Ping value sent by the client (to be returned by the server for
     * ping calculations) */
    enet_uint32 ping_send;
    /** @brief Rotation of the client (pitch, yaw, roll) */
    raylib::Vector3 rotation;
    /** @brief Indicates if the client is moving forward (i.e. pressed W this
     * tick) */
    bool forward;
    /** @brief Indicates if the client is moving backward (i.e. pressed S this
     * tick) */
    bool backward;
    /** @brief Indicates if the client is moving left (i.e. pressed A this tick)
     */
    bool left;
    /** @brief Indicates if the client is moving right (i.e. pressed D this
     * tick) */
    bool right;
    /** @brief Indicates if the client is jumping (i.e. pressed Space this tick)
     */
    bool jump;

    /**
     * @brief Construct a new ClientPacket object.
     *
     * Initializes the packet with the client's input state and rotation.
     *
     * @param forward_ Indicates if the client is moving forward (i.e. pressed W
     * this tick).
     * @param backward_ Indicates if the client is moving backward (i.e. pressed
     * S this tick).
     * @param left_ Indicates if the client is moving left (i.e. pressed A this
     * tick).
     * @param right_ Indicates if the client is moving right (i.e. pressed D
     * this tick).
     * @param jump_ Indicates if the client is jumping (i.e. pressed Space this
     * tick).
     * @param rotation_ Rotation of the client (pitch, yaw, roll).
     */
    ClientPacket(bool forward_, bool backward_, bool left_, bool right_,
                 bool jump_, raylib::Vector3 rotation_)
        : ping_send(enet_time_get()), rotation(rotation_), forward(forward_),
          backward(backward_), left(left_), right(right_), jump(jump_) {}

    /**
     * @brief Construct a new ClientPacket object from raw data.
     *
     * @param raw Raw data received from the client.
     */
    ClientPacket(ClientPacketRaw raw) {
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

    /**
     * @brief Get the raw data representation of the client packet.
     *
     * This is useful for network transmission, as it converts the high-level
     * data into a low-level format.
     *
     * @return ClientPacketRaw The raw data representation of the client packet.
     */
    ClientPacketRaw get_raw() {
        ClientPacketRaw out;
        out.ping = ping_send;
        out.raw = 0 | (forward << 0) | (backward << 1) | (left << 2) |
                  (right << 3) | (jump << 4);
        out.rotation[0] = rotation.x;
        out.rotation[1] = rotation.y;
        out.rotation[2] = rotation.z;
        return out;
    }

    /**
     * @brief Print the client's input state to the log.
     *
     * This is useful for debugging and logging purposes, to track the input
     * state of clients.
     */
    void print() {
        TraceLog(LOG_INFO, "Packet: %u | %s %s %s %s | %g %g %g", ping_send,
                 forward ? "+forward" : "", backward ? "+backward" : "",
                 left ? "+left" : "", right ? "+right" : "",
                 jump ? "+jump" : "", rotation.x, rotation.y, rotation.z);
    }
};

} // namespace SPRF

#endif // _SPRF_NETWORKING_PACKET_