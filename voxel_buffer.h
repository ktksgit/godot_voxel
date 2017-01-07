#ifndef VOXEL_BUFFER_H
#define VOXEL_BUFFER_H

#include <reference.h>
#include <vector.h>
#include "vector3i.h"

// Dense voxels data storage.
// Organized in 8-bit channels like images, all optional.
// Note: for float storage (marching cubes for example), you can map [0..256] to [0..1] and save 3 bytes per cell

class VoxelBuffer : public Reference {
	OBJ_TYPE(VoxelBuffer, Reference)

public:
	// Arbitrary value, 8 should be enough. Tweak for your needs.
	static const int MAX_CHANNELS = 8;

	VoxelBuffer();
	~VoxelBuffer();

	void create(int sx, int sy, int sz);
	void clear();
	void clear_channel(unsigned int channel_index, int clear_value=0);

	_FORCE_INLINE_ Vector3i get_size() const { return _size; }

	void set_default_values(uint16_t values[MAX_CHANNELS]);

	int get_voxel(int x, int y, int z, unsigned int channel_index=0) const;
	void set_voxel(int value, int x, int y, int z, unsigned int channel_index=0);
	void set_voxel_v(int value, Vector3 pos, unsigned int channel_index = 0);
	_FORCE_INLINE_ int get_voxel(const Vector3i pos, unsigned int channel_index = 0) const { return get_voxel(pos.x, pos.y, pos.z, channel_index); }
	_FORCE_INLINE_ void set_voxel(int value, const Vector3i pos, unsigned int channel_index = 0) { set_voxel(value, pos.x, pos.y, pos.z, channel_index); }

	void fill(int defval, unsigned int channel_index = 0);
	void fill_area(int defval, Vector3i min, Vector3i max, unsigned int channel_index = 0);

	bool is_uniform(unsigned int channel_index = 0);

	void optimize();

	void copy_from(const VoxelBuffer & other, unsigned int channel_index=0);
	void copy_from(const VoxelBuffer & other, Vector3i src_min, Vector3i src_max, Vector3i dst_min, unsigned int channel_index = 0);

	_FORCE_INLINE_ bool validate_pos(unsigned int x, unsigned int y, unsigned int z) const {
		return x < _size.x
			&& y < _size.y
			&& z < _size.x;
	}

	_FORCE_INLINE_ unsigned int index(unsigned int x, unsigned int y, unsigned int z) const {
		return (z * _size.z + x) * _size.x + y;
	}

	_FORCE_INLINE_ unsigned int row_index(unsigned int x, unsigned int y, unsigned int z) const {
		return (z * _size.z + x) * _size.x;
	}

	_FORCE_INLINE_ unsigned int get_volume() {
		return _size.x * _size.y * _size.z;
	}

private:
	void create_channel_noinit(int i, Vector3i size);
	void create_channel(int i, Vector3i size, uint16_t defval=0);
	void delete_channel(int i);

protected:
	static void _bind_methods();

	_FORCE_INLINE_ int get_size_x() const { return _size.x; }
	_FORCE_INLINE_ int get_size_y() const { return _size.y; }
	_FORCE_INLINE_ int get_size_z() const { return _size.z; }

	_FORCE_INLINE_ int _get_voxel_binding(int x, int y, int z, unsigned int channel) const { return get_voxel(x, y, z, channel); }
	_FORCE_INLINE_ void _set_voxel_binding(int value, int x, int y, int z, unsigned int channel) { set_voxel(value, x, y, z, channel); }
	void _copy_from_binding(Ref<VoxelBuffer> other, unsigned int channel);
	void _copy_from_area_binding(Ref<VoxelBuffer> other, Vector3 src_min, Vector3 src_max, Vector3 dst_min, unsigned int channel);
	_FORCE_INLINE_ void _fill_area_binding(int defval, Vector3 min, Vector3 max, unsigned int channel_index) { fill_area(defval, Vector3i(min), Vector3i(max), channel_index); }

private:
	struct Channel {
		// Allocated when the channel is populated.
		// Flat array, in order [z][x][y] because it allows faster vertical-wise access (the engine is Y-up).
		uint16_t * data;

		// Default value when data is null
		uint16_t defval;

		Channel() : data(NULL), defval(0) {}
	};

	// Each channel can store arbitary data.
	// For example, you can decide to store colors (R, G, B, A), gameplay types (type, state, light) or both.
	Channel _channels[MAX_CHANNELS];

	// How many voxels are there in the three directions. All populated channels have the same size.
	Vector3i _size;

};

#endif // VOXEL_BUFFER_H

