#pragma once
//#include "./enet/enet.h"
struct _ENetHost;
struct _ENetPeer;
struct _ENetEvent;

enum BasicNetworkMessages {
	None,
	Hello,
	Message,
	String_Message,
	Delta_State,	//1 byte per channel since the last state
	Full_State,		//Full transform etc
	Received_State, //received from a client, informs that its received packet n
	Player_Connected,
	Player_Disconnected,
	Player_ID_Assignment,
	Score_Update,
	Request_Scores,
	All_Scores,
	Goose_Position,
	Shutdown
};

struct GamePacket {
	short size;
	short type;

	GamePacket() {
		type		= BasicNetworkMessages::None;
		size		= 0;
	}

	GamePacket(short type) : GamePacket() {
		this->type	= type;
	}

	int GetTotalSize() {
		return sizeof(GamePacket) + size;
	}
};

struct PlayerIDPacket : public GamePacket {
	int playerID;

	PlayerIDPacket(int id) {
		type = Player_ID_Assignment;
		size = sizeof(PlayerIDPacket) - sizeof(GamePacket);
		playerID = id;
	}
};

struct ScorePacket : public GamePacket {
	int playerID;
	int score;

	ScorePacket(int pid, int s) {
		type = Score_Update;
		size = sizeof(ScorePacket) - sizeof(GamePacket);
		playerID = pid;
		score = s;
	}
};

struct AllScoresPacket : public GamePacket {
	struct PlayerScore {
		int playerID;
		int score;
	};

	static const int MAX_PLAYERS = 8;
	int numPlayers;
	PlayerScore scores[MAX_PLAYERS];

	AllScoresPacket() {
		type = All_Scores;
		size = sizeof(AllScoresPacket) - sizeof(GamePacket);
		numPlayers = 0;
	}

	void AddScore(int playerID, int score) {
		if (numPlayers < MAX_PLAYERS) {
			scores[numPlayers].playerID = playerID;
			scores[numPlayers].score = score;
			numPlayers++;
		}
	}
};

struct GoosePacket : public GamePacket {
	float posX;
	float posY;
	float posZ;
	char isChasing;

	GoosePacket(float x, float y, float z, bool chasing) {
		type = Goose_Position;
		size = sizeof(GoosePacket) - sizeof(GamePacket);
		posX = x;
		posY = y;
		posZ = z;
		isChasing = chasing ? 1 : 0;
	}
};

struct StringPacket : public GamePacket {
	char stringData[256];
	
	StringPacket(const std::string& message) {
		type = BasicNetworkMessages::String_Message;
		size = (short)message.length();

		memcpy(stringData, message.data(), size);
	};
	
	std::string GetStringFromData() {
		std::string realString(stringData);
		realString.resize(size);
		return realString;
	}
};

class PacketReceiver {
public:
	virtual void ReceivePacket(int type, GamePacket* payload, int source = -1) = 0;
};

class NetworkBase	{
public:
	static void Initialise();
	static void Destroy();

	static int GetDefaultPort() {
		return 45000;
	}

	void RegisterPacketHandler(int msgID, PacketReceiver* receiver) {
		packetHandlers.insert(std::make_pair(msgID, receiver));
	}

	_ENetHost* netHandle;

protected:
	NetworkBase();
	~NetworkBase();

	bool ProcessPacket(GamePacket* p, int peerID = -1);

	typedef std::multimap<int, PacketReceiver*>::const_iterator PacketHandlerIterator;

	bool GetPacketHandlers(int msgID, PacketHandlerIterator& first, PacketHandlerIterator& last) const {
		auto range = packetHandlers.equal_range(msgID);

		if (range.first == packetHandlers.end()) {
			return false; //no handlers for this message type!
		}
		first	= range.first;
		last	= range.second;
		return true;
	}

	std::multimap<int, PacketReceiver*> packetHandlers;
};