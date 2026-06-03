Status: Active
Source Idea Path: ideas/open/108_prepared_select_chain_dump_contract_coverage.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Design The Dump Contract

# Current Packet

## Just Finished

Step 2 dump contract design completed for prepared select-chain and
direct-global dependency visibility.

Chosen dump surfaces:
- Print carried call-argument direct-global dependency facts on the existing
  `prepared-call-plans` argument line in `prepared_printer/calls.cpp`.
- Add a narrow prepared-printer helper section for scalar select-chain
  materialization facts only if Step 3 can compute them from existing prepared
  module state without changing lookup authority. Suggested section label:
  `--- prepared-select-chain-materializations ---`.
- Defer store-source/source-producer dump surface for this runbook unless a
  source field is already carried by a printed plan. `PreparedStoreSourcePublicationPlan`
  is a transient planning struct in MIR memory lowering today, not a
  prepared-module section. Creating a full store-source publication dump would
  broaden the route beyond prepared-printer visibility for already-carried
  facts.

Stable call-argument labels:
- Emit only when `find_prepared_call_argument_direct_global_select_chain_dependency`
  returns a non-null dependency.
- Use `direct_global_select_chain=yes`.
- Use `direct_global_source=<value-name>` for
  `PreparedCallArgumentDirectGlobalSelectChainDependency::source_value_name`.
- Use `direct_global_root_is_select=yes|no` for nested
  `direct_global_dependency.root_is_select`.
- Use `direct_global_root_inst=<index>` for nested
  `direct_global_dependency.root_instruction_index`.
- `contains_direct_global_load` is represented by the presence of
  `direct_global_select_chain=yes`; do not print an unavailable/no variant on
  every argument.

Stable scalar select-chain labels:
- Section line shape:
  `select_chain function=<function> block=<block> value=<root-value> root_is_select=yes|no root_inst=<index>`.
- If nested direct-global dependency is available, append
  `direct_global_select_chain=yes direct_global_root_is_select=yes|no direct_global_root_inst=<index>`.
- The section should include only materializations where
  `PreparedScalarSelectChainMaterialization::available` is true. Do not print
  negative/unavailable rows.
- The intended implementation target is a small prepared-printer helper that
  walks BIR blocks using existing prepared lookup state and calls
  `find_prepared_scalar_select_chain_materialization`; it must not create new
  semantic lookup authority.

Source-producer provenance policy:
- Print source-producer kind/block/index only in the narrow scalar
  materialization section if the fields explain the same selected value root.
  Proposed labels: `source_producer=<kind>`,
  `source_producer_block=<block>`, and `source_producer_inst=<index>`.
- Do not add source-producer fields to unrelated edge/publication dumps in this
  route.
- Do not print generic indirect-callee direct-global dependency in Step 3
  unless a carried prepared-call-plan field is added narrowly. Current
  `PreparedCallPlan` does not store that dependency; the consumer recomputes it
  from lookup state.

Focused test assertions:
- Extend or add a prepared-printer fixture with a same-block direct-global
  load feeding a select chain passed as a call argument. Assert the call
  argument line contains `direct_global_select_chain=yes`,
  `direct_global_source=<selected-value>`,
  `direct_global_root_is_select=yes`, and `direct_global_root_inst=<index>`.
- Assert the scalar select-chain section prints `value=<selected-value>`,
  `root_is_select=yes`, `root_inst=<index>`, and direct-global nested fields
  for the same fixture.
- Assert source-producer provenance appears only on the scalar select-chain
  row and uses stable source-producer kind/block/index labels.
- Avoid store-source dump assertions unless Step 3 intentionally introduces a
  bounded store-source section with carried facts.

Implementation targets:
- `src/backend/prealloc/prepared_printer/calls.cpp` for call-argument
  dependency labels.
- A new narrow prepared-printer file or a tightly scoped helper in an existing
  file for `prepared-select-chain-materializations`; wire it through
  `prepared_printer/private.hpp` and `prepared_printer.cpp` only if needed.
- `tests/backend/bir/backend_prepared_printer_test.cpp` for focused dump text
  assertions.

Rejected routes:
- No broad dump of all edge publication/source-producer facts.
- No lookup API rewrite or target lowering policy change.
- No parser, MIR, x86/RISC-V/AArch64 codegen behavior change.
- No negative rows for every value/argument.
- No store-source publication dump unless Step 3 finds an already-carried
  prepared-module surface that can be printed without broadening ownership.

## Suggested Next

Execute `plan.md` Step 3: implement the chosen prepared-printer visibility for
call-argument direct-global dependency and, if feasible from existing prepared
lookup state, the narrow scalar select-chain materialization section.

## Watchouts

- If Step 3 cannot compute scalar select-chain rows from existing prepared
  module state without new carried facts, stop and report that missing carried
  fact instead of adding broad storage to prepared plans.
- The chosen labels intentionally omit unavailable/no rows; absence means no
  prepared dependency/materialization contract is available.
- Store-source/direct-global visibility remains deferred because the relevant
  plan is transient in MIR memory lowering and not a prepared-module dump
  surface today.

## Proof

Passed. Ran:
`git diff --quiet -- src/backend/prealloc tests/backend && printf 'analysis-only proof: no implementation or backend test diff for prepared select-chain dump contract design\n' > test_after.log`

Proof log: `test_after.log`.
