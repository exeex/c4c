Status: Active
Source Idea Path: ideas/open/514_rv64_register_source_stack_destination_move_bundles.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Define The Minimal Accepted Or Rejected Contract

# Current Packet

## Just Finished

Step 2 defined the minimal RV64 object-emission contract for prepared
register-source to stack-destination move bundles without implementation edits.

Implementation boundary:

- The current accepted RV64 materializer is in
  `fragment_for_prepared_move_bundle(...)` in
  `src/backend/mir/riscv/codegen/object_emission.cpp`.
- It already accepts a stack-destination value move only when the producer
  explicitly classifies it as `reason=consumer_register_to_stack`; the
  `consumer_stack_to_stack` arm delegates to the same-scalar stack-slot helper
  from idea 513 and must not be widened to cover register sources.
- `classify_prepared_object_move_bundle_consumer(...)` only proves the
  traversal event has an available, phase-matching bundle. It does not prove
  source/destination home kinds, scalar width compatibility, or extension
  semantics, so producer reason strings remain part of the authority contract.

Minimal accepted predicate for one explicit register-source to
stack-destination value move:

- Bundle predicate: `phase=before_instruction`, `authority=none`,
  `parallel_copy=no`, and exactly one move for the minimal single-move path.
- Move predicate: `op_kind=move`, `uses_cycle_temp_source=no`,
  `destination_kind=value`, `destination_storage=stack_slot`,
  `destination_contiguous_width=1`, no destination register placement, no
  occupied destination register set, no explicit destination stack offset, no
  immediate source, and no parallel-copy source metadata.
- Producer authority predicate: `reason=consumer_register_to_stack`. A
  register source with `reason=consumer_stack_to_stack` is producer
  misclassification, not an RV64 stack-to-stack materialization opportunity.
- Source predicate: `from_value_id` resolves through prepared value homes to a
  register home whose register name is an RV64 GPR accepted by
  `gpr_register_number_for_home(...)`; storage-plan evidence, when present,
  must agree with register/GPR/contiguous-width-1 authority.
- Destination predicate: `to_value_id` resolves through prepared value homes
  to a stack-slot home whose absolute frame offset is admitted by
  `prepared_stack_slot_home_absolute_offset(...)` for the destination scalar
  size.
- Width and semantics predicate: the destination type must have an RV64 scalar
  memory size accepted by `rv64_scalar_memory_size_for_type(...)` and the store
  helper must accept that size through `rv64_load_store_funct3_for_size(...)`
  (`1`, `2`, `4`, or `8` bytes). A plain prepared move has no extension or
  truncation semantics; if source and destination scalar sizes differ, the
  producer must publish an explicit converted source/result or the bundle must
  stay rejected/reclassified.

Adjacent fail-closed shapes:

- Missing source authority: no source home, non-register source home,
  non-RV64-GPR register identity, unknown register spelling, source immediate,
  cycle-temp source, or parallel-copy source metadata.
- Missing destination authority: no destination home, non-stack destination
  home, explicit destination stack offset instead of a value home, missing or
  out-of-range frame-slot facts, destination register placement, or occupied
  destination registers.
- Unsupported register bank: FPR or any non-GPR source register bank is not a
  GPR store-register-to-stack move. FPR-to-stack would need its own explicit
  producer reason and `append_rv64_store_fpr_to_stack...` contract.
- Unsupported width or extension semantics: unsupported scalar type, unsupported
  store size, multi-register width, or source/destination scalar-size mismatch
  without an explicit producer-owned conversion result.
- Multi-move ambiguity: more than one move, especially multiple register
  sources targeting the same stack destination, is not part of the minimal
  single-move contract. RV64 must not pick by order, drop one source, merge
  moves, or infer sequencing without a producer-owned contract.
- Producer misclassification: register-source to stack-destination moves labeled
  `consumer_stack_to_stack`, or extension-producing moves represented as plain
  copies, should be reclassified or diagnosed at the producer/consumer-contract
  boundary before RV64 materialization.

Representative separation:

- `src/pr27073.c` is a single-move register-source to stack-destination shape,
  but it is not currently accepted by the minimal predicate: the producer labels
  it `consumer_stack_to_stack`, and the semantic value is `i16` source to `i32`
  destination with no explicit extension authority in the move. Step 3 should
  reclassify this as producer-owned missing conversion/reason authority unless
  new source facts prove the extension result is already the register source.
- `src/20010518-1.c` is a separate multi-move case: two explicit GPR sources
  (`a0`, `a1`) target the same stack destination in one non-parallel bundle.
  Even though each individual value is `i32` GPR-to-stack-shaped, the bundle
  cardinality is ambiguous and remains rejected until Step 5 or a separate
  producer contract defines the intended sequencing/selection.

## Suggested Next

Proceed to Step 3 by making the single-move boundary precise in code:
materialize only the accepted `consumer_register_to_stack` predicate above, and
otherwise reclassify/diagnose the `pr27073.c` shape as missing producer
authority for the `consumer_stack_to_stack` reason plus `i16` to `i32`
extension semantics.

## Watchouts

Do not make the `consumer_stack_to_stack` helper accept register sources; that
would blur idea 513's same-scalar stack-slot contract. Keep `20010518-1.c`
separate from `pr27073.c`: the former is a multi-move ownership problem, while
the latter is a single-move producer-reason/extension-authority problem.

## Proof

Contract-definition packet; no code-changing proof was required and no
`test_after.log` was requested. Source/probe inspection covered
`object_emission.cpp` move-bundle materialization, store helpers,
`prepared_object_traversal.cpp` consumer classification, and focused RV64
object-emission tests. Ran `git diff --check -- todo.md`.
