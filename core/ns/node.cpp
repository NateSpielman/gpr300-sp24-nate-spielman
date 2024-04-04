#include "node.h"

namespace ns {
	Node createNode(glm::vec3 position, glm::quat rotation, glm::vec3 scale, unsigned int parentIndex) {
		Node node;
		node.position = position;
		node.rotation = rotation;
		node.scale = scale;
		node.parentIndex = parentIndex;

		return node;
	}
}