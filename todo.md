Status: Active
Source Idea Path: ideas/open/301_rv64_prepared_emitter_decomposition.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Functional Clusters

# Current Packet

## Just Finished

Completed Step 1: Inventory Functional Clusters for
`src/backend/mir/riscv/codegen/emit.cpp`. The current emitter clusters map as:

- Frame/support: signed-12-bit checks, stack-frame/slot alignment,
  prepared saved-register end calculation, `ra` spill offset selection inside
  function traversal, and `simple_frame_slot_sp_offset_for(...)`.
- Labels/traversal: label sanitizing/spelling, prepared function/block id
  lookup, `.globl`/function label emission, block iteration, instruction
  dispatch, terminator routing, and prologue/epilogue coordination.
- Scalar/value movement: prepared value-home lookup, register/immediate
  selection, move-to-register/location helpers, integer compare branches,
  binary add/sub/mul, casts, selects, pointer-add materialization, and scalar
  returns.
- Calls: direct fixed-arity scalar call-plan validation, argument register
  moves/materialization, call emission, and scalar result register move-back.
- Local memory: prepared frame-slot and pointer-value memory-access lookup,
  frame-slot address materialization, I8/I32/Ptr local load/store emission, and
  existing unaligned I32 bytewise stack load/store helpers.
- Edge publication: `EdgePublicationMoveIntent`, source operand rendering,
  route3/route5 agreement checks, `RiscvEdgePublicationMoveAdapter`, and the
  public `consume_edge_publication_move_intent(...)` /
  `append_edge_publication_move_instruction(...)` wrappers.
- Module/compatibility glue: `emit_prepared_module(...)`,
  `emit_prepared_lir_module(...)` tiny direct-LIR fallback, callee-saved
  register helpers, and `riscv::assemble_module(...)`.

Dependency/order notes:

- Extract frame/support and label helpers first because function traversal,
  scalar returns, local-memory stack offsets, and later owner files need them.
- Keep `emit.cpp` as compatibility glue while public declarations still live in
  `emit.hpp`; introduce new owner headers only for surfaces needed across
  owners.
- Scalar should follow support/traversal because calls and local memory reuse
  value-home lookup and move helpers.
- Local memory should follow scalar/support because load/store emission calls
  scalar move helpers and frame-slot offset helpers.
- Calls should follow scalar movement. Edge publication is already externally
  exposed and should wait until scalar/local-memory support is split enough to
  avoid pulling a broad helper pile into the edge owner.
- `src/backend/CMakeLists.txt` currently lists only
  `mir/riscv/codegen/emit.cpp`; each new `.cpp` extraction packet must add its
  owner file there.

## Suggested Next

First implementation packet: Extract the frame/label/basic support surface into
`src/backend/mir/riscv/codegen/prepared_frame_emit.{hpp,cpp}` with only
behavior-preserving moves. Move the signed-12-bit checks, stack-frame/slot
alignment, prepared saved-register end helper, frame-slot SP offset helpers,
and sanitized local-label spelling helpers needed by traversal. Add the new
`.cpp` to `src/backend/CMakeLists.txt`, keep `emit.cpp` compiling through
includes/wrappers as needed, and do not move scalar, call, local-memory, or
edge-publication emitters in this packet.

## Watchouts

- This idea is behavior-preserving. Do not add rv64 capabilities, weaken tests,
  accept BIR/LLVM fallback, or split by testcase/idea number.
- The first extraction should keep helper names boring and narrow; avoid a
  central `records`/`support` pile that would become the new monolith.
- `emit_i32_load_from_stack_offset(...)` is forward-declared before scalar
  movement but defined in the local-memory cluster; do not pull local-memory
  bytewise helpers into the first frame/label packet unless the dependency is
  explicitly resolved by a narrow declaration.
- `PhysReg` and the callee-saved constraint helpers sit in public namespace
  near the bottom and are compatibility glue for now, not part of the first
  frame extraction unless the compiler forces a tiny declaration move.

## Proof

Inventory-only scratchpad update; no build/tests run and no `test_after.log`
was produced because implementation files were not touched.

Recommended first implementation proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_rv64_runtime'`
