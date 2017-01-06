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

	Ref<Mesh> build_lighted(Ref<VoxelBuffer> buffer, int solid_channel, int light_channel);

protected:
	static void _bind_methods();

private:
	Ref<VoxelLibrary> _library;
	Ref<Material> _materials[MAX_MATERIALS];
	SurfaceTool _surface_tool[MAX_MATERIALS];
	float _baked_occlusion_darkness;
	bool _bake_occlusion;

};


#endif // VOXEL_MESHER
