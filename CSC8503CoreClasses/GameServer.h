#pragma once
#include "NetworkBase.h"

namespace NCL {
	namespace CSC8503 {
		struct _ENetPeer;
		class GameWorld;

		class GameServer : public NetworkBase {
		public:
			GameServer(int onPort, int maxClients);
			~GameServer();

			bool Initialise();
			void Shutdown();

			void SetGameWorld(GameWorld &g);

			bool SendGlobalPacket(int msgID);
			bool SendGlobalPacket(GamePacket& packet);

			virtual void UpdateServer();

		protected:
			int			port;
			int			clientMax;
			int			clientCount;
			GameWorld*	gameWorld;

			int incomingDataRate;
			int outgoingDataRate;

		private:
			int nextPlayerID = 2;  // Start from 2 since server is 1
		};
	}
}
