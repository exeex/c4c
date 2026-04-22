# X86 Codegen Rebuild Draft Index

This directory is the stage-3 Phoenix draft tree for the reviewed x86 codegen
replacement layout. The artifacts here are replacement contracts, not live
implementation.

## Scope

- preserve the exact stage-2 manifest from
  `docs/backend/x86_codegen_rebuild_plan.md`
- draft replacement ownership under `api`, `core`, `abi`, `module`,
  `lowering`, `prepared`, and `debug`
- keep prepared routes as bounded consumers of canonical seams instead of a
  second lowering stack
- keep the `peephole/` subtree out of claimed ownership during this rebuild

## Manifest Coverage

Top-level artifacts:

- [layout.md](/workspaces/c4c/docs/backend/x86_codegen_rebuild/layout.md)
- [review.md](/workspaces/c4c/docs/backend/x86_codegen_rebuild/review.md)

Ownership buckets:

- `api`: [x86_codegen_api.hpp.md](/workspaces/c4c/docs/backend/x86_codegen_rebuild/api/x86_codegen_api.hpp.md),
  [x86_codegen_api.cpp.md](/workspaces/c4c/docs/backend/x86_codegen_rebuild/api/x86_codegen_api.cpp.md)
- `core`: [x86_codegen_types.hpp.md](/workspaces/c4c/docs/backend/x86_codegen_rebuild/core/x86_codegen_types.hpp.md),
  [x86_codegen_output.hpp.md](/workspaces/c4c/docs/backend/x86_codegen_rebuild/core/x86_codegen_output.hpp.md),
  [x86_codegen_output.cpp.md](/workspaces/c4c/docs/backend/x86_codegen_rebuild/core/x86_codegen_output.cpp.md)
- `abi`: [x86_target_abi.hpp.md](/workspaces/c4c/docs/backend/x86_codegen_rebuild/abi/x86_target_abi.hpp.md),
  [x86_target_abi.cpp.md](/workspaces/c4c/docs/backend/x86_codegen_rebuild/abi/x86_target_abi.cpp.md)
- `module`: [module_emit.hpp.md](/workspaces/c4c/docs/backend/x86_codegen_rebuild/module/module_emit.hpp.md),
  [module_data_emit.hpp.md](/workspaces/c4c/docs/backend/x86_codegen_rebuild/module/module_data_emit.hpp.md),
  [module_emit.cpp.md](/workspaces/c4c/docs/backend/x86_codegen_rebuild/module/module_emit.cpp.md),
  [module_data_emit.cpp.md](/workspaces/c4c/docs/backend/x86_codegen_rebuild/module/module_data_emit.cpp.md)
- `lowering`: [frame_lowering.hpp.md](/workspaces/c4c/docs/backend/x86_codegen_rebuild/lowering/frame_lowering.hpp.md),
  [call_lowering.hpp.md](/workspaces/c4c/docs/backend/x86_codegen_rebuild/lowering/call_lowering.hpp.md),
  [return_lowering.hpp.md](/workspaces/c4c/docs/backend/x86_codegen_rebuild/lowering/return_lowering.hpp.md),
  [memory_lowering.hpp.md](/workspaces/c4c/docs/backend/x86_codegen_rebuild/lowering/memory_lowering.hpp.md),
  [comparison_lowering.hpp.md](/workspaces/c4c/docs/backend/x86_codegen_rebuild/lowering/comparison_lowering.hpp.md),
  [scalar_lowering.hpp.md](/workspaces/c4c/docs/backend/x86_codegen_rebuild/lowering/scalar_lowering.hpp.md),
  [float_lowering.hpp.md](/workspaces/c4c/docs/backend/x86_codegen_rebuild/lowering/float_lowering.hpp.md),
  [atomics_intrinsics_lowering.hpp.md](/workspaces/c4c/docs/backend/x86_codegen_rebuild/lowering/atomics_intrinsics_lowering.hpp.md),
  [frame_lowering.cpp.md](/workspaces/c4c/docs/backend/x86_codegen_rebuild/lowering/frame_lowering.cpp.md),
  [call_lowering.cpp.md](/workspaces/c4c/docs/backend/x86_codegen_rebuild/lowering/call_lowering.cpp.md),
  [return_lowering.cpp.md](/workspaces/c4c/docs/backend/x86_codegen_rebuild/lowering/return_lowering.cpp.md),
  [memory_lowering.cpp.md](/workspaces/c4c/docs/backend/x86_codegen_rebuild/lowering/memory_lowering.cpp.md),
  [comparison_lowering.cpp.md](/workspaces/c4c/docs/backend/x86_codegen_rebuild/lowering/comparison_lowering.cpp.md),
  [scalar_lowering.cpp.md](/workspaces/c4c/docs/backend/x86_codegen_rebuild/lowering/scalar_lowering.cpp.md),
  [float_lowering.cpp.md](/workspaces/c4c/docs/backend/x86_codegen_rebuild/lowering/float_lowering.cpp.md),
  [atomics_intrinsics_lowering.cpp.md](/workspaces/c4c/docs/backend/x86_codegen_rebuild/lowering/atomics_intrinsics_lowering.cpp.md)
- `prepared`: [prepared_query_context.hpp.md](/workspaces/c4c/docs/backend/x86_codegen_rebuild/prepared/prepared_query_context.hpp.md),
  [prepared_fast_path_dispatch.hpp.md](/workspaces/c4c/docs/backend/x86_codegen_rebuild/prepared/prepared_fast_path_dispatch.hpp.md),
  [prepared_fast_path_operands.hpp.md](/workspaces/c4c/docs/backend/x86_codegen_rebuild/prepared/prepared_fast_path_operands.hpp.md),
  [prepared_fast_path_dispatch.cpp.md](/workspaces/c4c/docs/backend/x86_codegen_rebuild/prepared/prepared_fast_path_dispatch.cpp.md),
  [prepared_fast_path_operands.cpp.md](/workspaces/c4c/docs/backend/x86_codegen_rebuild/prepared/prepared_fast_path_operands.cpp.md)
- `debug`: [prepared_route_debug.hpp.md](/workspaces/c4c/docs/backend/x86_codegen_rebuild/debug/prepared_route_debug.hpp.md),
  [prepared_route_debug.cpp.md](/workspaces/c4c/docs/backend/x86_codegen_rebuild/debug/prepared_route_debug.cpp.md)

## Step-1 Status

Step 1 materializes the full draft-tree skeleton and placeholder contracts so
later packets can fill content without drifting from the reviewed manifest.
