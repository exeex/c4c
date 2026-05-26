Status: Active
Source Idea Path: ideas/open/19_x86_riscv_prepared_edge_publication_handoff.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Audit x86 and RISC-V Consumer Surfaces

# Current Packet

## Just Finished

Completed Step 2 of `plan.md`: audited the x86 and RISC-V consumer surfaces
that would read shared prepared edge/block-entry publication facts.

x86 inventory:

- `src/backend/mir/x86/x86.hpp` already has `ConsumedPlans` and
  `consume_prepared_function_lookups`, which builds `PreparedFunctionLookups`
  from prepared control-flow and exposes shared call-plan lookups. This is the
  natural place to expose `edge_publications` without inventing a second x86
  authority.
- `src/backend/mir/x86/prepared/prepared.hpp` already has
  `x86::prepared::Query::collect_block_entry_publications(successor_label)`,
  so a narrow prepared-query consumer can read block-entry publications from
  shared value-location facts.
- `src/backend/mir/x86/module/module.cpp` already validates prepared
  control-flow authority, resolves prepared block labels, reads join-transfer
  incoming edges through `incoming_transfers_for_join`, and requires
  authoritative prepared parallel-copy bundles for supported loop/join shapes.
  It is therefore structurally ready for a minimal x86 proof that looks up
  `PreparedEdgePublication` for an existing predecessor/successor edge and
  keeps x86 emission local.
- Focused x86 tests already cover reuse of shared scalar/store publication
  plans and x86 handoff boundaries, but they do not yet prove that an x86
  consumer reads the new shared `PreparedEdgePublicationLookups`.

RISC-V inventory:

- `src/backend/mir/riscv/codegen/emit.cpp` exposes only a tiny direct LIR slice
  through `emit_prepared_lir_module(const LirModule&)`; it does not accept
  `PreparedBirModule`, prepared control-flow, prepared value locations, or
  `PreparedFunctionLookups`.
- `src/backend/mir/riscv/codegen/riscv_codegen.hpp` and sibling codegen files
  define target-local register, stack-slot, operand, comparison, move, branch,
  and assembly helpers, but they are still detached from shared prepared BIR
  traversal and have no x86-like `ConsumedPlans`/`Query` surface.
- RISC-V has target-local branch and move spelling in `comparison.cpp`,
  `peephole.cpp`, and helper code, but no prepared block-entry/predecessor-edge
  hook where shared publication facts could be consumed.

Recommendation: use x86 as the first viable minimal consumer proof. The slice
should add or test an x86-side query path that consumes
`PreparedFunctionLookups::edge_publications` or
`Query::collect_block_entry_publications` for one existing prepared CFG edge,
then lets x86 decide all scratch, clobber, register-class, register-name, and
assembly spelling details. RISC-V is not ready for the same proof until it has
an exact prepared-module lowerer entry point, prepared function/block traversal,
prepared value-location lookup/decode plumbing, and a target-local
block-entry/predecessor-edge move-publication emission hook.

## Suggested Next

Proceed to Step 3 by defining the smallest target-neutral query shape expected
by x86/RISC-V consumers. The API should reuse existing shared prepared lookup
facts where possible and should not encode target move spelling.

## Watchouts

- x86 is ready for a minimal consumer proof, but the proof should read shared
  edge/block-entry publication facts rather than extend existing
  shape-specific x86 fast paths with testcase matching.
- RISC-V needs prepared BIR/module lowerer infrastructure before it can consume
  edge-publication facts in implementation code.
- Scratch selection, clobber discipline, register classes/register names, stack
  operands, and assembly syntax remain target-local for both x86 and RISC-V.
- Do not create target-specific duplicate edge-copy authority.

## Proof

No build required per packet because this was an audit-only `todo.md` update.
Read-only audit commands inspected x86 prepared/module/lowering surfaces,
RISC-V codegen entry points, shared prepared lookup declarations, and focused
publication tests. `c4c-clang-tool-ccdb list-symbols` was used for x86 module
and prepared dispatch structure; RISC-V codegen files were not present in the
compile database, so they were inspected by focused text reads. No root-level
logs were created.
