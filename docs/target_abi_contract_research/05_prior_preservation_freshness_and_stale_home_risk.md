# 05. Prior Preservation Freshness And Stale-Home Risk

Question: Where does `PriorPreservation` or equivalent preservation fallback
have enough authority, and where can it hide stale-home or missing-producer
bugs?

## Short Answer

`PriorPreservation` has enough authority today when the preserved value is a
regalloc value whose live interval crosses an earlier call, the earlier call
publishes a complete stack-slot or callee-saved-register preservation record,
and the later call can find a unique dominating prior preservation row for the
same `PreparedValueId`.

The stale-home risk is that this proves a preserved storage location is
self-consistent, not that it is the latest semantic producer of the value.
Current call planning prefers explicit source routes before prior preservation,
but preservation records do not carry an explicit freshness epoch, producer
identity, or "this producer outranks the old home" fact. A self-consistent
stack slot or callee-saved register can therefore become too authoritative when
the real missing fact is producer rematerialization or publication into the
current home.

## Freshness In The Current Contract

In the current prepared/prealloc contract, freshness means:

- the value being consumed is the same `PreparedValueId` and value name as the
  preserved or published fact;
- the storage fact is complete enough for the route being consumed, including
  stack slot, offset, size, alignment, register bank, register name, contiguous
  width, occupied register names, and prepared placement where required;
- the preservation or publication point dominates or precedes the consuming
  call in the control-flow/order model used by prepared lookups;
- no more-specific direct source route, local-frame-address materialization,
  byval-lane source, computed-address source, or producer materialization is
  available for the same use.

That definition is inferred from several code surfaces rather than stated by a
single field. `build_call_preserved_values()` in
`src/backend/prealloc/call_plans.cpp` creates `PreparedCallPreservedValue`
records only for regalloc values whose live interval crosses the call. It
records either a `StackSlot` route from an existing value home, stack object,
or assigned stack slot, or a `CalleeSavedRegister` route from an assigned
callee-saved register span.

`append_incremental_prior_preserved_values()` indexes completed call plans by
value id. `find_unique_indexed_prior_preserved_value_source()` in
`src/backend/prealloc/prepared_lookups.cpp` then searches prior entries that
agree with the current call position, dominance, and reachability. It rejects
ambiguous rows at the same position and rejects rows whose preservation source
is incomplete.

So freshness today is mostly "the latest unique complete preservation row that
is positionally valid for this value id." It is not yet "the preservation row
is known to contain the value produced by the latest semantic producer."

## When Prior Preservation Is Valid Today

Prior preservation is valid today as a fallback for values that were already
materialized into a stable home before an earlier call and remain live across
that call.

The valid stack-slot case is:

1. `build_call_preserved_values()` sees the value live across the call.
2. The value has a stack-slot home, stack object, or assigned stack slot.
3. The preservation record carries slot id, stack offset, size, alignment, and
   spill-slot placement where available.
4. A later source selection finds a unique valid prior row for the same value.
5. The target consumer verifies the selected route fields before emission.

The valid callee-saved-register case is:

1. The live value has an assigned register.
2. `is_callee_saved_register_assignment()` proves the assigned register span is
   in the target callee-saved set.
3. The preservation record carries register name, bank, contiguous width,
   occupied names, placement, and optional callee-save index.
4. The later source selection is `PriorPreservation`.
5. The target consumer requires the complete payload before using it.

AArch64 and RV64 both treat the fallback as conditional. AArch64 call lowering
uses `make_prior_preserved_call_argument_source()` in
`src/backend/mir/aarch64/codegen/calls.cpp` and emits diagnostics such as
"requires prepared PriorPreservation source selection" or "requires complete
prepared source selection" when the route is missing or incomplete. RV64 call
emission uses `emit_riscv_prior_preserved_gpr_argument()`,
`gpr_register_number_for_prior_preserved_selection()`, and
`stack_slot_offset_for_prior_preserved_gpr_selection()` in
`src/backend/mir/riscv/codegen/prepared_call_emit.cpp`; those helpers reject
non-prior selections, incomplete callee-saved GPR payloads, mismatched stack
slot fields, wrong sizes, and invalid frame offsets.

The current fallback is therefore valid for "previously preserved, still-live,
complete, unique, target-verifiable storage." It is not a general proof that a
value's old home is the best source.

## When Rematerialization Should Outrank Prior Preservation

Producer rematerialization should outrank prior preservation when the current
use can be satisfied from an explicit producer fact that is closer to the
semantic value than the old preserved home.

The current ordering mostly reflects that rule. In
`select_prepared_call_argument_source()` in
`src/backend/prealloc/call_plans.cpp`, prior-preservation lookup happens only
after these routes have been attempted:

- byval register-lane source selection;
- frame-slot address selection for memory-return or local-frame-address
  materialization;
- frame-slot value selection when the source home is a stack slot;
- local aggregate address materialization from a register home;
- local aggregate address materialization from a
  `PointerBasePlusOffset` computed-address home.

Separate producer materialization also exists for scalar call arguments.
`find_prepared_call_argument_source_producer_materialization()` accepts
`LoadLocal` producers and selected binary producers. The binary helper
`prepared_call_argument_binary_producer_opcode_is_materializable()` currently
accepts `Add`, `Sub`, `And`, `Or`, `Xor`, `Mul`, `SDiv`, and `SRem`, and
rejects unsigned division/remainder, shifts, and comparisons for this path.
AArch64's `materialize_scalar_call_argument_value()` then consults Route 6 or
prepared source-producer facts before recursively materializing a supported
binary producer.

Those producer routes should outrank preservation when they identify the
current same-block producer, prove the instruction precedes the call, match the
requested value, and pass the target verifier. In that situation,
`PriorPreservation` is at best an old storage copy. The producer path is the
fresh authority because it ties the use to the actual instruction that created
the value.

## Stale-Home And Missing-Producer Risk Evidence

The strongest code evidence is the shape of the preservation payload itself.
`PreparedCallPreservedValue` carries value id/name, stack or register storage,
placements, and preservation endpoints. It does not carry the producer block,
producer instruction index, producer kind, source-memory identity, or a
freshness generation. `find_unique_indexed_prior_preserved_value_source()` can
choose the latest valid prior row by call position and dominance, but it cannot
prove that no newer producer or publication should have overwritten the old
home.

`seed_prior_call_preservation_from_later_dependency()` adds another narrow
risk. It can seed earlier call plans from a later callee-saved dependency when
`preservation_can_seed_prior_call()` proves a one-register callee-saved route
and a prior argument sources the same preserved register. That is useful for
complete callee-saved register chains, but it is still storage-equivalence
logic. It does not add producer freshness facts.

Closed idea evidence points in the same direction:

- `ideas/closed/265_phase_f4_memory_accesses_unsupported_stale_fail_closed_proof_map.md`
  records that stale-publication rows remain blocked because no synthetic
  stale-row matrix proves old prepared facts fail closed for producer block,
  instruction index, source value, base kind, wrong edge, duplicate, and
  obsolete owner across x86 and riscv consumers. That is direct evidence that
  prepared rows can be internally coherent while stale relative to semantic
  producer authority.
- `ideas/closed/184_phase_e_route1_producer_constant_view_consumer_migration.md`
  closed only one selected publication-source consumer migration and explicitly
  preserved prepared fallback/oracle visibility for incomplete or out-of-scope
  answers. It names no-producer, missing-producer, future-producer, recursive
  operand, and unrelated-register cases as guardrails, which shows why
  fallback cannot become semantic authority by itself.
- `ideas/closed/394_rv64_same_module_sret_callee_home_publication.md`
  describes a concrete stale-home shape: RV64 loaded a stack-homed
  `%ret.sret` pointer slot that had not been initialized from incoming `a0`.
  The fix had to publish the incoming ABI sret pointer into the callee home
  expected by prepared facts, not merely avoid the bad load.
- `ideas/closed/583_rv64_pointer_arithmetic_result_publication.md`
  says the missing tail was RV64 materialization plus publication of a
  pointer-valued add/sub result into the prepared destination home expected by
  later memory operations. That is a missing-producer/publication risk: later
  consumers needed the result home to be populated by the actual pointer
  arithmetic producer.
- `ideas/closed/406_rv64_object_route_residual_local_memory_boundaries.md`
  directs newly discovered missing producer facts into separate producer work
  instead of compensating in RV64 object emission. That preserves the boundary
  that target consumers should not paper over absent producer authority by
  trusting whatever home happens to exist.

Together, the evidence says the current contract is good at failing closed for
many incomplete routes, but incomplete at proving freshness when an old home,
a later producer, and a possible publication route all describe the same value.

## Minimum Facts Needed For Explicit Preservation Authority

To make preservation authority explicit, a preserved source should carry at
least these facts, either directly or through a linked authority record:

| Needed fact | Why it is needed |
| --- | --- |
| Preserved value identity | The existing `PreparedValueId` and value name remain necessary to bind the route to the consumed value. |
| Preservation point | The preserving call block/instruction and route are needed to order the record against later consumers. |
| Storage endpoint | Stack slot, offset, size, alignment, register bank/name/span, and placement are needed for target emission. |
| Freshness source | The producer block, instruction index, producer kind, or explicit publication fact that made the preserved home current. |
| Freshness relation | A machine-checkable relation saying the preserved source is fresh for this use: for example, latest dominating publication, same-block producer before call, or no newer producer between preservation and use. |
| Rematerialization priority | A flag or ordering rule that says a verified producer materialization outranks preservation for the same value/use. |
| Invalidation or supersession | A way to mark older preservation rows obsolete when a newer producer, publication, or home update becomes authoritative. |
| Target completeness | The existing per-target completeness checks for stack and callee-saved-register payloads must remain fail-closed. |

The smallest useful addition is not a broader backend fallback. It is a shared
prepared/prealloc authority relation that links preservation records to the
latest valid producer or publication fact and makes the current fallback order
explicit.

## Answer

`PriorPreservation` has enough authority when it is a unique, complete,
dominance-valid preservation of a still-live value into a target-verifiable
stack slot or callee-saved register. It can hide stale-home or missing-producer
bugs when that storage proof is treated as proof of latest semantic value
freshness.

Producer rematerialization should outrank prior preservation whenever a
verified `LoadLocal`, supported binary producer, local-frame-address
materialization, computed-address source, or other explicit publication proves
the current value more directly than the old home. The missing contract fact is
an explicit freshness authority: preserved storage needs to say which producer
or publication made it current, how that fact dominates the consuming use, and
whether any newer producer supersedes it.
