#include "structs.hpp"

namespace RitoFile {
    struct MAPGEOPlanarReflector {
        Matrix4 transform;
        Container3<float> plane1;
        Container3<float> plane2;
        Container3<float> normal;
    };

    enum class MAPGEOBUcketGridFlag : std::uint8_t {
        HasFaceVisibilityFlags = 1 << 0
    };

    struct MAPGEOBucket {
        /*
        __slots__ = (
        'max_stickout_x', 'max_stickout_z',
        'start_index', 'base_vertex',
        'inside_face_count', 'sticking_out_face_count'
        )
        */
    };

    struct MAPGEOBucketGrid {
        /*
        __slots__ = (
        'hash',
        'min_x', 'min_z', 'max_x', 'max_z', 
        'max_stickout_x', 'max_stickout_z', 'bucket_size_x', 'bucket_size_z',
        'is_disabled', 'bucket_grid_flags',
        'buckets', 'vertices', 'indices',
        'face_layers'
        )
        */
    };

    struct MAPGEOTextureOverride {
        std::uint32_t index;
        std::string path;
    }

    struct MAPGEOChannel {
        std::string path;
        Container2<float> scale;
        Container2<float> offset;
    }

    enum class MAPGEORender : std::uint8_t {
        IsDecal = 1 << 0,
        HasEnvironmentDistortion = 1 << 1,
        RenderOnlyIfEyeCandyOn = 1 << 2,
        RenderOnlyIfEyeCandyOff = 1 << 3,
        CreateShadowBuffer = 1 << 4,
        CreateShadowMapMaterial = 1 << 5,
        UnkCreateDepthBuffer2 = 1 << 6,
        CreateDepthBuffer = 1 << 7
    };

    enum class MAPGEOQuality : std::uint8_t {
        VeryLow = 1 << 0,
        Low = 1 << 1,
        Medium = 1 << 2,
        High = 1 << 3,
        VeryHigh = 1 << 4
    };

    enum class MAPGEOLayer : std::uint8_t {
        Layer1 = 1 << 0,
        Layer2 = 1 << 1,
        Layer3 = 1 << 2,
        Layer4 = 1 << 3,
        Layer5 = 1 << 4,
        Layer6 = 1 << 5,
        Layer7 = 1 << 6,
        Layer8 = 1 << 7
    };

    struct MAPGEOSubmesh {
        /*
            __slots__ = (
        'name', 'hash',
        'index_start', 'index_count',
        'min_vertex', 'max_vertex'
    )   
        */
    };

    struct MAPGEOVertex {
        // std::map<std::String eu acho?, sei lá> value;
    };

    struct MAPGEOModel {
        /*
        __slots__ = (
        'name',
        'vertex_buffer_count', 'vertex_description_id', 'vertex_buffer_ids', 'vertex_count', 'vertices', 
        'index_buffer_id', 'index_count', 'indices',
        'layer', 'quality', 'disable_backface_culling', 'is_bush', 'render', 'point_light', 'light_probe', 
        'bucket_grid_hash', 'submeshes',
        'bounding_box', 'matrix', 
        'baked_light', 'stationary_light', 'texture_overrides', 'texture_overrides_scale_offset'
        )
        */
    };

    enum class MAPGEOVertexElementName : std::uint8_t {
        Position = 0,
        BlendWeight = 1,
        Normal = 2,
        FogCoordinate = 3,
        PrimaryColor = 4,
        SecondaryColor = 5,
        BlendIndex = 6,
        Texcoord0 = 7,
        Texcoord1 = 8,
        Texcoord2 = 9,
        Texcoord3 = 10,
        Texcoord4 = 11,
        Texcoord5 = 12,
        Texcoord6 = 13,
        Texcoord7 = 14,
        Tangent = 15
    };

    enum class MAPGEOVertexElementFormat : std::uint8_t {
        X_Float32 = 0,
        XY_Float32 = 1,
        XYZ_Float32 = 2,
        XYZW_Float32 = 3,
        BGRA_Packed8888 = 4,
        ZYXW_Packed8888 = 5,
        RGBA_Packed8888 = 6,
        XY_Packed1616  = 7,
        XYZ_Packed161616 = 8,
        XYZW_Packed16161616 = 9
    };

    struct MAPGEOVertexElement {
        MAPGEOVertexElementName name;
        MAPGEOVertexElementFormat format;
    };

    enum class MAPGEOVertexUsage : std::uint8_t {
        Static = 0,
        Dynamic = 1,
        Stream = 2
    };

    struct MAPGEOVertexDescription {
        MAPGEOVertexUsage usage;
        std::vector<MAPGEOVertexElement> elements;
    };

    class MAPGEO {
        /*
        __slots__ = (
        'signature', 'version', 'texture_overrides', 'vertex_descriptions',
        'models', 'bucket_grids', 'planar_reflectors'
        )
        */
    };
}