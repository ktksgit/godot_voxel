#include "voxel.h"
#include "voxel_library.h"
#include "voxel_mesher.h"

Voxel::Voxel() : Reference(), 
    _id(-1), 
    _material_id(0), 
    _is_transparent(false), 
	_hidden_faces(0),
    _library(NULL), 
    _color(1.f, 1.f, 1.f)
{}

Ref<Voxel> Voxel::set_id(int id) {
    ERR_FAIL_COND_V(id < 0 || id >= 65536, Ref<Voxel>(this));
    // Cannot modify ID after creation
    ERR_FAIL_COND_V(_id != -1, Ref<Voxel>(this));

    _id = id;

    return Ref<Voxel>(this); 
}

Ref<Voxel> Voxel::set_material_id(unsigned int id) {
    ERR_FAIL_COND_V(id >= VoxelMesher::MAX_MATERIALS, Ref<Voxel>(this));
    _material_id = id;
    return Ref<Voxel>(this);
}

void Voxel::hide_faces(Array faces) {
	for (int i = 0; i < faces.size(); i++) {
		int face = faces[i];
		_hidden_faces |= 1 << face;
	}
}

Ref<Voxel> Voxel::set_cube_geometry(float sy) {
    const Vector3 vertices[SIDE_COUNT][6] = {
        {
            // LEFT
            Vector3(0, 0, 0),
            Vector3(0, sy, 0),
            Vector3(0, sy, 1),
            Vector3(0, 0, 0),
            Vector3(0, sy, 1),
            Vector3(0, 0, 1),
        },
        {
            // RIGHT
            Vector3(1, 0, 0),
            Vector3(1, sy, 1),
            Vector3(1, sy, 0),
            Vector3(1, 0, 0),
            Vector3(1, 0, 1),
            Vector3(1, sy, 1)
        },
        {
            // BOTTOM
            Vector3(0, 0, 0),
            Vector3(1, 0, 1),
            Vector3(1, 0, 0),
            Vector3(0, 0, 0),
            Vector3(0, 0, 1),
            Vector3(1, 0, 1)
        },
        {
            // TOP
            Vector3(0, sy, 0),
            Vector3(1, sy, 0),
            Vector3(1, sy, 1),
            Vector3(0, sy, 0),
            Vector3(1, sy, 1),
            Vector3(0, sy, 1)
        },
        {
            // BACK
            Vector3(0, 0, 0),
            Vector3(1, 0, 0),
            Vector3(1, sy, 0),
            Vector3(0, 0, 0),
            Vector3(1, sy, 0),
            Vector3(0, sy, 0),
        },
        {
            // FRONT
            Vector3(1, 0, 1),
            Vector3(0, 0, 1),
            Vector3(1, sy, 1),
            Vector3(0, 0, 1),
            Vector3(0, sy, 1),
            Vector3(1, sy, 1)
        }
    };

    for (unsigned int side = 0; side < SIDE_COUNT; ++side) {
        _model_side_vertices[side].resize(6);
        DVector<Vector3>::Write w = _model_side_vertices[side].write();
        for (unsigned int i = 0; i < 6; ++i) {
            w[i] = vertices[side][i];
        }
    }

    return Ref<Voxel>(this);
}

Ref<Voxel> Voxel::_set_cube_uv_sides(const Vector2 atlas_pos[6]) {
    ERR_FAIL_COND_V(_library == NULL, Ref<Voxel>());

    float e = 0.001;
    const Vector2 uv[4] = {
        Vector2(e, e),
        Vector2(1.f - e, e),
        Vector2(e, 1.f - e),
        Vector2(1.f - e, 1.f - e),
    };

    const int uv6[SIDE_COUNT][6] = {
        // LEFT
        { 2,0,1,2,1,3 },
        // RIGHT
        { 2,1,0,2,3,1 },
        // BOTTOM
        { 0,3,1,0,2,3 },
        // TOP
        { 0,1,3,0,3,2 },
        // BACK
        { 2,3,1,2,1,0 },
        // FRONT
        { 3,2,1,2,0,1 }
    };

    float s = 1.0 / (float)_library->get_atlas_size();

    for (unsigned int side = 0; side < SIDE_COUNT; ++side) {
        _model_side_uv[side].resize(6);
        DVector<Vector2>::Write w = _model_side_uv[side].write();
        for (unsigned int i = 0; i < 6; ++i) {
            w[i] = (atlas_pos[side] + uv[uv6[side][i]]) * s;
        }
    }

    return Ref<Voxel>(this); 
}

Ref<Voxel> Voxel::set_cube_uv_all_sides(Vector2 atlas_pos) {
    const Vector2 positions[6] = {
        atlas_pos,
        atlas_pos,
        atlas_pos,
        atlas_pos,
        atlas_pos,
        atlas_pos
    };
    return _set_cube_uv_sides(positions);
}

Ref<Voxel> Voxel::set_cube_uv_tbs_sides(Vector2 top_atlas_pos, Vector2 side_atlas_pos, Vector2 bottom_atlas_pos) {
    const Vector2 positions[6] = {
        side_atlas_pos,
        side_atlas_pos,
        bottom_atlas_pos,
        top_atlas_pos,
        side_atlas_pos,
        side_atlas_pos,
    };
    return _set_cube_uv_sides(positions);
}

Ref<Voxel> Voxel::set_cube_geometry_from_mesh(Ref<Mesh> mesh) {
	ERR_FAIL_COND_V(mesh.is_null(), Ref<Voxel>());

	int surfaceIdx = 0;
	ERR_FAIL_COND_V(mesh->surface_get_primitive_type(surfaceIdx) != Mesh::PRIMITIVE_TRIANGLES, Ref<Voxel>());

	Array a = mesh->surface_get_arrays(surfaceIdx);

	int vc = mesh->surface_get_array_len(surfaceIdx);
	DVector<Vector3> vertices = a[Mesh::ARRAY_VERTEX];
	DVector<Vector3>::Read vr=vertices.read();

	DVector<Vector2> uvtex = a[Mesh::ARRAY_TEX_UV];
	DVector<Vector2>::Read uvtexr = uvtex.read();

	DVector<Vector3> normals = a[Mesh::ARRAY_NORMAL];
	DVector<Vector3>::Read normalsr = normals.read();

	int facecount = 0;
	if (mesh->surface_get_format(surfaceIdx)&Mesh::ARRAY_FORMAT_INDEX) {
		facecount=mesh->surface_get_array_index_len(surfaceIdx);
	} else {
		facecount = mesh->surface_get_array_len(surfaceIdx);
	}

	 _model_vertices.resize(facecount);
	DVector<Vector3>::Write facesw = _model_vertices.write();

	_model_uv.resize(facecount);
	DVector<Vector2>::Write texuvw = _model_uv.write();

	_model_normals.resize(facecount);
	DVector<Vector3>::Write normalsw = _model_normals.write();

	int widx=0;
	if (mesh->surface_get_format(surfaceIdx)&Mesh::ARRAY_FORMAT_INDEX) {

		int ic = mesh->surface_get_array_index_len(surfaceIdx);
		DVector<int> indices = a[Mesh::ARRAY_INDEX];
		DVector<int>::Read ir = indices.read();

		for(int i=0;i<ic;i++)  {
			facesw[widx] = vr[ ir[i] ];
			texuvw[widx] = uvtexr[ ir[i] ];
			normalsw[widx] = normalsr[ ir[i] ];

			widx++;
		}
	} else {
		for(int i=0;i<vc;i++) {
			facesw[widx] = vr[ i ];
			texuvw[widx] = uvtexr[i];
			normalsw[widx] = normalsr[ i];

			widx++;
		}
	}

	return Ref<Voxel>(this);
}

//Ref<Voxel> Voxel::set_xquad_geometry(Vector2 atlas_pos) {
//    // TODO
//    return Ref<Voxel>(this);
//}

void Voxel::_bind_methods() {

    ObjectTypeDB::bind_method(_MD("set_name:Voxel", "name"), &Voxel::set_name);
    ObjectTypeDB::bind_method(_MD("get_name"), &Voxel::get_name);

    ObjectTypeDB::bind_method(_MD("set_id:Voxel", "id"), &Voxel::set_id);
    ObjectTypeDB::bind_method(_MD("get_id"), &Voxel::get_id);

    ObjectTypeDB::bind_method(_MD("set_color:Voxel", "color"), &Voxel::set_color);
    ObjectTypeDB::bind_method(_MD("get_color"), &Voxel::get_color);

    ObjectTypeDB::bind_method(_MD("set_transparent:Voxel", "color"), &Voxel::set_transparent, DEFVAL(true));
    ObjectTypeDB::bind_method(_MD("is_transparent"), &Voxel::is_transparent);

    ObjectTypeDB::bind_method(_MD("set_material_id", "id"), &Voxel::set_material_id);
    ObjectTypeDB::bind_method(_MD("get_material_id"), &Voxel::get_material_id);

    ObjectTypeDB::bind_method(_MD("hide_faces", "faces:Array"), &Voxel::hide_faces);

    ObjectTypeDB::bind_method(_MD("set_cube_geometry:Voxel", "height"), &Voxel::set_cube_geometry, DEFVAL(1.f));
    ObjectTypeDB::bind_method(_MD("set_cube_uv_all_sides:Voxel", "atlas_pos"), &Voxel::set_cube_uv_all_sides);
    ObjectTypeDB::bind_method(_MD("set_cube_uv_tbs_sides:Voxel", "top_atlas_pos", "side_atlas_pos", "bottom_atlas_pos"), &Voxel::set_cube_uv_tbs_sides);
    ObjectTypeDB::bind_method(_MD("set_cube_geometry_from_mesh:Voxel", "mesh:Mesh"), &Voxel::set_cube_geometry_from_mesh);

    BIND_CONSTANT(SIDE_LEFT);
    BIND_CONSTANT(SIDE_RIGHT);
    BIND_CONSTANT(SIDE_BOTTOM);
    BIND_CONSTANT(SIDE_TOP);
    BIND_CONSTANT(SIDE_BACK);
    BIND_CONSTANT(SIDE_FRONT);
}

