#include "voxel_mesher.h"
#include "voxel_library.h"


// The following tables respect the following conventions
//
//    7-------6
//   /|      /|
//  / |     / |  Corners
// 4-------5  |
// |  3----|--2
// | /     | /   y z
// |/      |/    |/
// 0-------1     o--x
//
//
//     o---10----o
//    /|        /|
//  11 7       9 6   Edges
//  /  |      /  |
// o----8----o   |
// |   o---2-|---o
// 4  /      5  /  
// | 3       | 1
// |/        |/
// o----0----o
//
// Sides are ordered according to the Voxel::Side enum.
//

static const unsigned int CORNER_COUNT = 8;
static const unsigned int EDGE_COUNT = 12;

static const Vector3 g_corner_position[CORNER_COUNT] = {
    Vector3(0, 0, 0),
    Vector3(1, 0, 0),
    Vector3(1, 0, 1),
    Vector3(0, 0, 1),
    Vector3(0, 1, 0),
    Vector3(1, 1, 0),
    Vector3(1, 1, 1),
    Vector3(0, 1, 1)
};

static const unsigned int g_side_coord[Voxel::SIDE_COUNT] = { 0, 0, 1, 1, 2, 2 };
static const unsigned int g_side_sign[Voxel::SIDE_COUNT] = { 0, 1, 0, 1, 0, 1 };

static const Vector3i g_side_normals[Voxel::SIDE_COUNT] = {
    Vector3i(-1, 0, 0),
    Vector3i(1, 0, 0),
    Vector3i(0, -1, 0),
    Vector3i(0, 1, 0),
    Vector3i(0, 0, -1),
    Vector3i(0, 0, 1),
};

static const unsigned int g_side_corners[Voxel::SIDE_COUNT][4] = {
    { 0, 3, 7, 4 },
    { 1, 2, 6, 5 },
    { 0, 1, 2, 3 },
    { 4, 5, 6, 7 },
    { 0, 1, 5, 4 },
    { 3, 2, 6, 7 }
};

static const unsigned int g_side_edges[Voxel::SIDE_COUNT][4] = {
    { 3, 7, 11, 4 },
    { 1, 6, 9, 5 },
    { 0, 1, 2, 3 },
    { 8, 9, 10, 11 },
    { 0, 5, 8, 4 },
    { 2, 6, 10, 7 }
};

// 3---2
// | / | {0,1,2,0,2,3}
// 0---1
//static const unsigned int g_vertex_to_corner[Voxel::SIDE_COUNT][6] = {
//    { 0, 3, 7, 0, 7, 4 },
//    { 2, 1, 5, 2, 5, 6 },
//    { 0, 1, 2, 0, 2, 3 },
//    { 7, 6, 5, 7, 5, 4 },
//    { 1, 0, 4 ,1, 4, 5 },
//    { 3, 2, 6, 3, 6, 7 }
//};

static const Vector3i g_corner_inormals[CORNER_COUNT] = {
    Vector3i(-1, -1, -1),
    Vector3i(1, -1, -1),
    Vector3i(1, -1, 1),
    Vector3i(-1, -1, 1),

    Vector3i(-1, 1, -1),
    Vector3i(1, 1, -1),
    Vector3i(1, 1, 1),
    Vector3i(-1, 1, 1)
};

static const Vector3i g_edge_inormals[EDGE_COUNT] = {
    Vector3i(0, -1, -1),
    Vector3i(1, -1, 0),
    Vector3i(0, -1, 1),
    Vector3i(-1, -1, 0),

    Vector3i(-1, 0, -1),
    Vector3i(1, 0, -1),
    Vector3i(1, 0, 1),
    Vector3i(-1, 0, 1),

    Vector3i(0, 1, -1),
    Vector3i(1, 1, 0),
    Vector3i(0, 1, 1),
    Vector3i(-1, 1, 0)
};

static const unsigned int g_edge_corners[EDGE_COUNT][2] = {
    { 0, 1 }, { 1, 2 }, { 2, 3 }, {3, 0},
    { 0, 4 }, { 1, 5 }, { 2, 6 }, {3, 7},
    { 4, 5 }, { 5, 6 }, { 6, 7 }, {7, 4}
};


VoxelMesher::VoxelMesher():
	_baked_occlusion_darkness(0.75),
	_bake_occlusion(true)
{}

void VoxelMesher::set_library(Ref<VoxelLibrary> library) {
    ERR_FAIL_COND(library.is_null());
    _library = library;
}

void VoxelMesher::set_material(Ref<Material> material, unsigned int id) {
    ERR_FAIL_COND(id >= MAX_MATERIALS);
    _materials[id] = material;
    _surface_tool[id].set_material(material);
}

Ref<Material> VoxelMesher::get_material(unsigned int id) const {
	ERR_FAIL_COND_V(id >= MAX_MATERIALS, Ref<Material>());
	return _materials[id];
}

void VoxelMesher::set_occlusion_darkness(float darkness) {
    _baked_occlusion_darkness = darkness;
    if (_baked_occlusion_darkness < 0.0)
        _baked_occlusion_darkness = 0.0;
    else if (_baked_occlusion_darkness >= 1.0)
        _baked_occlusion_darkness = 1.0;
}
   
void VoxelMesher::set_occlusion_enabled(bool enable) {
    _bake_occlusion = enable;
}

inline Color Color_greyscale(float c) { return Color(c, c, c); }

inline bool is_face_visible(const VoxelLibrary & lib, const Voxel & vt, int other_voxel_id, int other_face) {
    //if (other_voxel_id == 0) // air
    //    return true;
    if (lib.has_voxel(other_voxel_id)) {
        const Voxel & other_vt = lib.get_voxel_const(other_voxel_id);
        return (other_vt.is_transparent() && vt.get_id() != other_voxel_id) || !other_vt.is_face_visible(other_face);
    }
    return true;
}

inline bool is_transparent(const VoxelLibrary & lib, int voxel_id) {
    if (lib.has_voxel(voxel_id))
        return lib.get_voxel_const(voxel_id).is_transparent();
    return true;
}

Ref<Mesh> VoxelMesher::build_ref(Ref<VoxelBuffer> buffer_ref, unsigned int channel_number) {
    ERR_FAIL_COND_V(buffer_ref.is_null(), Ref<Mesh>());
    return build(**buffer_ref, channel_number);
}

Ref<Mesh> VoxelMesher::build(const VoxelBuffer & buffer, unsigned int channel_number) {
    ERR_FAIL_COND_V(_library.is_null(), Ref<Mesh>());

    const VoxelLibrary & library = **_library;

    for (unsigned int i = 0; i < MAX_MATERIALS; ++i) {
        _surface_tool[i].begin(Mesh::PRIMITIVE_TRIANGLES);
    }

    float baked_occlusion_darkness;
    if (_bake_occlusion)
        baked_occlusion_darkness = _baked_occlusion_darkness / 3.0;

    // The technique is Culled faces.
    // Could be improved with greedy meshing: https://0fps.net/2012/06/30/meshing-in-a-minecraft-game/
    // However I don't feel it's worth it yet:
    // - Not so much gain for organic worlds with lots of texture variations
    // - Works well with cubes but not with any shape
    // - Slower
    // => Could be implemented in a separate class?

    // Iterate 3D padded data to extract voxel faces.
    // This is the most intensive job in this class, so all required data should be as fit as possible.
    const Vector3i buffer_size = buffer.get_size();
    for (unsigned int z = 1; z < buffer_size.z-1; ++z) {
        for (unsigned int x = 1; x < buffer_size.x-1; ++x) {
            for (unsigned int y = 1; y < buffer_size.y-1; ++y) {

                int voxel_id = buffer.get_voxel(x, y, z, channel_number);

                if (library.has_voxel(voxel_id)) {

                    const Voxel & voxel = library.get_voxel_const(voxel_id);

                    SurfaceTool & st = _surface_tool[voxel.get_material_id()];

                    // Hybrid approach: extract cube faces and decimate those that aren't visible,
                    // and still allow voxels to have geometry that is not a cube

                    // Sides
                    for (unsigned int side = 0; side < Voxel::SIDE_COUNT; ++side) {

                    	if(!voxel.is_face_visible(side)) {
                    		continue;
                    	}

                        const DVector<Vector3> & vertices = voxel.get_model_side_vertices(side);
                        if (vertices.size() != 0) {

                            Vector3i normal = g_side_normals[side];
                            unsigned nx = x + normal.x;
                            unsigned ny = y + normal.y;
                            unsigned nz = z + normal.z;

                            int neighbor_voxel_id = buffer.get_voxel(nx, ny, nz, channel_number);
                            // TODO Better face visibility test
                            if (is_face_visible(library, voxel, neighbor_voxel_id, Voxel::opposite(side))) {

                                // The face is visible

                                int shaded_corner[8] = { 0 };

                                if (_bake_occlusion) {

                                    // Combinatory solution for https://0fps.net/2013/07/03/ambient-occlusion-for-minecraft-like-worlds/

                                    for (unsigned int j = 0; j < 4; ++j) {
                                        unsigned int edge = g_side_edges[side][j];
                                        Vector3i edge_normal = g_edge_inormals[edge];
                                        unsigned ex = x + edge_normal.x;
                                        unsigned ey = y + edge_normal.y;
                                        unsigned ez = z + edge_normal.z;
                                        if (!is_transparent(library, buffer.get_voxel(ex, ey, ez))) {
                                            shaded_corner[g_edge_corners[edge][0]] += 1;
                                            shaded_corner[g_edge_corners[edge][1]] += 1;
                                        }
                                    }
                                    for (unsigned int j = 0; j < 4; ++j) {
                                        unsigned int corner = g_side_corners[side][j];
                                        if (shaded_corner[corner] == 2) {
                                            shaded_corner[corner] = 3;
                                        }
                                        else {
                                            Vector3i corner_normal = g_corner_inormals[corner];
                                            unsigned int cx = x + corner_normal.x;
                                            unsigned int cy = y + corner_normal.y;
                                            unsigned int cz = z + corner_normal.z;
                                            if (!is_transparent(library, buffer.get_voxel(cx, cy, cz))) {
                                                shaded_corner[corner] += 1;
                                            }
                                        }
                                    }
                                }

                                DVector<Vector3>::Read rv = vertices.read();
                                DVector<Vector2>::Read rt = voxel.get_model_side_uv(side).read();
                                Vector3 pos(x - 1, y - 1, z - 1);

                                for (unsigned int i = 0; i < vertices.size(); ++i) {
                                    Vector3 v = rv[i];

                                    if (_bake_occlusion) {
                                        // General purpose occlusion colouring.
                                        // TODO Optimize for cubes
                                        // TODO Fix occlusion inconsistency caused by triangles orientation
                                        float shade = 0;
                                        for (unsigned int j = 0; j < 4; ++j) {
                                            unsigned int corner = g_side_corners[side][j];
                                            if (shaded_corner[corner]) {
                                                float s = baked_occlusion_darkness * static_cast<float>(shaded_corner[corner]);
                                                float k = 1.0 - g_corner_position[corner].distance_to(v);
                                                if (k < 0.0)
                                                    k = 0.0;
                                                s *= k;
                                                if (s > shade)
                                                    shade = s;
                                            }
                                        }
                                        float gs = 1.0 - shade;
                                        st.add_color(Color(gs, gs, gs));
                                    }

                                    st.add_normal(Vector3(normal.x, normal.y, normal.z));
                                    st.add_uv(rt[i]);
                                    st.add_vertex(v + pos);
                                }
                            }
                        }
                    }

                    // Inside
                    if (voxel.get_model_vertices().size() != 0) {

                        const DVector<Vector3> & vertices = voxel.get_model_vertices();
                        DVector<Vector3>::Read rv = voxel.get_model_vertices().read();
                        DVector<Vector3>::Read rn = voxel.get_model_normals().read();
                        DVector<Vector2>::Read rt = voxel.get_model_uv().read();
                        Vector3 pos(x - 1, y - 1, z - 1);

                        for (unsigned int i = 0; i < vertices.size(); ++i) {
                            st.add_normal(rn[i]);
                            st.add_uv(rt[i]);
                            st.add_vertex(rv[i] + pos);
                        }
                    }

                }

            }
        }
    }

    // Commit mesh
    int count_valid_materials = 0;
    Ref<Mesh> mesh_ref;
    for (unsigned int i = 0; i < MAX_MATERIALS; ++i) {
        if (_materials[i].is_valid()) {
        	count_valid_materials++;
            SurfaceTool & st = _surface_tool[i];
            
            // Index mesh to reduce memory usage and make upload to VRAM faster
            // TODO actually, we could make it indexed from the ground up without using SurfaceTool, so we also save time!
            st.index();

            mesh_ref = st.commit(mesh_ref);
            st.clear();
        }
    }

    ERR_FAIL_COND_V(count_valid_materials == 0, Ref<Mesh>());

    return mesh_ref;
}

void VoxelMesher::_bind_methods() {

	ObjectTypeDB::bind_method(_MD("set_material", "material", "id"), &VoxelMesher::set_material);
	ObjectTypeDB::bind_method(_MD("get_material:Material", "id"), &VoxelMesher::get_material);

    ObjectTypeDB::bind_method(_MD("set_library", "voxel_library"), &VoxelMesher::set_library);
	ObjectTypeDB::bind_method(_MD("get_library:VoxelLibrary"), &VoxelMesher::get_library);

    ObjectTypeDB::bind_method(_MD("set_occlusion_enabled", "enable"), &VoxelMesher::set_occlusion_enabled);
	ObjectTypeDB::bind_method(_MD("get_occlusion_enabled"), &VoxelMesher::get_occlusion_enabled);

    ObjectTypeDB::bind_method(_MD("set_occlusion_darkness", "value"), &VoxelMesher::set_occlusion_darkness);
	ObjectTypeDB::bind_method(_MD("get_occlusion_darkness"), &VoxelMesher::get_occlusion_darkness);

	ObjectTypeDB::bind_method(_MD("build", "voxel_buffer:VoxelBuffer", "channel_number:int"), &VoxelMesher::build_ref);

}
