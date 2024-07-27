#ifndef _SPRF_SERVER_SERVER_HPP_
#define _SPRF_SERVER_SERVER_HPP_

#include <enet/enet.h>
#include <string>
#include "raylib-cpp.hpp"

namespace SPRF {
namespace Networking {

class Server {
  private:
    std::string m_host;
    enet_uint16 m_port;
    size_t m_peer_count;
    size_t m_channel_count;
    enet_uint32 m_iband;
    enet_uint32 m_oband;

    ENetAddress m_address;
    ENetHost* m_enet_server;

  public:
    void get_event(){
        ENetEvent event;
        if (!(enet_host_service (m_enet_server, &event, 1000) > 0))
            return;
        switch(event.type){
            case ENET_EVENT_TYPE_CONNECT:
                TraceLog(LOG_INFO,"Peer Connected");
                break;
            case ENET_EVENT_TYPE_RECEIVE:
                TraceLog(LOG_INFO,"Recieved Data");
                break;
            case ENET_EVENT_TYPE_DISCONNECT:
                TraceLog(LOG_INFO,"Peer Disconnected");
                break;
            default:
                TraceLog(LOG_INFO,"Got Unknown Event");
                break;
        }
    }

    Server(std::string host, enet_uint16 port, size_t peer_count = 4,
           size_t channel_count = 2, enet_uint32 iband = 0,
           enet_uint32 oband = 0)
        : m_host(host), m_port(port), m_peer_count(peer_count),
          m_channel_count(channel_count), m_iband(iband), m_oband(oband) {
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

        m_enet_server = enet_host_create(&m_address, m_peer_count, m_channel_count,
                                         m_iband, m_oband);
        if (m_enet_server == NULL) {
            TraceLog (LOG_ERROR, "Create an ENet server host failed");
            exit (EXIT_FAILURE);
        }
        TraceLog(LOG_INFO, "ENet server host created");
    }

    ~Server() {
        enet_host_destroy(m_enet_server);
        TraceLog(LOG_INFO, "ENet server host destroyed");
        enet_deinitialize();
        TraceLog(LOG_INFO, "Enet deinitialized");
    }
};

}; // namespace Networking
}; // namespace SPRF

#endif