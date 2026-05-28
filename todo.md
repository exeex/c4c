Status: Active
Source Idea Path: ideas/open/63_aarch64_shared_select_chain_dependency_authority.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Inventory Select-Chain Dependency Callers

# Current Packet

## Just Finished

Completed Step 1 inventory for idea 63. No implementation files were edited.

Inventory:
- `dispatch_edge_copies.cpp::emit_select_chain_value_to_register` is the shared
  AArch64 target emitter for select-chain branches, labels, compare emission,
  scratch use, and recursive value materialization. It still rediscovers the
  select producer through `find_prepared_same_block_select_producer`; when a
  `PreparedDirectGlobalSelectChainDependency*` is supplied, it only validates
  root identity and direct-global presence.
- `dispatch_edge_copies.cpp::materialize_direct_global_select_chain_call_argument`
  is reached only from `calls.cpp::lower_scalar_call_argument_producers`. It
  uses `PreparedCallArgumentPlan` for call-argument index/source value/home
  constraints, then separately calls
  `prepare::find_prepared_direct_global_select_chain_dependency` before
  delegating target emission to `emit_select_chain_value_to_register` or FP
  value emission.
- `dispatch_producers.cpp::prepared_select_chain_contains_direct_global_load`
  is currently AArch64-local prepared traversal over
  `PreparedEdgePublicationSourceProducerKind`. Its public wrapper
  `select_chain_contains_direct_global_load` falls back to raw
  `mir::select_chain_contains_dependency`; no current AArch64 caller outside
  this file was found.
- Non-edge ALU consumer:
  `alu.cpp` uses `prepare::find_prepared_scalar_select_chain_materialization`
  before calling `emit_select_chain_value_to_register`; this already packages
  root value name plus direct-global dependency, but the target emitter still
  performs local select-producer lookup for recursive emission.
- Non-edge value-materialization consumer:
  `dispatch_value_materialization.cpp::emit_value_publication_to_register`
  calls `emit_select_chain_value_to_register` for select producers without a
  direct-global dependency fact. It needs a prepared materialization fact or a
  deliberate non-direct-global/materialization-only contract before local
  select traversal can be removed.
- Indirect-call callee consumer:
  `calls.cpp::materialize_indirect_call_callee_to_prepared_register` uses
  `PreparedCallPlan::indirect_callee` for ABI destination and
  `find_prepared_indirect_callee_direct_global_select_chain` for stored-value
  direct-global chains, but `emit_indirect_callee_value_to_register_with_csel`
  still recursively follows prepared source producers to emit select/csel
  materialization.
- Store-source publication is a related existing prepared owner:
  `memory.cpp::plan_store_local_source_publication` uses
  `prepare::find_prepared_store_source_direct_global_select_chain_dependency`
  and stores direct-global select-chain booleans/root index in
  `PreparedStoreSourcePublicationPlan`; this covers publication decisions, not
  general call-argument or value-materialization emission.
- Other `make_select_chain_materialization_instruction` callers in calls,
  casts, globals, memory, FP value materialization, and dispatch publication
  are target-instruction wrappers for already chosen lines; they do not appear
  to rediscover select-chain dependency identity.

Existing shared/prepared fact owners:
- `src/backend/prealloc/publication_plans.cpp` already owns
  `find_prepared_direct_global_select_chain_dependency`,
  `find_prepared_store_source_direct_global_select_chain_dependency`, and
  `find_prepared_scalar_select_chain_materialization`, backed by prepared edge
  publication source producer lookups.
- `PreparedCallPlan` and `PreparedCallArgumentPlan` already own call presence,
  argument index, source value id, homes, destination ABI registers, indirect
  callee destination, and preservation facts, but they do not currently carry a
  direct-global/select-chain dependency fact for arguments or indirect callees.

Fact gaps before removing local traversal:
- Add/extend a shared prepared select-chain materialization fact usable by
  generic value publication, not only scalar ALU direct-global cases.
- Add a call-argument prepared fact, likely on or reachable from
  `PreparedCallArgumentPlan`, that answers whether an argument source is a
  direct-global select chain and provides root instruction/root-is-select/root
  value identity.
- Add an indirect-callee prepared fact, likely on `PreparedCallPlan` or
  `PreparedIndirectCalleePlan`, for stored-value select-chain dependency and
  materialization source identity.
- Replace AArch64-local `dispatch_producers.cpp` direct-global traversal and
  its raw BIR fallback only after the shared prepared owner can represent the
  same yes/no/unknown behavior fail-closed.
- Keep AArch64 ownership of labels, register spelling, scratch selection,
  `cmp`/branch or `csel` sequencing, and assembler instruction packaging.

## Suggested Next

Supervisor should delegate Step 2 to define the shared/prepared dependency
facts, starting with a call-argument query/fact that lets
`materialize_direct_global_select_chain_call_argument` stop asking
`publication_plans` directly.

## Watchouts

`PreparedScalarSelectChainMaterialization` already covers the ALU direct-global
case, so avoid inventing a second parallel scalar fact. The biggest uncovered
paths are generic value publication, call arguments, and indirect callees. The
raw `mir::select_chain_contains_dependency` fallback in `dispatch_producers.cpp`
is a route-drift hazard and should not be cloned.

## Proof

Read-only inspection plus `todo.md` update only. Commands run:
`rg` for the named helpers and select-chain/prepared facts; targeted `sed`/`nl`
reads of `dispatch_edge_copies.cpp`, `dispatch_producers.cpp`, `calls.cpp`,
`alu.cpp`, `dispatch_value_materialization.cpp`, `memory.cpp`, and
`src/backend/prealloc/{publication_plans,calls}.{hpp,cpp}`. No build or test
proof required by the packet; `git diff --check` run after the update.
