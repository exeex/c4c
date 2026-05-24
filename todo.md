Status: Active
Source Idea Path: ideas/open/prealloc-call-boundary-ordering-republication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Map Call-Boundary Ordering Surfaces

# Current Packet

## Just Finished

Completed Step 1: `Map Call-Boundary Ordering Surfaces`.

Used `c4c-clang-tools` first to inspect the prealloc call-plan and lookup
surfaces plus the AArch64 call lowering files. The current boundary is:

- Generic ordering facts already visible in prealloc:
  `PreparedCallPlan` identifies call position, wrapper kind, direct/indirect
  callee facts, memory-return facts, arguments, result, preserved values, and
  clobbered registers. `PreparedMoveBundle` lookup keys identify before-call
  and after-call bundles by phase/block/instruction, and
  `classify_prepared_call_boundary_move` already relates a move to a call
  argument/result plan plus the matching ABI binding.
- Generic republication intent facts already visible in prealloc:
  `PreparedCallPreservedValue` records value id/name, preservation route,
  callee-save index, stack-slot facts, register facts, and placement snapshots.
  `PreparedCallPlanLookups` records prior preserved values and first stack
  preserved values per call, including same-block and dominating-call lookup
  helpers.
- Generic source/destination facts already visible in prealloc:
  call arguments and results include source/destination storage kind, value id,
  register name/placement/bank, stack offsets, frame-slot ids, immediates,
  symbol addresses, pointer base-plus-offset, destination ABI index, and
  contiguous/occupied-register metadata.
- Keep-local AArch64 ABI/emission facts:
  register parsing/conversion and view selection, scratch-register choice,
  immediate operand spelling, memory operand construction, outgoing stack-base
  instruction construction, byval copy mnemonics/chunking/scratch policy,
  indirect-callee register conversion, variadic helper validation, dynamic
  stack helper lowering, final `CallBoundaryMoveInstructionRecord` and
  `CallInstructionRecord` construction, and printed machine instruction text.
- Mixed facts requiring hooks:
  before-call ordering currently depends on emitted AArch64 register aliasing
  (`move_source_aliases_destination`); missing frame-slot call-argument
  materialization depends on AArch64 register conversion and publication
  emission; callee-saved home population/republication depends on generic
  preservation intent but target register/view conversion and source/dest
  operand legality; byval register-lane handling depends on generic lane
  coverage/source offsets plus AArch64 chunking and instruction policy.

Smallest viable first implementation slice: add a neutral prealloc
call-boundary effect/order record for the existing before-call and after-call
move bundles, built from `PreparedCallPlan`, `PreparedMoveBundle`, and
`PreparedCallBoundaryMoveClassification`. The first slice should expose ordered
records for explicit prepared moves and synthetic callee-saved
population/republication intent, carrying only phase, call position, move
classification, source/destination storage/value/ABI identity, preservation
route, and reason. It should not construct registers, memory operands,
instructions, byval chunk sequences, or ABI-specific register views.

## Suggested Next

Execute Step 2 by adding the neutral call-boundary effect/order record and a
builder that produces records for before-call and after-call explicit bundle
moves plus callee-saved preservation-home population/republication intent,
leaving AArch64 emission unchanged.

## Watchouts

Do not move `order_before_call_moves_for_source_preservation` as-is: it orders
already-materialized machine instructions via AArch64 register aliasing. The
generic record can preserve an effect ordering key and intent, but target hooks
still need to answer register alias/materialized-address questions. Also keep
byval aggregate copy details and indirect-callee operand conversion in AArch64
until a separate hook boundary is explicit.

## Proof

No validation run; audit-only `todo.md` update. No `test_after.log` created.
