
#include "voxel_illumination.h"

VoxelIllumination::VoxelIllumination() {
}

VoxelIllumination::~VoxelIllumination() {
}


void VoxelIllumination::spread_ambient_light(unsigned int solid_channel,
		unsigned int light_channel, Set<Vector3i> & from_nodes,
		Map<Vector3i, VoxelBlock*> & modified_blocks) {

	const Vector3i dirs[6] = {
			Vector3i(0, 0, 1), // back
			Vector3i(0, 1, 0), // top
			Vector3i(1, 0, 0), // right
			Vector3i(0, 0, -1), // front
			Vector3i(0, -1, 0), // bottom
			Vector3i(-1, 0, 0), // left
	};

	if (from_nodes.size() == 0) {
		return;
	}

	Set<Vector3i> lightedNodes;

	VoxelBlock *block = NULL;

	bool checkedInModifiedList = false;

	for (Set<Vector3i>::Element* j = from_nodes.front(); j != from_nodes.back();
			j = j->next()) {
		Vector3i pos = j->get();
		Vector3i blockPos = _map->voxel_to_block(pos);
		Vector3i relPos = pos - blockPos;

		VoxelBlock * lastBlock = block;
		block = _map->get_block(blockPos);
		if (lastBlock != block) {
			checkedInModifiedList = false;
		} else if (block == NULL) {
			continue;
		}

		Light oldlight = Light(block->voxels->get_voxel(relPos, light_channel));
		Light newlight = oldlight.diminish();

		for (int i = 0; i < 6; i++) {
			Vector3i neighbourPos = pos + dirs[i];

			Vector3i blockPos = _map->voxel_to_block(neighbourPos);
			Vector3i relPos = neighbourPos - blockPos;

			VoxelBlock * lastBlock = block;
			block = _map->get_block(blockPos);
			if (lastBlock != block) {
				checkedInModifiedList = false;
			} else if (block == NULL) {
				continue;
			}

			Light neighbour = Light(block->voxels->get_voxel(relPos, light_channel));

			bool changed = false;

			if (neighbour > oldlight.increase()) {
				lightedNodes.insert(neighbourPos);
				changed = true;
			} else if (neighbour < newlight) {
				int solid_id = block->voxels->get_voxel(relPos, solid_channel);
				const Voxel & solid = _library->get_voxel_const(solid_id);

				if (solid.is_transparent()) {
					neighbour = Light(newlight);
					block->voxels->set_voxel(neighbour.value, relPos,
							light_channel);
					lightedNodes.insert(neighbourPos);
					changed = true;
				}
			}

			if (changed == true && checkedInModifiedList == false) {
				if (modified_blocks.find(blockPos) == NULL) {
					modified_blocks[blockPos] = block;
				}
				checkedInModifiedList = true;
			}
		}
	}

	if (lightedNodes.size() != 0) {
		spread_ambient_light(solid_channel, light_channel, lightedNodes, modified_blocks);
	}
}

void VoxelIllumination::_bind_methods() {
    ObjectTypeDB::bind_method(_MD("set_library", "voxel_library:VoxelLibrary"), &VoxelIllumination::set_library);
    ObjectTypeDB::bind_method(_MD("set_map", "voxel_map:VoxelMap"), &VoxelIllumination::set_map);
}
