#ifndef VOXEL_ILLUMINATION_H_
#define VOXEL_ILLUMINATION_H_

#include <core/reference.h>
#include <core/map.h>
#include <core/set.h>
#include "voxel.h"
#include "voxel_map.h"
#include "voxel_buffer.h"
#include "voxel_library.h"

struct VComp {
	inline bool operator()(const Vector3i& lhs, const Vector3i& rhs) const {
		return (lhs.x < rhs.x) && (lhs.y < rhs.y) && (lhs.z < rhs.z);
	}

};

class VoxelIllumination: public Reference {
	OBJ_TYPE(VoxelIllumination, Reference)

	Ref<VoxelLibrary> _library;
	Ref<VoxelMap> _map;

	class Light {
	public:
		uint8_t value;
		enum {
			LIGHT_MAX = 14,
			LIGHT_MARKING = 15
		};

		_FORCE_INLINE_ Light (int light) {
			value = uint8_t(light & 0x0000000f);
		}

		_FORCE_INLINE_ Light (uint8_t light) {
			value = light;
		}

		Light (const Light &light) {
			value = light.value;
		}

		_FORCE_INLINE_ Light diminish() {
			if (value == 0) {
				return Light(0);
			}
			if (value >= LIGHT_MAX) {
				return Light(LIGHT_MAX - 1);
			}

			return Light(value - 1);
		}

		_FORCE_INLINE_ Light increase() {
			if (value == 0) {
				return Light(0);
			}
			if (value == LIGHT_MAX) {
				return Light(int(value));
			}

			return Light(value + 1);
		}

		_FORCE_INLINE_ bool operator> (const Light& rhs) {
			return value > rhs.value;
		}

		_FORCE_INLINE_ bool operator< (const Light& rhs) {
			return value < rhs.value;
		}

		_FORCE_INLINE_ bool operator!= (const Light& rhs) {
			return value != rhs.value;
		}


	};
public:
	VoxelIllumination();
	virtual ~VoxelIllumination();

    void set_library(Ref<VoxelLibrary> library) {
        ERR_FAIL_COND(library.is_null());
        _library = library;
    }

    void set_map(Ref<VoxelMap> map) {
        ERR_FAIL_COND(map.is_null());
        _map = map;
    }

	void spread_ambient_light(unsigned int solid_channel, unsigned int light_channel,
			Set<Vector3i> & from_nodes,
			Map<Vector3i, VoxelBlock*> & modified_blocks, int recursion_countdown);

protected:

    static void _bind_methods();


private:
	void _spread_ambient_light_binding(unsigned int solid_channel,
			unsigned int light_channel, const DVector<Vector3>& from_nodes,
			Dictionary  modified_blocks);
};

#endif /* VOXEL_ILLUMINATION_H_ */
