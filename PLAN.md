# MAPGEO Write Plan

## Goal
Make `MAPGEO::write` in the C++ code the source of truth for MAPGEO serialization, instead of depending on the Python conversion path.

## Why this needs to change
The current Python-based write logic is useful as a reference, but it is not reliable enough to serve as the production serializer. The C++ writer should mirror the read path and own the full binary layout so round-trips are deterministic.

## Primary design
Use the existing vertex-format table in [include/mapgeo.hpp](CXXRitoFile/include/mapgeo.hpp) as the shared basis for packing vertex data:

- `MAPGEOFormatMap` should define the byte size and item count for each `MAPGEOVertexElementFormat`.
- `getElementSize()` should remain the helper used anywhere byte size is needed.
- The writer should derive vertex buffer layout from the model’s vertex elements rather than hard-coding format sizes in multiple places.

## Implementation steps
1. Rebuild the writer around the reader’s structure.
   - Write the file signature and version first.
   - Serialize texture overrides, vertex descriptions, vertex buffers, index buffers, models, bucket grids, and planar reflectors in the same order used by `MAPGEO::read`.

2. Centralize vertex packing.
   - For each model, inspect its `MAPGEOVertex` data and derive a `MAPGEOVertexDescription`.
   - Use `MAPGEOFormatMap` to determine expected byte size and component count for each element.
   - Convert `VertexDataVariant` values into a flat binary buffer for each vertex buffer.

3. Make the writer version-aware.
   - Handle the supported versions explicitly instead of relying on Python-side heuristics.
   - Keep version-specific branches close to the read-path branches so the binary layout stays symmetric.

4. Preserve existing metadata.
   - Serialize model names, layer, quality, render flags, bucket-grid hashes, submeshes, bounding boxes, matrices, light channels, and texture overrides.
   - Keep default handling explicit for fields that are optional or version-dependent.

5. Write bucket grids and planar reflectors last.
   - Bucket grids should serialize counts, flags, vertices, indices, buckets, and face visibility data in the same shape that `read()` expects.
   - Planar reflectors should write the transform, plane bounds, and normal.

## Data rules to enforce
- A vertex buffer must only be emitted when its element formats are known and supported by `MAPGEOFormatMap`.
- Every emitted vertex element must map to a real `VertexDataVariant` alternative.
- The writer should fail fast if a model contains an unsupported combination of vertex element names or formats.
- The writer should keep `version_to_write` constrained to the versions the reader already supports.

## Validation plan
- Add or update a round-trip test in `TestRito.cpp` for at least one MAPGEO file.
- Verify that `read(write(x))` preserves the key structural data for supported versions.
- If binary equality is not practical, compare parsed fields and buffer counts.

## Recommended follow-up
After the writer is in place, remove any remaining dependency on the Python MAPGEO conversion path for production serialization and keep it only as a reference implementation if needed.