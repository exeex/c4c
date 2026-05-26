Status: Active
Source Idea Path: ideas/open/20_aarch64_codegen_layout_classification.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Build The Durable Classification Table

# Current Packet

## Just Finished

Completed the evidence-resolution packet for `Step 4: Build The Durable
Classification Table` by auditing callers and source-selection usage for the
two entries previously marked `needs-more-evidence`.

Exact caller/usage evidence for `compatibility_projection.*`:

- `compatibility_projection.cpp` defines only
  `derive_compatibility_function_records` and `derive_compatibility_projection`.
  The first maps lowered `module::MachineFunction` values into legacy
  `module::FunctionRecord` rows by copying the function id, deriving the label
  from prepared names, copying the machine function, and extracting
  `selected_machine_nodes`; the second wraps those rows in
  `module::CompatibilityProjection`.
- `module_compile.cpp` is the only implementation caller found by `rg`: it
  initializes an empty compatibility projection in `make_module_shell`, then
  immediately after `lower_prepared_functions` assigns
  `built_module.functions = derive_compatibility_function_records(...)` and
  `built_module.compatibility = derive_compatibility_projection(...)`.
- `module/module.hpp` documents `FunctionRecord::machine_nodes` as a
  compatibility-only flat projection; the authoritative stream is `mir`, and
  terminal assembly must not print from that vector.
- Classification recommendation: `fold-back`. Proposed owner is
  `module_compile.cpp` or a module-private compatibility section. This is
  target-local legacy projection glue at the module build boundary, not
  shared prepare authority and not a durable standalone owner.

Exact caller/usage evidence for `memory_store_sources.*`:

- Dispatch callers are confined to AArch64 codegen. `dispatch.cpp` calls
  `lower_pointer_base_plus_offset_store_local_publication`,
  `lower_store_local_value_publication`, and
  `lower_stack_homed_pointer_store_writeback` while lowering local stores with
  address materialization; the main dispatch loop calls
  `lower_stack_homed_pointer_value_load_publication`,
  `lower_pending_store_global_stack_value_publications`,
  `lower_store_local_value_publication`,
  `lower_store_global_value_publication`,
  `lower_pointer_base_plus_offset_store_local_publication`, and
  `lower_stack_homed_pointer_store_writeback` around ordinary memory lowering.
- The only non-dispatch implementation caller found is
  `dispatch_value_materialization.cpp`, which reuses
  `find_latest_narrow_store_for_wide_local_load` before emitting a recovered
  narrow stored value into a register for wide local-load materialization.
- The source-selection predicates are not mere AArch64 instruction spelling.
  `find_latest_narrow_store_for_wide_local_load` scans earlier same-block BIR
  stores, compares logical slot/frame-object/lane-offset names, and returns a
  recovered source value plus producer instruction index. The byval, cast,
  select, and scalar-FP-binary predicates inspect same-block BIR producers and
  prepared memory accesses before `lower_store_local_value_publication`
  decides whether to synthesize an extra source-publication instruction.
- Shared prepare already has a partial carrier:
  `PreparedStoreSourcePublicationPlan` records source value/name/home,
  destination access/frame slot/stack object, recovered source value and
  recovered source instruction index, pointer writeback metadata, and
  publication intent. However, the AArch64 file currently constructs that plan
  only after doing same-block producer discovery and then consumes the plan for
  AArch64 scratch-register, frame-slot, symbol-address, pointer-base-plus-offset,
  load/store mnemonic, and inline-assembler emission decisions.
- Classification recommendation: `move-forward` for the semantic
  source-publication decision layer, with a scoped mechanical follow-up to
  `fold-back` the remaining AArch64 emission helpers into `memory.cpp` after
  that move. The follow-up should move target-neutral producer/source facts for
  narrow-store recovery, byval frame-slot loads, cast/select/scalar-FP producer
  publication, direct global-load select chains, and pending store-global stack
  publications into shared prepare or BIR/MIR publication planning. It should
  leave target register selection, scratch policy, symbol-address materializing,
  frame-slot address spelling, and concrete AArch64 load/store emission local.

## Suggested Next

Continue `Step 4` by building the durable classification table from the prior
inventory/classification packets, using the resolved recommendations above:
`compatibility_projection.*` is `fold-back`; `memory_store_sources.*` is
`move-forward` for semantic source-publication planning plus later memory-owner
fold-back for target-local emission.

## Watchouts

Do not classify all of `memory_store_sources.*` as a pure `fold-back` just
because its callers are AArch64-local. The caller audit proves local use, but
the predicate audit shows target-neutral same-block producer and publication
decision facts that should become prepared facts before the AArch64 emission
file is mechanically consolidated.

Do not classify `compatibility_projection.*` as `move-forward`: the audited
helpers only create legacy flat views from already-lowered machine functions.

## Proof

Not run. This was an audit/classification-only packet, and the delegated proof
explicitly said not to run build/tests and not to modify `test_after.log`
because this packet only resolves classification evidence and the plan forbids
root proof-log changes.
