# AArch64 Memory Prepared Authority Repair Runbook

Status: Active
Source Idea: ideas/open/50_aarch64_memory_prepared_authority_repair.md

## Purpose

Repair duplicate memory-authority recovery in AArch64 memory lowering so
`memory.cpp` consumes shared prepared memory, value-home, move, store-source,
address-materialization, and aggregate stack facts instead of reconstructing
semantic identity locally.

## Goal

Make `src/backend/mir/aarch64/codegen/memory.cpp` rely on prepared authority
for memory records, return retargeting, local-address stores, pointer-base
materialization, and store publication state, adding narrow shared lookups only
where the existing prepared facts cannot answer the consumer query.

## Core Rule

Memory lowering may choose AArch64 instruction spelling, addressing forms,
scratch registers, and emission order, but semantic authority must come from
prepared facts rather than value-name scans, raw move-bundle scans,
stack-object spelling, `@`-prefixed global matching, or local pending
publication tables.

## Read First

- `ideas/open/50_aarch64_memory_prepared_authority_repair.md`
- `src/backend/mir/aarch64/codegen/memory.cpp`
- Prepared memory, value-home, addressing, move-bundle, store-source
  publication, and aggregate stack source definitions and lookup helpers.
- Recent closed memory/store-source authority ideas when a shared prepared
  contract is unclear, especially ideas 34, 39, and 39a.

## Current Targets

- `src/backend/mir/aarch64/codegen/memory.cpp`
- Shared prepared lookup surfaces only when an existing prepared contract cannot
  represent the memory consumer's semantic query.

## Non-Goals

- Do not change load/store opcode choice, offset folding, pointer-register
  materialization, duplicate-emission avoidance, source reload sequencing, or
  stack/global store instruction spelling except to consume a shared semantic
  contract.
- Do not fold ALU, publication, dispatch, comparison, calls, or value
  materialization repair into this plan.
- Do not treat target-local source reloads, scratch choices, address-register
  allocation, or instruction formatting as duplicate semantic authority by
  themselves.
- Do not weaken supported-path expectations or downgrade failing tests to
  unsupported without explicit user approval.

## Working Model

- `memory.cpp` is the AArch64 memory lowering owner and should remain the place
  that converts prepared memory facts into AArch64 memory instructions.
- Prepared memory and value-home records should provide identity and storage
  meaning before lowering reaches `memory.cpp`.
- If the current prepared lookup is missing a key needed by memory lowering,
  add the smallest shared query that exposes the already-prepared fact.
- Delete or narrow obsolete local recovery once the prepared fact is consumed;
  do not keep the old authority under a renamed helper.

## Execution Rules

- Keep each code-changing step behavior-preserving apart from authority-source
  repair.
- Use `todo.md` to record audit findings and proof results; do not rewrite the
  source idea for routine discoveries.
- Prefer one bounded repair family per executor packet.
- Reject testcase-shaped matching, named-case shortcuts, expectation rewrites,
  or helper-only renames as progress.
- For each implementation step, run at least a fresh build or compile proof and
  the supervisor-selected narrow memory/backend tests.
- Escalate to broader backend validation before closure because memory lowering
  affects stack, global, pointer, return, and aggregate-store routes.

## Step 1: Audit Memory Recovery Sites

Goal: map each duplicate authority path in `memory.cpp` to an existing or
missing prepared fact before making implementation edits.

Primary target:

- `src/backend/mir/aarch64/codegen/memory.cpp`

Actions:

- Inspect value-home id recovery through `find_value_home_id` and
  `find_unique_value_home_id`.
- Inspect prepared memory record consumers such as
  `make_memory_record_from_prepared_access`,
  `make_prepared_*_memory_instruction_record`, `apply_load_identity`, and
  `apply_store_identity` to confirm they remain prepared consumers.
- Inspect before-return retargeting through
  `find_memory_return_abi_register` and
  `retarget_load_result_to_return_abi`.
- Inspect local-address store, pointer-base-plus-offset, store-source, and
  store-global publication paths named in the source idea.
- Record in `todo.md` the exact replacement authority or missing lookup needed
  for each family.

Completion check:

- The duplicate recovery sites are grouped into bounded repair packets with
  named prepared facts or explicit missing shared queries.

## Step 2: Replace Value-Home Id Recovery Scans

Goal: route memory value-id recovery through prepared indexed value-home lookup
helpers.

Primary target:

- `src/backend/mir/aarch64/codegen/memory.cpp`

Actions:

- Replace manual scans over `value_locations.value_homes` with
  `PreparedValueHomeLookups::value_ids`, `find_prepared_value_id`,
  `find_indexed_prepared_value_id`, or `PreparedFunctionLookups::value_homes`
  as appropriate.
- Keep memory record construction as a consumer of `PreparedMemoryAccess`,
  `PreparedValueHome`, `PreparedAddressingFunction`, and
  `PreparedStoragePlanFunction`.
- Remove obsolete value-name recovery helpers once call sites consume prepared
  lookup authority.

Completion check:

- Value-home id recovery no longer depends on manual value-home scans or raw
  value spelling, and narrow proof covers the affected memory load/store route.

## Step 3: Repair Return Retargeting Authority

Goal: make before-return load retargeting consume prepared move or return
authority.

Primary target:

- `src/backend/mir/aarch64/codegen/memory.cpp`

Actions:

- Replace raw `move_bundles` scans in return retargeting with an existing
  prepared move lookup if one can answer source value id plus destination bank.
- If no lookup exists, add a narrow shared before-return move query and route
  `memory.cpp` through it.
- Keep ABI register spelling and final load retarget emission behavior
  unchanged.

Completion check:

- Before-return memory load retargeting is keyed by prepared source/destination
  authority rather than raw move-bundle scans, with focused return-path proof.

## Step 4: Repair Local Address And Pointer-Base Authority

Goal: remove value spelling, stack-object name matching, and global-symbol
spelling from local-address store and pointer-base materialization paths.

Primary target:

- `src/backend/mir/aarch64/codegen/memory.cpp`

Actions:

- Route local-address store value recovery through prepared stack-object,
  frame-slot, value-home, or aggregate stack facts.
- Route pointer-base-plus-offset and global-symbol materialization through a
  prepared pointer-base field or query for
  `PreparedValueHomeKind::PointerBasePlusOffset`.
- Add the smallest shared lookup only when the prepared contract has the fact
  but no consumer query.
- Preserve AArch64 address materialization and store instruction spelling.

Completion check:

- Local-address store and pointer-base paths no longer depend on value names,
  stack-object names, or `@`-prefixed global text as durable authority.

## Step 5: Repair Store-Source And Store-Global Publication State

Goal: make store-local, pointer-store writeback, recovered narrow store source,
and store-global publication state consume prepared publication planning.

Primary target:

- `src/backend/mir/aarch64/codegen/memory.cpp`

Actions:

- Route `plan_store_local_source_publication`,
  `prepared_store_source_producer`, and
  `prepared_recovered_narrow_store_source` through
  `PreparedStoreSourcePublicationPlan`,
  `PreparedStoreSourcePublicationInputs`, and related prepared source-producer
  lookups.
- Use prepared store-source helpers for byval formal pointer and direct-global
  select-chain dependency cases.
- Replace local pending or duplicate store-global publication tracking with a
  prepared plan or narrow shared query carrying explicit pending and
  duplicate-stack-homes-only state.
- Keep source reload sequencing and emitted store instructions unchanged except
  where consuming the prepared fact requires a local call-site adjustment.

Completion check:

- Store-local, pointer-store writeback, recovered narrow store source, and
  store-global publication behavior is represented by prepared publication
  authority rather than AArch64-local pending state.

## Step 6: Final Validation And Closure Check

Goal: prove the source idea's acceptance criteria are satisfied without
testcase overfit or expectation downgrades.

Actions:

- Run the supervisor-selected narrow memory/backend proof for each repaired
  family.
- Run broader backend validation sufficient to catch memory lowering
  regressions across stack, global, pointer, return, and aggregate-store paths.
- Review the diff for new value-home scans, raw BIR memory operand recovery,
  raw move-bundle scans, text-name matching, ad hoc store-global scans, or
  unsupported-test rewrites.

Completion check:

- All source acceptance criteria are satisfied, regression proof is current,
  and any remaining work is outside this source idea's scope.
