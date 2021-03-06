#ifndef VOXEL_PROVIDER_TEST_H
#define VOXEL_PROVIDER_TEST_H

#include "voxel_provider.h"


class VoxelProviderTest : public VoxelProvider {
	OBJ_TYPE(VoxelProviderTest, VoxelProvider)

public:
	enum Mode {
		MODE_FLAT,
		MODE_WAVES
	};

	VoxelProviderTest();

	virtual void emerge_block(Ref<VoxelBuffer> out_buffer, Vector3i block_pos);

	void set_mode(Mode mode);
	Mode get_mode() const { return _mode; }

	void set_voxel_type(int t);
	int get_voxel_type() const;

	Vector3i get_pattern_size() const { return _pattern_size; }
	void set_pattern_size(Vector3i size);

	Vector3i get_pattern_offset() const { return _pattern_offset; }
	void set_pattern_offset(Vector3i offset);

protected:
	void generate_block_flat(VoxelBuffer & out_buffer, Vector3i block_pos);
	void generate_block_waves(VoxelBuffer & out_buffer, Vector3i block_pos);

	static void _bind_methods();

	Vector3 _get_pattern_size() const { return get_pattern_size().to_vec3(); }
	void _set_pattern_size(Vector3 size) { set_pattern_size(Vector3i(size)); }

	Vector3 _get_pattern_offset() const { return get_pattern_offset().to_vec3(); }
	void _set_pattern_offset(Vector3 offset) { set_pattern_offset(Vector3i(offset)); }

private:
	Mode _mode;
	int _voxel_type;
	Vector3i _pattern_offset;
	Vector3i _pattern_size;
};


#endif // VOXEL_PROVIDER_TEST_H

