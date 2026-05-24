Status: Active
Source Idea Path: ideas/open/subsystem-entropy-reduction-refactor-generator.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Prioritize Hotspots

# Current Packet

## Just Finished

Step 3 ranked the `src/backend/mir/aarch64/codegen/` entropy hotspots from the
Step 2 map by safety, reuse value, and durable ownership clarity. Each accepted
candidate below has exactly one source-idea refactor type and an expected
behavior-preservation proof.

Prioritized hotspot list:
- Rank 1: `calls_dispatch_bridge.cpp` / `calls_dispatch_bridge.hpp`.
  Refactor type: `Helper absorption`. Durable owner: call-boundary adapter
  logic that translates dispatch facts into call-lowering inputs. Why first:
  the files are route-specific glue, have clear bridge ownership, and can be
  tightened without changing call ABI semantics or the core dispatch contract.
  Suggested follow-up shape: absorb or relocate thin bridge helpers into the
  durable call/dispatch adapter owner while preserving the public entry points
  needed by dispatch. Proof expectation: build plus focused AArch64 MIR/codegen
  tests that exercise normal calls, select-chain call arguments, call-result
  source registers, preserved-value materialization, and local-load fallback
  call arguments.
- Rank 2: `compatibility_projection.cpp` /
  `compatibility_projection.hpp`, with the call site in `module_compile.cpp`
  only as proof context. Refactor type: `Bridge retirement`. Durable owner:
  compatibility bridge from selected target records to legacy projection
  records. Why second: the boundary is intentionally narrow, the helper names
  already describe a compatibility route, and retirement or contraction can be
  proved by showing the newer selected-record contract still owns the behavior.
  Suggested follow-up shape: remove or reduce compatibility projection glue
  only where selected target records fully cover the projected records; defer
  if any behavior still depends on the legacy projection surface. Proof
  expectation: build plus focused module/AArch64 compatibility tests covering
  selected target records, unsupported-node reporting, and object/global record
  projection.
- Rank 3: `alu.cpp` fallback/control-publication helpers behind `alu.hpp`.
  Refactor type: `Phase extraction`. Durable owner: scalar phase logic for
  prepared-home lookup, fallback operand selection, and control publication
  materialization. Why third: the extraction has reuse value, but `alu.hpp` is
  widely included and the implementation mixes real scalar lowering with
  fallback and publication policy. Suggested follow-up shape: split only one
  helper group, preferably fallback operand selection or control-publication
  materialization, into a small phase-local implementation unit without
  widening header exposure. Proof expectation: build plus scalar ALU, control
  publication, fallback operand, and same-block producer tests.
- Rank 4: printer/emitter boundary around `machine_printer.cpp`,
  `asm_emitter.cpp`, and family print helpers. Refactor type:
  `Printer alignment`. Durable owner: debug/print logic consuming structured
  machine records. Why fourth: printer-only changes are usually lower semantic
  risk, but the surface is broad enough that a follow-up must pick one family
  printer alignment rather than the whole print pipeline. Suggested follow-up
  shape: align one family printer with the data family it prints and avoid
  changing record construction. Proof expectation: build plus machine-printer
  and assembly text output tests for the selected family.
- Rank 5: cross-cutting `AssemblerInstructionRecord` /
  `inline_asm_template` construction in phase files such as `dispatch.cpp`,
  `alu.cpp`, `calls_moves.cpp`, `comparison_branch_fusion.cpp`,
  `memory_store_sources.cpp`, `memory_dynamic_stack.cpp`,
  `prologue_entry_formals.cpp`, and `cast_ops.cpp`. Refactor type:
  `Phase extraction`. Durable owner: target-local emission adapter between
  phase decisions and assembler records. Why fifth: the reuse value is high,
  but this is cross-cutting and should only become a follow-up if scoped to one
  family or one helper group. Proof expectation: build plus targeted tests for
  the selected family's inline-asm record emission and unchanged text output.

## Suggested Next

Step 4 should create follow-up idea files in this order:

1. A bounded `calls_dispatch_bridge.*` helper-absorption idea.
2. A `compatibility_projection.*` bridge-retirement idea, explicitly blocked
   unless selected target records fully own the projected behavior.
3. Optionally one scalar `alu.cpp` phase-extraction idea if Step 4 wants a
   third candidate, scoped to one helper group only.

Each generated idea should include target files, the single refactor type,
durable owner category, proof command expectations, and reject signals for
semantic call ABI changes, expectation downgrades, testcase-shaped shortcuts,
or file-count reductions that increase responsibility mixing.

## Watchouts

Too-broad exclusions for Step 4:
- Exclude `dispatch.cpp` as a first implementation target. It is the broadest
  phase hub and mixes producer lookup, publication, materialization,
  retargeting, family dispatch, and branch-fusion hooks. A first slice there
  would likely require ownership redesign rather than behavior-preserving
  cleanup.
- Exclude `instruction.hpp` / `instruction.cpp` as a first target. They are
  high-entropy but also the durable machine-record contract; splitting them
  safely requires prior evidence from smaller adapter cleanups.
- Exclude broad `calls_moves.cpp` refactors. The file spans before-call,
  after-call, before-return, value moves, stack copies, byval lanes, immediate
  bindings, and callee-saved republication. A follow-up may target one helper
  group later, but not the whole file.
- Defer whole-pipeline printer/emitter cleanup unless Step 4 narrows it to one
  family printer or one emission adapter.
- Defer cross-family inline-asm record construction cleanup unless it is scoped
  to one family or one helper group with unchanged assembly output.

Reject any follow-up candidate that needs semantic behavior changes, call ABI
changes, selected-record contract changes, expectation downgrades,
unsupported-test conversions, broad renames without durable concept proof, or
target-specific instruction/register logic moved into generic layers.

## Proof

`git diff --check` passed. No build, test run, or `test_after.log` was needed
for this planning-only packet.
