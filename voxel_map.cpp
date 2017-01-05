#include "voxel_map.h"

VoxelMap::VoxelMap() : _last_accessed_block(NULL) {
	for (unsigned int i = 0; i < VoxelBuffer::MAX_CHANNELS; ++i) {
		_default_voxel[i] = 0;
	}
}

VoxelMap::~VoxelMap() {

}

int VoxelMap::get_voxel(Vector3i pos, unsigned int c) {
	Vector3i bpos = voxel_to_block(pos);
	VoxelBlock * block = get_block(bpos);
	if (block == NULL) {
		return _default_voxel[c];
	}
	return block->voxels->get_voxel(pos - block_to_voxel(bpos), c);
}

MeshInstance * VoxelBlock::get_mesh_instance(const Node & root) {
	if (mesh_instance_path.is_empty())
		return NULL;
	Node * n = root.get_node(mesh_instance_path);
	if (n == NULL)
		return NULL;
	return n->cast_to<MeshInstance>();
}

VoxelBlock::~VoxelBlock() {

}

// Helper
VoxelBlock * VoxelBlock::create(Vector3i bpos, VoxelBuffer * buffer) {
	VoxelBlock * block = memnew(VoxelBlock);
	block->pos = bpos;
	if (buffer) {
		const int bs = VoxelBlock::SIZE;
		ERR_FAIL_COND_V(buffer->get_size() != Vector3i(bs, bs, bs), NULL);
	}
	else {
		buffer = memnew(VoxelBuffer);
		buffer->create(SIZE, SIZE, SIZE);
	}
	ERR_FAIL_COND_V(buffer == NULL, NULL);
	block->voxels = Ref<VoxelBuffer>(buffer);
	//block->map = &map;
	return block;
}

void VoxelMap::set_voxel(int value, Vector3i pos, unsigned int c) {
	Vector3i bpos = voxel_to_block(pos);
	VoxelBlock * block = get_block(bpos);
	if (block == NULL) {
		block = VoxelBlock::create(bpos);
		set_block(bpos, block);
	}
	block->voxels->set_voxel(value, pos - block_to_voxel(bpos), c);
}

void VoxelMap::set_default_voxel(int value, unsigned int channel) {
	ERR_FAIL_INDEX(channel, VoxelBuffer::MAX_CHANNELS);
	_default_voxel[channel] = value;
}

int VoxelMap::get_default_voxel(unsigned int channel) {
	ERR_FAIL_INDEX_V(channel, VoxelBuffer::MAX_CHANNELS, 0);
	return _default_voxel[channel];
}

VoxelBlock * VoxelMap::get_block(Vector3i bpos) {
	if (_last_accessed_block && _last_accessed_block->pos == bpos) {
		return _last_accessed_block;
	}
	Ref<VoxelBlock> * p = _blocks.getptr(bpos);
	if (p) {
		_last_accessed_block = p->ptr();
		return _last_accessed_block;
	}
	return NULL;
}

void VoxelMap::set_block(Vector3i bpos, VoxelBlock * block) {
	if (_last_accessed_block == NULL || _last_accessed_block->pos == bpos) {
		_last_accessed_block = block;
	}
	_blocks.set(bpos, block);
}

void VoxelMap::set_block_buffer(Vector3i bpos, Ref<VoxelBuffer> buffer) {
	ERR_FAIL_COND(buffer.is_null());
	VoxelBlock * block = get_block(bpos);
	if (block == NULL) {
		block = VoxelBlock::create(bpos, *buffer);
		set_block(bpos, block);
	}
	else {
		block->voxels = buffer;
	}
}

bool VoxelMap::has_block(Vector3i pos) const {
	return /*(_last_accessed_block != NULL && _last_accessed_block->pos == pos) ||*/ _blocks.has(pos);
}

Vector3i g_moore_neighboring_3d[26] = {
	Vector3i(-1,-1,-1),
	Vector3i(0,-1,-1),
	Vector3i(1,-1,-1),
	Vector3i(-1,-1,0),
	Vector3i(0,-1,0),
	Vector3i(1,-1,0),
	Vector3i(-1,-1,1),
	Vector3i(0,-1,1),
	Vector3i(1,-1,1),

	Vector3i(-1,0,-1),
	Vector3i(0,0,-1),
	Vector3i(1,0,-1),
	Vector3i(-1,0,0),
	//Vector3i(0,0,0),
	Vector3i(1,0,0),
	Vector3i(-1,0,1),
	Vector3i(0,0,1),
	Vector3i(1,0,1),

	Vector3i(-1,1,-1),
	Vector3i(0,1,-1),
	Vector3i(1,1,-1),
	Vector3i(-1,1,0),
	Vector3i(0,1,0),
	Vector3i(1,1,0),
	Vector3i(-1,1,1),
	Vector3i(0,1,1),
	Vector3i(1,1,1),
};

bool VoxelMap::is_block_surrounded(Vector3i pos) const {
	for (unsigned int i = 0; i < 26; ++i) {
		Vector3i bpos = pos + g_moore_neighboring_3d[i];
		if (!has_block(bpos)) {
			return false;
		}
	}
	return true;
}

void VoxelMap::get_buffer_copy(Vector3i min_pos, VoxelBuffer & dst_buffer, unsigned int channel) {
	ERR_FAIL_INDEX(channel, VoxelBuffer::MAX_CHANNELS);

	Vector3i max_pos = min_pos + dst_buffer.get_size();

	Vector3i min_block_pos = voxel_to_block(min_pos);
	Vector3i max_block_pos = voxel_to_block(max_pos - Vector3i(1,1,1)) + Vector3i(1,1,1);
	ERR_FAIL_COND((max_block_pos - min_block_pos) != Vector3(3, 3, 3));

	Vector3i bpos;
	for (bpos.z = min_block_pos.z; bpos.z < max_block_pos.z; ++bpos.z) {
		for (bpos.x = min_block_pos.x; bpos.x < max_block_pos.x; ++bpos.x) {
			for (bpos.y = min_block_pos.y; bpos.y < max_block_pos.y; ++bpos.y) {

				VoxelBlock * block = get_block(bpos);
				if (block) {

					VoxelBuffer & src_buffer = **block->voxels;
					Vector3i offset = block_to_voxel(bpos);
					// Note: copy_from takes care of clamping the area if it's on an edge
					dst_buffer.copy_from(src_buffer, min_pos - offset, max_pos - offset, offset - min_pos, channel);
				}
				else {
					Vector3i offset = block_to_voxel(bpos);
					dst_buffer.fill_area(
						_default_voxel[channel],
						offset - min_pos,
						offset - min_pos + Vector3i(VoxelBlock::SIZE,VoxelBlock::SIZE, VoxelBlock::SIZE)
					);
				}

			}
		}
	}
}

void VoxelMap::remove_blocks_not_in_area(Vector3i min, Vector3i max) {

	Vector3i::sort_min_max(min, max);

	Vector<Vector3i> to_remove;
	Vector3i * key = NULL;

	while (_blocks.next(key)) {

		Ref<VoxelBlock> & block_ref = _blocks.get(*key);
		ERR_FAIL_COND(block_ref.is_null()); // Should never trigger
		VoxelBlock & block = **block_ref;

		if (!block.pos.is_contained_in(min, max)) {

			//if (_observer)
			//    _observer->block_removed(block);

			to_remove.push_back(*key);

			if (&block == _last_accessed_block)
				_last_accessed_block = NULL;
		}
	}

	for (unsigned int i = 0; i < to_remove.size(); ++i) {
		_blocks.erase(to_remove[i]);
	}
}

MeshInstance *VoxelMap::_get_block_mesh_instance_binding(Vector3 bpos, Node * root) {
	VoxelBlock* block = get_block(Vector3i(bpos));
	ERR_FAIL_COND_V(!block, NULL);

	return block->get_mesh_instance(*root);
}

void VoxelMap::_set_block_mesh_instance_binding(Vector3 bpos, Node * node) {
	ERR_FAIL_NULL(node);
	MeshInstance * mesh_instance = node->cast_to<MeshInstance>();
	VoxelBlock* block = get_block(Vector3i(bpos));

    if (block == NULL) {
        block = VoxelBlock::create(Vector3i(bpos));
        set_block(bpos, block);
    }

	block->mesh_instance_path = mesh_instance->get_path();
}

Ref<NavigationMesh> VoxelMap::_create_navigation_mesh_binding(Ref<Mesh> mesh) {
	ERR_FAIL_COND_V(mesh.is_null(), Ref<NavigationMesh>());

	Ref<NavigationMesh> navigation_mesh = Ref<NavigationMesh>(memnew(NavigationMesh));
	navigation_mesh->create_from_mesh(mesh);
	return navigation_mesh;
}

void VoxelMap::_bind_methods() {

	ObjectTypeDB::bind_method(_MD("get_voxel", "vector:Vector3", "channel:int"), &VoxelMap::_get_voxel_binding, DEFVAL(0));
    ObjectTypeDB::bind_method(_MD("set_voxel", "value:int", "vector:Vector3", "channel:int"), &VoxelMap::_set_voxel_binding, DEFVAL(0));
    ObjectTypeDB::bind_method(_MD("get_default_voxel", "channel"), &VoxelMap::get_default_voxel, DEFVAL(0));
    ObjectTypeDB::bind_method(_MD("set_default_voxel", "value", "channel"), &VoxelMap::set_default_voxel, DEFVAL(0));
    ObjectTypeDB::bind_method(_MD("has_block", "vector:Vector3"), &VoxelMap::_has_block_binding);
    ObjectTypeDB::bind_method(_MD("get_buffer_copy", "min_pos", "out_buffer:VoxelBuffer", "channels:Array"), &VoxelMap::_get_buffer_copy_binding);
    ObjectTypeDB::bind_method(_MD("set_block_buffer", "block_pos", "buffer:VoxelBuffer"), &VoxelMap::_set_block_buffer_binding);
    ObjectTypeDB::bind_method(_MD("voxel_to_block", "voxel_pos"), &VoxelMap::_voxel_to_block_binding);
    ObjectTypeDB::bind_method(_MD("block_to_voxel", "block_pos"), &VoxelMap::_block_to_voxel_binding);
    ObjectTypeDB::bind_method(_MD("get_block_size"), &VoxelMap::get_block_size);
	ObjectTypeDB::bind_method(_MD("get_block_mesh_instance:MeshInstance","block_pos:Vector3", "root:Node"),&VoxelMap::_get_block_mesh_instance_binding);
	ObjectTypeDB::bind_method(_MD("set_block_mesh_instance","block_pos:Vector3", "mesh_instance:MeshInstance"),&VoxelMap::_set_block_mesh_instance_binding);
	ObjectTypeDB::bind_method(_MD("create_navigation_mesh","mesh:Mesh"),&VoxelMap::_create_navigation_mesh_binding);

	//ADD_PROPERTY(PropertyInfo(Variant::INT, "iterations"), _SCS("set_iterations"), _SCS("get_iterations"));

}


void VoxelMap::_get_buffer_copy_binding(Vector3 pos, Ref<VoxelBuffer> dst_buffer_ref, DVector<int> channels) {
	ERR_FAIL_COND(dst_buffer_ref.is_null());

	if (channels.size() == 0) {
		return;
	}

	DVector<int>::Read read = channels.read();
	for (int i = 0; i < channels.size(); i++) {
		//TODO: pass all channels to get_buffer_copy
		get_buffer_copy(Vector3i(pos), **dst_buffer_ref, read[i]);
	}
}

