#include "GameServer.h"
#include "GameWorld.h"
#include "./enet/enet.h"
using namespace NCL;
using namespace CSC8503;

static std::map<int, ENetPeer*> connectedClients;

GameServer::GameServer(int onPort, int maxClients)	{
	port		= onPort;
	clientMax	= maxClients;
	clientCount = 0;
	netHandle	= nullptr;
	Initialise();
}

GameServer::~GameServer()	{
	connectedClients.clear();
	Shutdown();
}

void GameServer::Shutdown() {
	SendGlobalPacket(BasicNetworkMessages::Shutdown);
	enet_host_destroy(netHandle);
	netHandle = nullptr;
}

bool GameServer::Initialise() {
	ENetAddress address;
	address.host = ENET_HOST_ANY;
	address.port = port;
	
	std::cout << "Attempting to create server on port " << port << "...\n";

	netHandle = enet_host_create(&address, clientMax, 1, 0, 0);
	
	if (!netHandle) {
		std::cout << "Server creation failed. This could be because:\n";
		std::cout << "- Port " << port << " is already in use\n";
		std::cout << "- Application lacks required permissions\n";
		std::cout << "- Network system initialization failed\n";
		std::cout << __FUNCTION__ << "failed to create network handle!" << std::endl;
		return false;
	}
	
	std::cout << "Server created successfully!\n";
	return true;
}

bool GameServer::SendGlobalPacket(int msgID) {
	GamePacket packet;
	packet.type = msgID;
	return SendGlobalPacket(packet);
}

bool GameServer::SendGlobalPacket(GamePacket& packet) {
	ENetPacket* dataPacket = enet_packet_create(&packet, packet.GetTotalSize(), 0);
	enet_host_broadcast(netHandle, 0, dataPacket);
	return true;
}

void GameServer::UpdateServer() {
	if (!netHandle) { return; }
	ENetEvent event;
	while (enet_host_service(netHandle, &event, 0) > 0) {
		int type = event.type;
		ENetPeer* p = event.peer;
		int peer = p->incomingPeerID;
		
		if (type == ENetEventType::ENET_EVENT_TYPE_CONNECT) {
			std::cout << "Server: New client connected" << std::endl;

			int newPlayerID = nextPlayerID++;
			connectedClients[newPlayerID] = p;

			// Send ID to client
			PlayerIDPacket idPacket(newPlayerID);
			ENetPacket* dataPacket = enet_packet_create(&idPacket, idPacket.GetTotalSize(), ENET_PACKET_FLAG_RELIABLE);
			enet_peer_send(p, 0, dataPacket);

			std::cout << "Server: Assigned ID " << newPlayerID << " to new client\n";
		}
		else if (type == ENetEventType::ENET_EVENT_TYPE_DISCONNECT) {
			std::cout << "Server: A client has disconnected" << std::endl;

			for (auto it = connectedClients.begin(); it != connectedClients.end(); ++it) {
				if (it->second == p) {
					std::cout << "Server: Player " << it->first << " disconnected\n";
					connectedClients.erase(it);
					break;
				}
			}
		}
		else if (type == ENetEventType::ENET_EVENT_TYPE_RECEIVE) {
			GamePacket* packet = (GamePacket*)event.packet->data;
			ProcessPacket(packet, peer);
		}

		if (event.packet) {
			enet_packet_destroy(event.packet);
		}
	}
}

void GameServer::SetGameWorld(GameWorld &g) {
	gameWorld = &g;
}