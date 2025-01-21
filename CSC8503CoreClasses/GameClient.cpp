#include "GameClient.h"
#include "./enet/enet.h"
using namespace NCL;
using namespace CSC8503;

GameClient::GameClient()	{
	netHandle = enet_host_create(nullptr, 1, 1, 0, 0);
}

GameClient::~GameClient()	{
	enet_host_destroy(netHandle);
}

bool GameClient::Connect(uint8_t a, uint8_t b, uint8_t c, uint8_t d, int portNum) {
	if (!netHandle) {
		std::cout << "Client network handle not initialized!\n";
		return false;
	}

	ENetAddress address;
	address.port = portNum;
	address.host = (d << 24) | (c << 16) | (b << 8) | (a);

	std::cout << "Attempting to connect to " << (int)a << "." << (int)b << "." << (int)c << "." << (int)d << ":" << portNum << "\n";
	
	netPeer = enet_host_connect(netHandle, &address, 2, 0);

	if (!netPeer) {
		std::cout << "Failed to create network peer!\n";
		return false;
	}

	// Wait for connection success/failure
	ENetEvent event;
	std::cout << "Waiting for connection response...\n";

	// Wait up to 5 seconds for the connection to succeed
	if (enet_host_service(netHandle, &event, 5000) > 0 &&
		event.type == ENET_EVENT_TYPE_CONNECT) {
		std::cout << "Connection established!\n";
		return true;
	}
	
	std::cout << "Connection timed out!\n";
	enet_peer_reset(netPeer);
	return false;
}

void GameClient::UpdateClient() {
	if (netHandle == nullptr) {
		return;
	}
	// Handle all incoming packets
	ENetEvent event;
	while (enet_host_service(netHandle, &event, 0) > 0) {
		if (event.type == ENET_EVENT_TYPE_CONNECT) {
			std::cout << "Connected to server!" << std::endl;
		}
		else if (event.type == ENET_EVENT_TYPE_RECEIVE) {
			GamePacket * packet = (GamePacket*)event.packet->data;
			ProcessPacket(packet);
		}
		enet_packet_destroy(event.packet);
	}
}

void GameClient::SendPacket(GamePacket&  payload) {
	ENetPacket* dataPacket = enet_packet_create(&payload, payload.GetTotalSize(), 0);
	enet_peer_send(netPeer, 0, dataPacket);
}
