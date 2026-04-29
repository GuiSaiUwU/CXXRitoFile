#include "structs.hpp"
#include <variant>
#include <map>
#include <string>
#include "binary_stream.hpp"
#include <format>
#include <array>


namespace RitoFile {
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

    struct MAPGEOPlanarReflector {
        Matrix4 transform;
        Container2<Container3<float>> plane; // min, max
        Container3<float> normal;
    };

    enum class MAPGEOBucketGridFlag : std::uint8_t {
        HasFaceVisibilityFlags = 1 << 0
    };

    struct MAPGEOBucket {
        float max_stickout_x, max_stickout_z;
        std::uint32_t start_index, base_vertex;
        std::uint16_t inside_face_count, sticking_out_face_count;
    };

    struct MAPGEOBucketGrid {
        std::uint32_t hash;
        float min_x, min_z, max_x, max_z, max_stickout_x, max_stickout_z, bucket_size_x, bucket_size_z;
        MAPGEOBucketGridFlag bucket_grid_flags;
        std::vector<MAPGEOBucket> buckets;
        std::vector<Container3<float>> vertices;
        std::vector<std::uint16_t> indices;
        std::vector<MAPGEOBucketGridFlag> face_layers;
        bool is_disabled;
    };

    struct MAPGEOTextureOverride {
        std::uint32_t index;
        std::string path;
    };

    struct MAPGEOChannel {
        std::string path;
        Container2<float> scale;
        Container2<float> offset;
    };

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
        std::uint32_t hash;
        std::string name;
        std::uint32_t index_start, index_count, min_vertex, max_vertex;
    };

    struct MAPGEOFormatInfo {
        std::uint32_t byte_size;
        std::uint32_t item_count;
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

    static const std::map<MAPGEOVertexElementFormat, MAPGEOFormatInfo> MAPGEOFormatMap = {
        {MAPGEOVertexElementFormat::X_Float32,          {4, 1}},
        {MAPGEOVertexElementFormat::XY_Float32,         {8, 2}},
        {MAPGEOVertexElementFormat::XYZ_Float32,        {12, 3}},
        {MAPGEOVertexElementFormat::XYZW_Float32,       {16, 4}},
        {MAPGEOVertexElementFormat::BGRA_Packed8888,    {4, 4}},
        {MAPGEOVertexElementFormat::ZYXW_Packed8888,    {4, 4}},
        {MAPGEOVertexElementFormat::RGBA_Packed8888,    {4, 4}},
        {MAPGEOVertexElementFormat::XY_Packed1616,      {4, 2}},
        {MAPGEOVertexElementFormat::XYZ_Packed161616,   {8, 4}},  // 8 bytes with padding
        {MAPGEOVertexElementFormat::XYZW_Packed16161616,{8, 4}}
    };

    static std::uint32_t getElementSize(MAPGEOVertexElementFormat format) {
        auto it = MAPGEOFormatMap.find(format);
        if (it != MAPGEOFormatMap.end()) {
            return it->second.byte_size;
        }
        // Throw or return 0 for unknown formats
        throw std::runtime_error(std::format("Unknown vertex element format: {}",
            static_cast<int>(format)));
    }

    using VertexDataVariant = std::variant<
        std::monostate,
        float,                // X_Float32
        Container2<float>,    // XY_Float32
        Container3<float>,    // XYZ_Float32
        Container4<float>,    // XYZW_Float32
        Container4<uint8_t>,  // BGRA/RGBA/ZYXW_Packed8888
        Container2<uint16_t>, // XY_Packed1616
        Container3<uint16_t>, // XYZ_Packed161616 (with padding)
        Container4<uint16_t>  // XYZW_Packed16161616
    >;

    struct MAPGEOVertex {
        std::map<
            MAPGEOVertexElementName,
            VertexDataVariant
        >
        value;
    };

    struct MAPGEOModel {
        std::string name;
        std::uint32_t
            vertex_buffer_count, vertex_count, index_count,
            vertex_description_id, index_buffer_id,
            bucket_grid_hash;
        
        std::vector<std::uint32_t> vertex_buffer_ids;
        std::vector<MAPGEOVertex> vertices;
        std::vector<std::uint16_t> indices;
        MAPGEOLayer layer;
        MAPGEOQuality quality;
        bool disable_backface_culling, is_bush;
        MAPGEORender render;
        Container3<float> point_light;
        Container3<
            std::array<float, 9> // 27 floats, 9 for each RGB channel
        > light_probe;
        std::vector<MAPGEOSubmesh> submeshes;
        Container2<Container3<float>> bounding_box; // min, max
        Matrix4 matrix;
        MAPGEOChannel baked_light, stationary_light;
        std::vector<MAPGEOTextureOverride> texture_overrides;
        Container4<float> texture_overrides_scale_offset;
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
    public:
        std::string signature;
        std::uint32_t version;
        std::vector<MAPGEOTextureOverride> texture_overrides;
        std::vector<MAPGEOVertexDescription> vertex_descriptions;
        std::vector<MAPGEOModel> models;
        std::vector<MAPGEOBucketGrid> bucket_grids;
        std::vector<MAPGEOPlanarReflector> planar_reflectors;
        /* uwu */

        BinaryReader reader;
        MAPGEO(std::stringstream& inpt_file);
        void read();
    };

    class MAPGEOVertexReader {
    public:
        static VertexDataVariant readVertexElement(BinaryReader& reader, MAPGEOVertexElementFormat format) {
            switch (format) {
                case MAPGEOVertexElementFormat::X_Float32: {
                    return reader.readF32();
                }

                case MAPGEOVertexElementFormat::XY_Float32: {
                    Container2<float> vec;
                    vec.x = reader.readF32();
                    vec.y = reader.readF32();
                    return vec;
                }
                
                case MAPGEOVertexElementFormat::XYZ_Float32: {
                    Container3<float> vec;
                    vec.x = reader.readF32();
                    vec.y = reader.readF32();
                    vec.z = reader.readF32();
                    return vec;
                }
                
                case MAPGEOVertexElementFormat::XYZW_Float32: {
                    Container4<float> vec;
                    vec.x = reader.readF32();
                    vec.y = reader.readF32();
                    vec.z = reader.readF32();
                    vec.w = reader.readF32();
                    return vec;
                }
                
                case MAPGEOVertexElementFormat::BGRA_Packed8888:
                case MAPGEOVertexElementFormat::ZYXW_Packed8888:
                case MAPGEOVertexElementFormat::RGBA_Packed8888: {
                    Container4<uint8_t> packed;
                    packed.x = reader.readU8();
                    packed.y = reader.readU8();
                    packed.z = reader.readU8();
                    packed.w = reader.readU8();
                    return packed;
                }
                
                case MAPGEOVertexElementFormat::XY_Packed1616: {
                    Container2<uint16_t> packed;
                    packed.x = reader.readU16();
                    packed.y = reader.readU16();
                    return packed;
                }
                
                case MAPGEOVertexElementFormat::XYZ_Packed161616: {
                    Container3<uint16_t> packed;
                    packed.x = reader.readU16();
                    packed.y = reader.readU16();
                    packed.z = reader.readU16();
                    reader.pad(2); // padding
                    return packed;
                }

                case MAPGEOVertexElementFormat::XYZW_Packed16161616: {
                    Container4<uint16_t> packed;
                    packed.x = reader.readU16();
                    packed.y = reader.readU16();
                    packed.z = reader.readU16();
                    packed.w = reader.readU16();
                    return packed;
                }
                
                default:
                    throw std::runtime_error(std::format("Unknown vertex element format: {}", static_cast<int>(format)));
            }
        }
        
        static MAPGEOVertex readVertex(BinaryReader& reader, 
                                    const std::vector<MAPGEOVertexElement>& elements) {
            MAPGEOVertex vertex;
            
            for (const auto& element : elements) {
                vertex.value[element.name] = readVertexElement(reader, element.format);
            }
            
            return vertex;
        }
    };
}