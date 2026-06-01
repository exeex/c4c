Status: Active
Source Idea Path: ideas/open/70_aarch64_memory_prepared_address_authority_cleanup.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Consume Prepared Stack-Source Authorities

# Current Packet

## Just Finished

Completed the opening mapping packet for `plan.md` Step 4 in
`src/backend/mir/aarch64/codegen/memory.cpp` without changing implementation
files. Used `c4c-clang-tools` first to inspect the relevant signatures and call
graph for stack-source publication helpers.

Classification:

- Aggregate stack-source authority exists as
  `prepare::PreparedAggregateStackSourceAuthority`, populated on
  `prepare::PreparedEdgePublication::aggregate_stack_source_authority`, with
  `prepare::prepare_aggregate_stack_source_authority(...)` as the prepared
  authority constructor. `memory.cpp` currently has no caller of that prepared
  authority helper, so any aggregate stack-source implementation packet must
  consume this fact rather than recreating aggregate copy/source validity from
  local MIR shape.
- Typed stack-source publication exists as
  `prepare::PreparedTypedStackSourcePublication`, with
  `prepare::prepare_same_width_i32_stack_source_publication(...)` as the
  prepared fact constructor. `memory.cpp` currently has no caller of that helper;
  `make_load_result_stack_publication_scratch(...)` and its
  `make_load_memory_instruction_record(...)` caller still classify the stack
  result path from local home/storage/type checks before AArch64 scratch
  selection.
- Store-local publication already routes source authority through prepared
  facts: `plan_store_local_source_publication(...)` gathers prepared memory
  access, source/destination homes, producer lookups, recovered narrow source,
  byval source status, and direct-global select-chain dependency, then calls
  `prepare::plan_prepared_store_source_publication(...)`.
- Store-global publication already routes source authority through prepared
  facts: `lower_store_global_value_publication(...)` calls
  `prepare::plan_prepared_store_global_publication(...)`, and
  `lower_pending_store_global_stack_value_publications(...)` calls
  `prepare::plan_pending_prepared_store_global_publications(...)`.
- AArch64-local policy remains local: scratch-register choice, emitted-register
  conflict checks, scalar/fp materialization calls, load/store mnemonic choice,
  base-plus-offset legality checks, stack address spelling, and final
  assembler-record construction.

## Suggested Next

First executable Step 4 implementation packet: consume
`prepare::PreparedTypedStackSourcePublication` in the load-result stack-source
path by routing same-width i32 stack-source classification through
`prepare::prepare_same_width_i32_stack_source_publication(...)` before
`make_load_result_stack_publication_scratch(...)` selects the AArch64 scratch
register. Keep aggregate transport planning out of the packet and leave
AArch64 opcode/register/address spelling policy in `memory.cpp`.

## Watchouts

Do not treat the existing store-local/store-global publication plans as missing
authority; they already consume prepared source facts and mainly need to keep
AArch64 emission policy local. The missing consumption boundary found in this
mapping is the unused aggregate/typed stack-source prepared facts, not the
prepared store-source plan itself. If the next packet cannot identify the
relevant edge publication to pass into the typed helper, stop and record that
lookup boundary instead of inventing a target-local substitute.

## Proof

Passed. Logless mapping-only proof:

Command:

```sh
git diff --check -- todo.md
```
