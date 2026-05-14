# AArch64 Module Stage 2 To Stage 3 Handoff

## Purpose

This handoff is the intake contract for the Stage 3 markdown draft stage of the
AArch64 module phoenix route. Stage 3 can start from the accepted Stage 2 layout
without re-deriving the legacy architecture from implementation files.

Stage 3 remains a draft stage. It must write markdown replacement drafts only
unless the active Stage 3 plan later authorizes real `.cpp`, `.hpp`, build, or
test edits.

## Trustworthy Evidence

Stage 3 may trust these Stage 1 extraction artifacts as evidence:

- `src/backend/mir/aarch64/module/module.cpp.md`
- `src/backend/mir/aarch64/module/module.hpp.md`
- `src/backend/mir/aarch64/module/module.md`

The accepted Stage 2 layout artifact is:

- `src/backend/mir/aarch64/module/stage2_review_layout.md`

It is the architectural source for the Stage 3 draft map, replacement seams,
parent-224 judgment, and drift rejection rules.

## Evidence Repair Status

Stage 2 found no required Stage 1 extraction repair before Stage 3 can draft
from the trustworthy evidence above. The required correction is already route
state, not a blocking repair: `src/backend/mir/aarch64/module/module.cpp`
remains compiled legacy evidence until replacement implementation and build
wiring exist.

## Mandatory Stage 3 Draft Files

Stage 3 must produce exactly this initial replacement draft set from
`stage2_review_layout.md`.

Directory index draft:

- `src/backend/mir/aarch64/module/module.md`

Mandatory header draft:

- `src/backend/mir/aarch64/module/module.hpp.md`

Mandatory implementation drafts:

- `src/backend/mir/aarch64/module/module.cpp.md`
- `src/backend/mir/aarch64/module/function_traversal.cpp.md`
- `src/backend/mir/aarch64/module/operand_resolution.cpp.md`
- `src/backend/mir/aarch64/module/instruction_lowering.cpp.md`
- `src/backend/mir/aarch64/module/branch_control_lowering.cpp.md`
- `src/backend/mir/aarch64/module/call_lowering.cpp.md`
- `src/backend/mir/aarch64/module/public_assembly_bridge.cpp.md`
- `src/backend/mir/aarch64/module/compatibility_projection.cpp.md`

`module.hpp.md` is the only mandatory header draft. `helper.hpp` is the only
allowed header exception, and Stage 3 must explicitly justify it before
drafting one.

## Route Constraints

The lowering input is `prepare::PreparedBirModule`. It drives module dispatch,
function traversal, operand resolution, instruction lowering, branch/control
lowering, call lowering, public assembly bridging, and compatibility
projection.

The MIR carrier is hierarchical by function, block, and instruction. Stage 3
must not design the replacement around a flat `vector<MachineNode>` or preserve
`FunctionRecord::machine_nodes` as the primary machine product. The replacement
product should be organized around `MachineModule`, `MachineFunction`,
`MachineBlock`, and `MachineInstruction` or equivalent typed MIR concepts that
preserve the same hierarchy.

Origin and provenance should be lightweight metadata: stable IDs, indices,
labels, source locations when already durable, and reason enums/strings that
explain which prepared authority produced a fact. Stage 3 must not make broad
prepared object ownership, raw prepared/source object retention, or
`std::string_view` lifetime coupling the replacement authority model.

The shared `mir_printer` owns one scan over the MIR carrier and the common
emission mechanics. AArch64 owns instruction, operand, register, memory, label,
symbol, and immediate print/render methods. Instruction rendering and operand
rendering must remain separate target-owned surfaces.

Target-owned printable surfaces must use ordinary C++ API names. Stage 3 must
not introduce a C++ method or API named `__repr__`.

Do not delete, disable, or build-disconnect
`src/backend/mir/aarch64/module/module.cpp` in Stage 3. That can only happen
after replacement implementation and wiring exist in a later authorized plan.

## Non-Goals

- Do not edit real `.cpp` or `.hpp` implementation files.
- Do not edit build wiring or tests.
- Do not change expectations or downgrade supported paths.
- Do not draft extra component files beyond the mandatory map unless the
  active Stage 3 plan expands scope.
- Do not use broad public inspection records as semantic lowering authority.
- Do not preserve the legacy catch-all module emitter under renamed helper
  files.

## Rejection Signals

Reject Stage 3 drafts that:

- make `FunctionRecord::machine_nodes` or another flat target-local vector the
  canonical output
- treat cached display strings as instruction or operand authority
- route printer traversal or assembly structure through AArch64-specific code
  instead of the shared `mir_printer`
- make compatibility fields choose among broad optional records to satisfy a
  narrow case
- expand returned `add`/`sub` handling by named testcase instead of drafting
  semantic instruction lowering families
- recover identity or registers primarily from spellings when structured
  prepared facts are available
- introduce raw prepared/source object ownership as the replacement provenance
  model
- name target render APIs `__repr__`
- delete or disconnect the compiled legacy `module.cpp` before replacement
  wiring exists

## Sequencing Dependencies

1. Start Stage 3 from `stage2_review_layout.md`, especially its Stage 3 draft
   artifact map and responsibility seams.
2. Draft the directory index and `module.hpp.md` contract first so every
   implementation draft shares the same MIR carrier, provenance, printer, and
   compatibility vocabulary.
3. Draft implementation files by dependency direction: module dispatch,
   function traversal, operand resolution, instruction lowering,
   branch/control lowering, call lowering, public assembly bridge, then
   compatibility projection.
4. Keep compatibility projections derived from canonical MIR and typed facts
   after lowering, not as a second lowering path.
