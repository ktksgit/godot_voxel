#ifndef VOXEL_MESHER
#define VOXEL_MESHER

#include <reference.h>
#include <scene/resources/mesh.h>
#include <scene/resources/surface_tool.h>
#include "voxel.h"
#include "voxel_buffer.h"
#include "voxel_library.h"

class VoxelMesher : public Reference {
	OBJ_TYPE(VoxelMesher, Reference)

public:
	static const unsigned int MAX_MATERIALS = 8; // Arbitrary. Tweak if needed.

	VoxelMesher();

	void set_material(Ref<Material> material, unsigned int id);
	Ref<Material> get_material(unsigned int id) const;

	void set_library(Ref<VoxelLibrary> library);
	Ref<VoxelLibrary> get_library() const { return _library; }

	void set_occlusion_darkness(float darkness);
	float get_occlusion_darkness() const { return _baked_occlusion_darkness; }

	void set_occlusion_enabled(bool enable);
	bool get_occlusion_enabled() const { return _bake_occlusion; }

    Ref<Mesh> build(const VoxelBuffer & buffer_ref, unsigned int channel_number);
    Ref<Mesh> build_ref(Ref<VoxelBuffer> buffer_ref, unsigned int channel_number);

private:
    Ref<Mesh> _build_lighted_binding(Ref<VoxelBuffer> buffer, int solid_channel, int light_channel, Vector3 block_pos_in_world) {
    	return build_lighted(buffer, solid_channel, light_channel, Vector3i(block_pos_in_world));
    }
public:
	Ref<Mesh> build_lighted(Ref<VoxelBuffer> buffer, int solid_channel, int light_channel, Vector3i block_pos_in_world);
	void set_randomize_factor (Vector3 factor, Vector3 block_size) {
		if (factor == Vector3(0, 0, 0)) {
			_randomize_corners = false;
		} else {
			_randomize_corners = true;
		}

		_randomize_factor = factor;
		_block_size = Vector3i(block_size);
	}

protected:
	static void _bind_methods();

private:
	Vector3 _fixed_randomize (Vector3 input) {
		uint32_t hash = Math::floor(input.x * 10 + 0.5);
		hash = 100 * Math::floor(input.y * 10 + 0.5) + hash;
		hash = 100 * Math::floor(input.z * 10 + 0.5) + hash;

		Vector3 out;
		uint32_t t = Math::rand_from_seed(&hash);
		t = Math::rand_from_seed(&hash);
		out.y = _randomize_factor.y * (t / float(Math::RANDOM_MAX) - 0.5);
		t =Math::rand_from_seed(&hash);
		out.z = _randomize_factor.z * (t / float(Math::RANDOM_MAX) - 0.5);
		t = Math::rand_from_seed(&hash);
		out.x = _randomize_factor.x * (t / float(Math::RANDOM_MAX) - 0.5);

		return out;
	}


	bool _randomize_corners;
	Vector3 _randomize_factor;
	Vector3i _block_size;


	Ref<VoxelLibrary> _library;
	Ref<Material> _materials[MAX_MATERIALS];
	SurfaceTool _surface_tool[MAX_MATERIALS];
	float _baked_occlusion_darkness;
	bool _bake_occlusion;


};


#endif // VOXEL_MESHER
