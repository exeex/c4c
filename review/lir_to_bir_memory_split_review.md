# `lir_to_bir_memory.cpp` Split Review

- Review base: `51ba6051` (`[plan] switch active runbook to local-memory family`)
- Why this base: it is the git-history activation checkpoint for the current active runbook sourced from `ideas/open/54_x86_backend_c_testsuite_capability_families.md`, and later history has only two implementation commits on that same runbook.
- Commit count since base: `2`

## Findings

### High: `lir_to_bir_memory.cpp` has already outgrown a single semantic ownership bucket, and further Step 3 work will keep forcing unrelated edits into the same TU

The existing `lir_to_bir` split architecture is by lowering responsibility, not by arbitrary file size. `BirFunctionLowerer` explicitly exposes shared buckets "used by the split lowering translation units" in [src/backend/bir/lir_to_bir.hpp](/workspaces/c4c/src/backend/bir/lir_to_bir.hpp:216), and neighboring files are already separated by concern: [lir_to_bir_scalar.cpp](/workspaces/c4c/src/backend/bir/lir_to_bir_scalar.cpp:1) is `553` lines, [lir_to_bir_module.cpp](/workspaces/c4c/src/backend/bir/lir_to_bir_module.cpp:1) is `704`, [lir_to_bir_globals.cpp](/workspaces/c4c/src/backend/bir/lir_to_bir_globals.cpp:1) is `955`, and [lir_to_bir_calling.cpp](/workspaces/c4c/src/backend/bir/lir_to_bir_calling.cpp:1) is `1069`, while [lir_to_bir_memory.cpp](/workspaces/c4c/src/backend/bir/lir_to_bir_memory.cpp:1) is `4926`.

That size would be tolerable only if the file owned one coherent memory subsystem. It does not. The helper declarations already mix at least three different ownership domains in one block:
- aggregate-layout and index walking helpers in [src/backend/bir/lir_to_bir.hpp](/workspaces/c4c/src/backend/bir/lir_to_bir.hpp:536)
- global-address and global-pointer provenance helpers in [src/backend/bir/lir_to_bir.hpp](/workspaces/c4c/src/backend/bir/lir_to_bir.hpp:551)
- local aggregate slot / pointer-array helpers in [src/backend/bir/lir_to_bir.hpp](/workspaces/c4c/src/backend/bir/lir_to_bir.hpp:588)

The implementation mirrors that collapse:
- shared aggregate/GEP traversal at [src/backend/bir/lir_to_bir_memory.cpp](/workspaces/c4c/src/backend/bir/lir_to_bir_memory.cpp:364)
- global-address and global dynamic-array resolution at [src/backend/bir/lir_to_bir_memory.cpp](/workspaces/c4c/src/backend/bir/lir_to_bir_memory.cpp:555)
- local aggregate slot and local dynamic-array resolution at [src/backend/bir/lir_to_bir_memory.cpp](/workspaces/c4c/src/backend/bir/lir_to_bir_memory.cpp:907)
- pointer provenance, addressed-global access, scratch-slot materialization, and the main opcode dispatcher at [src/backend/bir/lir_to_bir_memory.cpp](/workspaces/c4c/src/backend/bir/lir_to_bir_memory.cpp:1388) and [src/backend/bir/lir_to_bir_memory.cpp](/workspaces/c4c/src/backend/bir/lir_to_bir_memory.cpp:1508)

Recent churn confirms this is a route-quality problem, not just a stylistic one. The last five commits touching the file are all local-memory-family work (`0b11d52b`, `2fa68082`, `1e5f5676`, `3364124d`, `5c05ce2e`), and the latest diff still had to touch both [src/backend/bir/lir_to_bir_module.cpp](/workspaces/c4c/src/backend/bir/lir_to_bir_module.cpp:359) and [src/backend/bir/lir_to_bir_memory.cpp](/workspaces/c4c/src/backend/bir/lir_to_bir_memory.cpp:2923) to preserve pointer provenance. Leaving the monolith intact means every additional local-memory lane keeps widening the same ownership hotspot.

### Medium: the defensible split seam is not `local` versus `global`; it is `addressing/layout`, `pointer provenance`, and `opcode lowering`

A naive local/global split would be arbitrary and would either duplicate code or create excessive cross-TU back-and-forth. The strongest evidence is that the aggregate walkers are shared by both sides:
- `find_repeated_aggregate_extent_at_offset` in [src/backend/bir/lir_to_bir_memory.cpp](/workspaces/c4c/src/backend/bir/lir_to_bir_memory.cpp:364)
- `find_pointer_array_length_at_offset` in [src/backend/bir/lir_to_bir_memory.cpp](/workspaces/c4c/src/backend/bir/lir_to_bir_memory.cpp:507)
- `can_reinterpret_byte_storage_view` in [src/backend/bir/lir_to_bir_memory.cpp](/workspaces/c4c/src/backend/bir/lir_to_bir_memory.cpp:124)

Those helpers are semantic "addressing/layout" logic used by both global and local aggregate resolution. They should not be forced into only one side of a split.

The most defensible seams are:
- `lir_to_bir_memory_addressing.cpp`
  - own type/layout walking and GEP byte-offset resolution shared across local and global memory
  - candidate members: `can_reinterpret_byte_storage_view`, `find_repeated_aggregate_extent_at_offset`, `find_pointer_array_length_at_offset`, `resolve_relative_gep_target`, `resolve_global_gep_address`, `resolve_relative_global_gep_address`, `resolve_local_aggregate_gep_slot`, `resolve_local_aggregate_gep_target`, and the dynamic aggregate/pointer array address walkers
- `lir_to_bir_memory_provenance.cpp`
  - own pointer/global alias and addressed-object provenance, including values reloaded from pointer slots
  - candidate members: `collect_local_pointer_values`, `collect_global_array_pointer_values`, `record_pointer_global_object_alias`, `resolve_pointer_store_address`, `resolve_local_aggregate_pointer_value_alias`, `lower_call_pointer_arg_value`, `resolve_honest_pointer_base`, `resolve_honest_addressed_global_access`, `make_global_pointer_slot_key`
- keep `lir_to_bir_memory.cpp` as the coordinator for actual memory opcode lowering
  - keep `lower_scalar_or_local_memory_inst` in [src/backend/bir/lir_to_bir_memory.cpp](/workspaces/c4c/src/backend/bir/lir_to_bir_memory.cpp:1508) as the main admission surface that consumes the helper families

That split preserves semantic ownership. It also matches the current architecture better than creating files named after one testcase cluster or one temporary lane.

### Medium: the split is the right move, but not as hidden churn inside the current local-memory proving packet

The active `todo.md` says the current route likely needs a supervisor re-baseline because the original proving cluster is no longer coherent after recent capability movement; see [todo.md](/workspaces/c4c/todo.md:8). That means a structural split should not be smuggled into the next Step 3 semantic patch as "while I was here" cleanup. Doing so would blur whether the packet advanced the source idea's local-memory lane or only rearranged files.

The correct route is:
- treat the split as a distinct technical-debt slice tied to semantic ownership
- keep it narrow enough that no behavior change is claimed beyond file movement and helper relocation
- then resume whichever re-baselined local-memory packet the supervisor chooses

## Judgments

- Plan-alignment judgment: `on track` for the current implementation history; starting an arbitrary split inside the current packet sequence would be `drifting`
- Idea-alignment judgment: `matches source idea`
- Technical-debt judgment: `action needed`
- Validation sufficiency: `needs broader proof`

## Recommendation

- Reviewer recommendation: `narrow next packet`

Concrete interpretation:
- Yes, `src/backend/bir/lir_to_bir_memory.cpp` should be split now.
- No, it should not be split by line count or by a simplistic local/global divide.
- The first split packet should extract semantic helper families into `addressing/layout` and `pointer provenance`, while keeping `lower_scalar_or_local_memory_inst` as the coordinator in `lir_to_bir_memory.cpp`.
- Run at least a build plus the current narrow backend proof surface after the split, because these helpers sit on the active source idea's central local-memory route.
