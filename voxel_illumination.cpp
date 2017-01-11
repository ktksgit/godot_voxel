
#include "voxel_illumination.h"

VoxelIllumination::VoxelIllumination() {
}

VoxelIllumination::~VoxelIllumination() {
}

static const Vector3i g_side_normals[Voxel::SIDE_COUNT] = {
    Vector3i(-1, 0, 0),
    Vector3i(1, 0, 0),
    Vector3i(0, -1, 0),
    Vector3i(0, 1, 0),
    Vector3i(0, 0, -1),
    Vector3i(0, 0, 1),
};

void VoxelIllumination::spread_ambient_light(unsigned int solid_channel, unsigned int light_channel,
		Set<Vector3i> & from_nodes, Set<Vector3i> & modified_blocks, int recursion_countdown) {

	ERR_FAIL_COND(recursion_countdown <= 0);

	if (from_nodes.size() == 0) {
		return;
	}

	Set<Vector3i> lightedNodes;

	VoxelBlock *block = NULL;

	bool checkedInModifiedList = false;

	for (Set<Vector3i>::Element* j = from_nodes.front(); j; j = j->next()) {
		Vector3i pos = j->get();
		Vector3i blockPos = _map->voxel_to_block(pos);
		Vector3i relPos = pos - _map->block_to_voxel(blockPos);

		VoxelBlock * lastBlock = block;
		block = _map->get_block(blockPos);
		if (block == NULL) {
			continue;
		}

		if (lastBlock != block) {
			checkedInModifiedList = false;
		}

		Light oldlight(block->voxels->get_voxel(relPos, light_channel));
		Light newlight = oldlight.diminish();

		for (int i = 0; i < 6; i++) {
			Vector3i neighbourPos = pos + g_side_normals[i];

			Vector3i blockPos = _map->voxel_to_block(neighbourPos);
			Vector3i relPos = neighbourPos - _map->block_to_voxel(blockPos);

			VoxelBlock * lastBlock = block;
			block = _map->get_block(blockPos);
			if (block == NULL) {
				continue;
			}

			if (lastBlock != block) {
				checkedInModifiedList = false;
			}

			Light neighbour(block->voxels->get_voxel(relPos, light_channel));

			bool changed = false;

			if (neighbour > oldlight.increase() && neighbour != Light(Light::LIGHT_MARKING)) {
				lightedNodes.insert(neighbourPos);
				changed = true;
			} else if (neighbour < newlight) {
				int solid_id = block->voxels->get_voxel(relPos, solid_channel);
				const Voxel & solid = _library->get_voxel_const(solid_id);

				if (solid.is_transparent()) {
					block->voxels->set_voxel(newlight.value, relPos,
							light_channel);
					lightedNodes.insert(neighbourPos);
					changed = true;
				}
			}

			if (changed == true && checkedInModifiedList == false) {
				modified_blocks.insert(blockPos);
				checkedInModifiedList = true;
			}
		}
	}

	if (lightedNodes.size() != 0) {
		from_nodes.clear();
		spread_ambient_light(solid_channel, light_channel, lightedNodes, modified_blocks, recursion_countdown - 1);
	}
}

void VoxelIllumination::remove_ambient_light(unsigned int solid_channel, unsigned int light_channel,
		Map<Vector3i, Light> & from_nodes, Set<Vector3i> & light_sources, Set<Vector3i> & modified_blocks,
		int recursion_countdown) {

	ERR_FAIL_COND(recursion_countdown <= 0);

	if (from_nodes.empty()) {
		return;
	}

	Map<Vector3i, Light> unlightedVoxels;

	VoxelBlock *block = NULL;

	bool checkedInModifiedList = false;

	for (Map<Vector3i, Light>::Element* j = from_nodes.front(); j; j = j->next()) {
		Vector3i pos = j->key();

		Light oldlight = j->get();
		ERR_FAIL_COND(oldlight.value == 0);
		for (int i = 0; i < 6; i++) {
			Vector3i neighborPos = pos + g_side_normals[i];
			Vector3i blockPos = _map->voxel_to_block(neighborPos);
			Vector3i relPos = neighborPos - _map->block_to_voxel(blockPos);

			VoxelBlock * lastBlock = block;
			block = _map->get_block(blockPos);
			if (block == NULL) {
				continue;
			}

			if (lastBlock != block) {
				checkedInModifiedList = false;
			}

			Light neighbourLight(block->voxels->get_voxel(relPos, light_channel));

			bool changed = false;

			if (neighbourLight < oldlight) {
				if (neighbourLight != Light(0)) {
					int solid_id = block->voxels->get_voxel(relPos, solid_channel);
					const Voxel & solid = _library->get_voxel_const(solid_id);

					if (solid.is_transparent()) {
						block->voxels->set_voxel(0, relPos, light_channel);

						unlightedVoxels[neighborPos] = neighbourLight;
						changed = true;
					}
				}
			} else if (neighbourLight != Light(0)) {
				light_sources.insert(neighborPos);
			}

			if (changed == true && checkedInModifiedList == false) {
				modified_blocks.insert(blockPos);
				checkedInModifiedList = true;
			}
		}
	}

	if (!unlightedVoxels.empty()) {
		from_nodes.clear();
		remove_ambient_light(solid_channel, light_channel, unlightedVoxels, light_sources, modified_blocks,
				recursion_countdown - 1);
	}
}

void VoxelIllumination::_bind_methods() {
    ObjectTypeDB::bind_method(_MD("set_library", "voxel_library:VoxelLibrary"), &VoxelIllumination::set_library);
    ObjectTypeDB::bind_method(_MD("set_map", "voxel_map:VoxelMap"), &VoxelIllumination::set_map);
    ObjectTypeDB::bind_method(_MD("spread_ambient_light:Vector3Array", "solid_channel:int", "light_channel:int", "from_nodes:Vector3Array"), &VoxelIllumination::_spread_ambient_light_binding);
    ObjectTypeDB::bind_method(_MD("remove_ambient_light:Vector3Array", "solid_channel:int", "light_channel:int", "from_nodes:Vector3Array"), &VoxelIllumination::_remove_ambient_light_binding);
}

DVector<Vector3> VoxelIllumination::_spread_ambient_light_binding(unsigned int solid_channel, unsigned int light_channel,
		const DVector<Vector3>& from_nodes) {

	Set<Vector3i> fromNodes;
	Set<Vector3i> modifiedBlocks;
	const DVector<Vector3>::Read readNodes = from_nodes.read();

	for (int i = 0; i < from_nodes.size(); i++) {
		fromNodes.insert(readNodes[i]);
	}

	spread_ambient_light(solid_channel, light_channel, fromNodes, modifiedBlocks, Light::LIGHT_MAX);

	DVector<Vector3> retModifiedBlocks;
	retModifiedBlocks.resize(modifiedBlocks.size());
	DVector<Vector3>::Write writeNodes = retModifiedBlocks.write();

	if (modifiedBlocks.size() != 0) {
		int i = 0;
		for (Set<Vector3i>::Element * e = modifiedBlocks.front(); e; e = e->next(), i++) {
			writeNodes[i] = e->get().to_vec3();
		}
	}

	return retModifiedBlocks;
}

DVector<Vector3> VoxelIllumination::_remove_ambient_light_binding(unsigned int solid_channel,
		unsigned int light_channel, const DVector<Vector3>& from_nodes) {

	Map<Vector3i, Light> unlightedVoxels;
	Set<Vector3i> lightSources;
	Set<Vector3i> modifiedBlocks;

	const DVector<Vector3>::Read readNodes = from_nodes.read();

	for (int i = 0; i < from_nodes.size(); i++) {
		Light l(_map->get_voxel(readNodes[i], light_channel));
		_map->set_voxel(0, readNodes[i], light_channel);
		unlightedVoxels[readNodes[i]] = l;
	}

	remove_ambient_light(solid_channel, light_channel, unlightedVoxels, lightSources, modifiedBlocks, Light::LIGHT_MAX);
	spread_ambient_light(solid_channel, light_channel, lightSources, modifiedBlocks, Light::LIGHT_MAX);

	DVector<Vector3> retModifiedBlocks;
	retModifiedBlocks.resize(modifiedBlocks.size());
	DVector<Vector3>::Write writeNodes = retModifiedBlocks.write();


	if (modifiedBlocks.size() != 0) {
		int i = 0;
		for (Set<Vector3i>::Element * e = modifiedBlocks.front(); e; e = e->next(), i++) {
			writeNodes[i] = e->get().to_vec3();
		}
	}

	return retModifiedBlocks;
}
