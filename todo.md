Status: Active
Source Idea Path: ideas/open/243_phase_f0_x86_riscv_bir_portability_convergence_audit.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Split Target Policy from Semantic Authority

# Current Packet

## Just Finished

Completed plan.md Step 4 analysis for splitting target wrapper policy from
semantic authority without moving target policy into BIR.

Inspected source/symbol surfaces:

- `src/backend/mir/x86/x86.hpp`: `ConsumedPlans`,
  `shared_function_lookups()`, `shared_call_plan_lookups()`,
  `shared_route6_call_use_source_index()`,
  `find_consumed_call_argument_plan(...)`,
  `find_consumed_scalar_i32_call_argument_source(...)`, and
  `consume_plans(...)`.
- `src/backend/mir/x86/prepared/dispatch.cpp`: x86
  `consume_edge_publication_move_intent(...)`,
  `append_edge_publication_move_instruction(...)`, and local
  `render_edge_publication_source_operand(...)`.
- `src/backend/mir/x86/module/module.cpp`: compare-join edge move wrapper
  calls into x86 prepared edge-publication intent, direct extern call argument
  wrappers consume Route 6 under prepared agreement, and x86 wrapper code keeps
  call-bundle, frame, register, symbol, and instruction spelling policy local.
- `src/backend/mir/x86/debug/debug.cpp`: route-debug output for Route 6 scalar
  call argument source rows.
- `src/backend/mir/riscv/codegen/emit.cpp` and
  `src/backend/mir/riscv/codegen/emit.hpp`: riscv
  `EdgePublicationMoveIntent`, prepared edge-publication iteration in
  `emit_prepared_module(...)`, source operand rendering, stack/memory source
  checks, register placement mapping, signed-12-bit offset decisions, and
  emitted `mv`/`li`/`lw`/`ld`/`sw`/`addi` text.
- `src/backend/mir/riscv/codegen/calls.cpp` and
  `src/backend/mir/riscv/codegen/memory.cpp`: target-owned ABI, stack, register,
  address, load/store, and helper-register policy outside the prepared wrapper
  audit surface.
- `src/backend/prealloc/publication_plans.hpp`,
  `src/backend/prealloc/publication_plans.cpp`,
  `src/backend/prealloc/prepared_lookups.cpp`, and
  `src/backend/prealloc/call_plans.cpp`: current Route 4/5/6 agreement helpers,
  prepared publication source facts, source-memory facts, call source
  publication, and status-string surfaces.
- `src/backend/bir/bir.cpp`: Route 3 memory access records and the BIR route
  fact families that can own target-neutral memory/source identity.
- `tests/backend/bir/backend_x86_handoff_boundary_direct_extern_call_test.cpp`
  and `tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp`:
  representative externally observed x86 Route 6 agreement and riscv prepared
  edge-publication wrapper behavior.

AST-backed inspection used:

- `c4c-clang-tool-ccdb function-signatures
  /workspaces/c4c/src/backend/mir/x86/prepared/dispatch.cpp
  build/compile_commands.json`
- `c4c-clang-tool-ccdb function-callees
  /workspaces/c4c/src/backend/mir/x86/prepared/dispatch.cpp
  consume_edge_publication_move_intent build/compile_commands.json`
- `c4c-clang-tool function-signatures src/backend/mir/x86/x86.hpp
  -- --std=c++17 -I/workspaces/c4c/src -I/workspaces/c4c/src/codegen/lir
  -I/workspaces/c4c/src/frontend/parser`
- `c4c-clang-tool-ccdb function-signatures
  /workspaces/c4c/src/backend/mir/riscv/codegen/emit.cpp
  build/compile_commands.json`
- `c4c-clang-tool-ccdb function-callees
  /workspaces/c4c/src/backend/mir/riscv/codegen/emit.cpp
  consume_edge_publication_move_intent build/compile_commands.json`

Target-policy split map:

| Wrapper reader | Prepared-shape semantic derivation | BIR route fact it should eventually consume | Target-owned policy that must stay local | Follow-up disposition |
| --- | --- | --- | --- | --- |
| x86 `prepared::consume_edge_publication_move_intent(...)` in `prepared/dispatch.cpp` and compare-join use in `module.cpp` | Looks up `PreparedEdgePublication` by predecessor/successor/destination id, copies `publication->source_value_id`, requires `Available`, checks prepared move authority, and renders a source operand from `PreparedValueHome`. This makes prepared edge-publication shape the source/destination identity authority for wrapper moves. | Route 5 current-block/edge join source records for source/destination edge identity, with Route 4 block-entry publication attribution where the destination fact is the relevant publication fact. Route 3 is needed only when the publication source is a load/memory-derived value. | x86 move emission, `mov` spelling, i32-only operand support, stack/register/immediate operand rendering, error/status strings, compare-join fallback, parallel-copy move authority checks, and x86 wrapper output. | Create an x86-specific wrapper split follow-up that adds a Route 5/Route 4 agreement gate or route-native adapter to the edge-publication move intent path while preserving `EdgePublicationMoveIntentStatus`, compare-join diagnostics, and wrapper text. Do not delete prepared publication lookup or value-home use in the first packet. |
| x86 direct extern call wrappers in `module.cpp` plus `find_consumed_scalar_i32_call_argument_source(...)` in `x86.hpp` | Uses prepared call argument plans and `PreparedCallArgumentPlan::source_value_id` as the agreement gate before Route 6 is allowed to pick the source value name for scalar i32 call arguments. Prepared call bundles still decide destination registers. | Route 6 call argument source records should own scalar call argument source identity after the agreement gate can be retired or inverted. | Direct extern wrapper classification, fixed/variadic wrapper kind, x86 ABI argument/result registers, stack/frame adjustment, string constant addressing, symbol rendering, emitted `mov`/`call`, zeroing `eax`, result move bundles, and fallback behavior. | Create a narrow x86 Route 6 call-wrapper follow-up that keeps ABI/call-bundle policy target-owned but moves scalar i32 source-name selection toward Route 6 authority. Preserve the current prepared-agreement tests and route-debug rows until an x86/riscv parity proof exists. |
| riscv `consume_edge_publication_move_intent(...)` and `render_edge_publication_source_operand(...)` in `codegen/emit.cpp` | Reads prepared edge publications, source/destination ids, source producer kind, source memory access status, memory base kind/name/offset/size/align/address-space/volatile flags, value-home ids, pointer-base homes, and move authority to infer the publication source and whether a memory source is legal. | Route 5 should own edge join source identity; Route 3 should own memory access/source identity for load-local/load-global/pointer-derived sources; Route 4 may own destination publication identity where the wrapper consumes block-entry publication facts. | riscv register naming, register bank/placement mapping, stack-slot alias checks, signed-12-bit offset policy, scratch register contracts (`t0`, `t6`), instruction selection and spelling (`li`, `mv`, `lw`, `ld`, `sw`, `addi`, `add`), stack destination policy, and fail-closed unsupported statuses. | Create a riscv-specific edge-publication adapter follow-up that introduces Route 5/Route 3 consumption behind prepared agreement while leaving register/stack/addressing/emission choices in riscv. It must preserve `EdgePublicationMoveIntentStatus`, existing tests in `backend_riscv_prepared_edge_publication_test.cpp`, and current fail-closed behavior for unsupported homes or offsets. |
| riscv `emit_prepared_module(...)` in `codegen/emit.cpp` | Rebuilds `PreparedFunctionLookups` per control-flow function, iterates prepared edge-publication rows, and emits wrapper moves from prepared publication availability. This makes prepared publication rows the top-level semantic iteration surface for the riscv prepared emitter. | Route 5 edge/source records should eventually drive semantic edge-source iteration, with prepared lookup retained only as target policy/status compatibility until parity is proven. | Module/function symbol emission, `.text`/`.globl` formatting, target wrapper emission order, instruction text, and unsupported publication filtering. | Fold this into the riscv edge-publication adapter follow-up rather than treating it as deletion-ready. The first riscv packet should compare Route 5 records against prepared rows and keep prepared iteration as fallback/status authority. |

Pure target policy boundaries confirmed:

- ABI and wrapper classification stay target-owned: x86 direct extern
  fixed/variadic wrapper kind, riscv call ABI config, argument/result register
  banks, stack-argument layout, variadic behavior, and helper-call resource
  choices are not BIR semantic facts.
- Layout and stack policy stay target-owned: frame size, stack slots, stack
  offsets, dynamic stack handling, local slot address materialization,
  over-aligned alloca handling, and stack alias/scratch contracts must not move
  into BIR.
- Register policy stays target-owned: prepared homes and placements may still
  be consumed by wrappers, but choosing x86 register names, riscv `a*`/`s*`/`t*`
  registers, callee-saved mapping, scratch usage, and grouped-span handling is
  target policy.
- Emission and formatting stay target-owned: `.text`, `.globl`, symbol spelling,
  private data labels, instruction selection, operand text, route-debug strings,
  handoff error strings, and wrapper output are public compatibility surfaces.
- BIR should own only portable semantic route/source facts: call argument source
  identity (Route 6), memory access/source identity (Route 3), block-entry or
  edge publication identity (Route 4/5), and producer/consumer relationships
  where the route records are proven across x86 and riscv.

## Suggested Next

Execute Step 5: audit diagnostic, oracle, and string authority. Focus on the
public strings and observed rows that would block the x86 Route 6 call-wrapper
split and the riscv Route 5/Route 3 edge-publication adapter: route-debug rows,
`EdgePublicationMoveIntentStatus`, prepared publication/source-memory status
names, helper-oracle expectations, fallback behavior, wrapper output, and
baseline tests.

## Watchouts

- Do not move ABI, layout, register, stack, emission, formatting, or wrapper
  decisions into BIR. BIR route records should replace only target-neutral
  semantic source/route identity authority.
- x86 Route 6 evidence remains narrow scalar `i32` call-argument source
  agreement. It does not justify call-plan, call-bundle, wrapper-kind,
  value-home, or result-move demotion.
- x86 edge-publication move intent still consumes prepared publication and
  value-home shape directly. Any Route 4/5 follow-up must be target-specific
  and string/fallback preserving.
- riscv has no direct Route 3/4/5/6 wrapper consumption in the inspected
  prepared edge-publication path. It currently re-derives source identity and
  memory-source legality from prepared publication shape.
- Route 3 can own memory/source identity, but riscv offset legality,
  volatile/address-space policy, pointer-base register lookup, scratch register
  use, and emitted load/store spelling remain riscv policy.
- Prepared statuses, route-debug output, handoff diagnostics, helper-oracle
  names/statuses, wrapper text, and baseline expectations are blockers for any
  contraction until Step 5 classifies their string-stability requirements.

## Proof

Analysis-only packet; no build or ctest required by delegated proof. Used
read-only inspection with `rg`, targeted source reads, and the clang-tool
queries listed above. No `test_after.log` was generated because this packet has
no compile/test proof requirement and changed only `todo.md`.
