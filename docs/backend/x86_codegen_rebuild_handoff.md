# X86 Codegen Rebuild Handoff

## Stage-2 To Stage-3 Intake Contract

Stage 3 must consume the stage-1 extraction set under
`docs/backend/x86_codegen_legacy/` as reviewed evidence, not as a file-for-file
template. The approved replacement layout and mandatory draft manifest live in
`docs/backend/x86_codegen_rebuild_plan.md` and are the authoritative contract
for the next stage.

## What Stage 3 May Trust As-Is

Stage 3 may trust these stage-1 judgments without reopening the live source
tree first:

- the declared legacy-source boundary for this rebuild is the top-level
  `src/backend/mir/x86/codegen/*.cpp` set plus `x86_codegen.hpp`
- the prepared family is currently acting like a parallel lowering stack rather
  than a thin client of canonical seams
- `calls.cpp`, `returns.cpp`, `prologue.cpp`, `memory.cpp`, `mod.cpp`, and
  `shared_call_support.cpp` represent the clearest canonical ownership seams in
  the current subsystem
- `route_debug.cpp` is a proof/debug consumer, not a lowering owner
- `x86_codegen.hpp` is a primary rebuild pressure point because it mixes
  unrelated contracts in one header

## What Stage 3 Must Not Inherit Blindly

Stage 3 must not treat the following as a ready-made replacement architecture:

- the exact file boundaries in `prepared_module_emit.cpp`,
  `prepared_local_slot_render.cpp`, `prepared_param_zero_render.cpp`, and
  `prepared_countdown_render.cpp`
- the broad shared-context style currently exposed through `x86_codegen.hpp`
- any design that keeps module-data emission fused with prepared-route
  dispatch
- any design that allows prepared fast paths to own their own copies of frame,
  address, call-lane, or comparison semantics

## Required Route Constraints

Every stage-3 draft artifact must preserve these constraints:

- prepared routes may be optional fast paths only when they remain thin
  adapters over canonical lowering seams
- canonical ownership for frame, call, return, memory, and comparison policy
  must remain outside prepared-specialized files
- route-debug surfaces may observe and summarize route facts, but they must not
  become hidden lowering utilities
- compatibility seams must be named explicitly instead of being hidden in
  generic helper headers
- the replacement layout must keep the `peephole/` subtree out of claimed
  ownership unless a later lifecycle change explicitly expands the boundary

## Failure Pressure The Drafts Must Address

The draft set must make it structurally hard to recreate the idea-75 failure
family. The motivating bug is a downstream consumer problem: truthful
prepared-home and addressing facts already exist upstream, but a prepared/x86
consumer still turns them into overlapping pre-call writes. Any replacement
draft that leaves prepared routes free to reimplement call-lane ordering,
frame-home lookup, or operand formation locally has missed the point of this
stage.

## Mandatory Draft Outputs

Stage 3 must create every artifact listed in the replacement manifest in
`docs/backend/x86_codegen_rebuild_plan.md`, including:

- the directory/index artifacts under `docs/backend/x86_codegen_rebuild/`
- one `.hpp.md` for every planned replacement header
- one `.cpp.md` for every planned replacement implementation file
- `docs/backend/x86_codegen_rebuild/review.md`

Missing, merged, renamed, or silently dropped files are a stage-2 contract
violation unless stage 2 is repaired first.

## Drafting Guidance

When stage 3 writes each replacement draft, it should state:

- owned inputs
- owned outputs
- allowed indirect queries
- forbidden knowledge
- whether the file is core lowering, dispatch, optional fast path, debug, or
  compatibility

That per-file discipline is mandatory because the whole purpose of the rebuild
is to replace hidden cross-file knowledge with legible ownership direction.

## Completion Gate For Stage 3

Stage 3 should consider itself ready for review only when the draft set:

- matches the exact manifest from stage 2
- keeps prepared routes as consumers of shared seams instead of parallel
  lowering owners
- makes the dependency graph legible at the file level
- records a coherent review in `docs/backend/x86_codegen_rebuild/review.md`
