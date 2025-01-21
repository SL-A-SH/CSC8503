#include "NavigationGrid.h"
#include "Assets.h"

#include <fstream>

using namespace NCL;
using namespace CSC8503;

const int LEFT_NODE = 0;
const int RIGHT_NODE = 1;
const int TOP_NODE = 2;
const int BOTTOM_NODE = 3;

const char WALL_NODE = 'x';
const char FLOOR_NODE = '.';

NavigationGrid::NavigationGrid() {
	nodeSize = 0;
	gridWidth = 0;
	gridHeight = 0;
	allNodes = nullptr;
}

NavigationGrid::NavigationGrid(const std::string& filename) : NavigationGrid() {
	std::ifstream infile(Assets::DATADIR + filename);

	infile >> nodeSize;
	infile >> gridWidth;
	infile >> gridHeight;

	std::cout << "Creating NavGrid: " << gridWidth << "x" << gridHeight <<
		" nodes, size " << nodeSize << "\n";

	allNodes = new GridNode[gridWidth * gridHeight];

	// Read in the node types first
	for (int y = 0; y < gridHeight; ++y) {
		for (int x = 0; x < gridWidth; ++x) {
			GridNode& n = allNodes[(gridWidth * y) + x];
			char type = 0;
			infile >> type;
			n.type = type;
			n.position = Vector3((float)(x * nodeSize), 0, (float)(y * nodeSize));
		}
	}

	// Now build connections, ignoring walls
	for (int y = 0; y < gridHeight; ++y) {
		for (int x = 0; x < gridWidth; ++x) {
			GridNode& n = allNodes[(gridWidth * y) + x];

			// Skip making connections if this is a wall
			if (n.type == 'x') {
				continue;
			}

			if (y > 0) { //get the above node
				GridNode* above = &allNodes[(gridWidth * (y - 1)) + x];
				n.connected[0] = above->type == 'x' ? nullptr : above;
				if (n.connected[0]) {
					n.costs[0] = 1;
				}
			}
			if (y < gridHeight - 1) { //get the below node
				GridNode* below = &allNodes[(gridWidth * (y + 1)) + x];
				n.connected[1] = below->type == 'x' ? nullptr : below;
				if (n.connected[1]) {
					n.costs[1] = 1;
				}
			}
			if (x > 0) { //get left node
				GridNode* left = &allNodes[(gridWidth * y) + (x - 1)];
				n.connected[2] = left->type == 'x' ? nullptr : left;
				if (n.connected[2]) {
					n.costs[2] = 1;
				}
			}
			if (x < gridWidth - 1) { //get right node
				GridNode* right = &allNodes[(gridWidth * y) + (x + 1)];
				n.connected[3] = right->type == 'x' ? nullptr : right;
				if (n.connected[3]) {
					n.costs[3] = 1;
				}
			}
		}
	}
}

NavigationGrid::~NavigationGrid() {
	delete[] allNodes;
}

bool NavigationGrid::FindPath(const Vector3& from, const Vector3& to, NavigationPath& outPath) {

	// Convert world coordinates to grid coordinates
	// Shift coordinates to be relative to grid center
	int fromX = (int)((from.x + (nodeSize * gridWidth / 2)) / nodeSize);
	// Invert Z coordinate since grid starts at top
	int fromZ = (int)(((-from.z) + (nodeSize * gridHeight / 2)) / nodeSize);

	int toX = (int)((to.x + (nodeSize * gridWidth / 2)) / nodeSize);
	int toZ = (int)(((-to.z) + (nodeSize * gridHeight / 2)) / nodeSize);

	// Verify within grid bounds and not in walls
	if (fromX < 0 || fromX >= gridWidth || fromZ < 0 || fromZ >= gridHeight ||
		toX < 0 || toX >= gridWidth || toZ < 0 || toZ >= gridHeight) {
		return false;
	}

	// Check if start/end points are walls
	GridNode& startNode = allNodes[(fromZ * gridWidth) + fromX];
	GridNode& endNode = allNodes[(toZ * gridWidth) + toX];

	if (startNode.type == WALL_NODE || endNode.type == WALL_NODE) {
		return false;
	}

	std::vector<GridNode*> openList;
	std::vector<GridNode*> closedList;

	openList.push_back(&startNode);

	startNode.f = 0;
	startNode.g = 0;
	startNode.parent = nullptr;

	GridNode* currentBestNode = nullptr;

	while (!openList.empty()) {
		currentBestNode = RemoveBestNode(openList);

		if (currentBestNode == &endNode) {
			// Found the path! Convert grid coords to world coords
			GridNode* node = currentBestNode;
			while (node != nullptr) {
				int gridX = (int)(node - allNodes) % gridWidth;
				int gridZ = (int)(node - allNodes) / gridWidth;

				// Convert back to world coordinates
				float worldX = (gridX * nodeSize) - (nodeSize * gridWidth / 2);
				// Invert Z back to world coordinates
				float worldZ = -(gridZ * nodeSize - (nodeSize * gridHeight / 2));

				outPath.PushWaypoint(Vector3(worldX, 0, worldZ));
				node = node->parent;
			}

			// Debug waypoints
			NavigationPath tempPath = outPath;
			Vector3 waypoint;
			while (tempPath.PopWaypoint(waypoint)) {
				/*std::cout << "Waypoint world pos: " << waypoint.x << "," << waypoint.z << "\n";*/
			}
			return true;
		}
		else {
			for (int i = 0; i < 4; ++i) {
				GridNode* neighbour = currentBestNode->connected[i];
				if (!neighbour) {
					continue;
				}
				bool inClosed = NodeInList(neighbour, closedList);
				if (inClosed) {
					continue;
				}

				float h = Heuristic(neighbour, &endNode);
				float g = currentBestNode->g + currentBestNode->costs[i];
				float f = h + g;

				bool inOpen = NodeInList(neighbour, openList);

				if (!inOpen) {
					openList.emplace_back(neighbour);
				}
				if (!inOpen || f < neighbour->f) {
					neighbour->parent = currentBestNode;
					neighbour->f = f;
					neighbour->g = g;
				}
			}
			closedList.emplace_back(currentBestNode);
		}
	}

	return false;
}

bool NavigationGrid::NodeInList(GridNode* n, std::vector<GridNode*>& list) const {
	std::vector<GridNode*>::iterator i = std::find(list.begin(), list.end(), n);
	return i == list.end() ? false : true;
}

GridNode* NavigationGrid::RemoveBestNode(std::vector<GridNode*>& list) const {
	std::vector<GridNode*>::iterator bestI = list.begin();

	GridNode* bestNode = *list.begin();

	for (auto i = list.begin(); i != list.end(); ++i) {
		if ((*i)->f < bestNode->f) {
			bestNode = (*i);
			bestI = i;
		}
	}
	list.erase(bestI);

	return bestNode;
}

float NavigationGrid::Heuristic(GridNode* hNode, GridNode* endNode) const {
	return Vector::Length(hNode->position - endNode->position);
}