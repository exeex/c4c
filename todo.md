Status: Active
Source Idea Path: ideas/open/225_phase_e2_fused_compare_operand_producer_helper_private_pass_context.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Extract The Private Agreement Reader

# Current Packet

## Just Finished

Step 1 - Inventory Public Consumers is complete.

Direct public prepared helper family:

- `src/backend/prealloc/comparison.hpp`: declares
  `PreparedFusedCompareOperandProducer`,
  `PreparedFusedCompareOperandProducerFacts`,
  `find_prepared_fused_compare_operand_producer(...)`, and
  `find_prepared_fused_compare_operand_producer_facts(...)`. Retain this public
  prepared surface for fallback, helper-oracle, and tests.
- `src/backend/prealloc/comparison.cpp`: implements the public prepared
  per-operand helper and facts helper. It reads immediate operands, prepared
  same-block scalar producers, prepared integer constants, and fails closed for
  non-fused or missing-lhs/rhs conditions. Retain as the public prepared
  fallback surface; Step 2 should add the private agreement reader around this
  family without removing fallback behavior.

Production comparison-lowering consumer:

- `src/backend/mir/aarch64/codegen/comparison.cpp` local
  `find_prepared_fused_compare_operand_producer_facts(context,
  branch_condition)` is the selected migration candidate. It is the only direct
  production caller of the public prepared facts helper and currently performs
  prepared lookup, optional `PreparedFunctionLookups` reuse, Route 7 index
  lookup, conversion to prepared facts, agreement checking, and prepared
  fallback.
- `lower_prepared_conditional_branch_terminator(...)` consumes that local
  wrapper before `make_prepared_conditional_branch_record_impl(...)`. Retain
  AArch64 ownership of target branch policy, fused legality, suffix/condition
  selection, hazards, emitted-register state, final instruction order, and
  assembler rows.
- `make_prepared_conditional_branch_record_impl(...)` consumes optional operand
  producers to form compare operand records and immediate-folding records. This
  stays target branch policy, not a Route 7/private-boundary migration target.

Immediate helper-family support code:

- `route7_comparison_operand_producer_to_prepared(...)`,
  `prepared_fused_compare_operand_producer_matches(...)`, and
  `route7_fused_compare_operand_producer_facts_if_agree_with_prepared(...)` in
  `src/backend/mir/aarch64/codegen/comparison.cpp` are the current agreement
  bridge. Step 2 should move only the selected Route 7/prepared agreement
  identity read behind a private pass-context boundary and keep non-agreement
  falling back to the public prepared facts.
- `src/backend/bir/bir.hpp` / `src/backend/bir/bir.cpp`
  `FusedCompareOperandProducerFacts`,
  `find_fused_compare_operand_producer_facts(...)`, and
  `route7_find_fused_compare_operand_producer_facts(...)` are BIR/Route 7
  aggregate lookup surfaces. Retain these public surfaces for tests, oracle
  comparison, route-index validation, and unrelated branch-support paths.
- `src/backend/prealloc/select_chain_lookups.*` and
  `PreparedFunctionLookups::edge_publication_source_producers` are supporting
  prepared lookup sources for the public helper. Retain as prepared aggregate
  lookup infrastructure.

Retained target branch policy and wrapper surfaces:

- `find_prepared_fused_compare_branch_facts(...)`,
  `apply_prepared_conditional_branch_targets(...)`,
  `append_prepared_compare_branch_lines(...)`,
  `install_fused_compare_print_operands(...)`, `make_branch_instruction(...)`,
  and the fused-compare support lowering functions in
  `src/backend/mir/aarch64/codegen/comparison.cpp` stay AArch64 target policy
  and printer/debug support.
- `fused_compare_uses_selected_operand(...)` reads Route 7/BIR producer facts
  directly for dispatch support selection. It is a target branch-support
  policy consumer, not the selected public prepared-helper migration target.
- `src/backend/mir/aarch64/codegen/dispatch.cpp` calls the fused-compare
  support helpers and remains a wrapper/dispatch surface.
- x86 `PreparedBranchConditionKind::FusedCompare` consumers and prepared branch
  target helper calls are wrapper/target policy surfaces outside this helper
  contraction. No x86 migration candidate was found for this prepared facts
  helper.

Printer/debug and expected-string surfaces:

- `src/backend/mir/aarch64/codegen/machine_printer.cpp`,
  `src/backend/mir/aarch64/codegen/instruction.cpp`, and
  AArch64 branch-control/machine-printer tests keep fused-compare printer and
  debug output authority. Retain byte-stable output and expected strings.
- `tests/backend/mir/backend_aarch64_branch_control_lowering_test.cpp` covers
  selected fused branch lowering, non-fusable fail-closed behavior, and nearby
  materialized-condition Route 7 fallback behavior. Retain as production/output
  proof, not a helper API migration target.
- `tests/backend/mir/backend_aarch64_machine_printer_test.cpp` covers
  fused-compare immediate-left, both-immediate, and non-encodable immediate
  printer behavior. Retain expected strings and printer behavior.

Helper-oracle and test-only coverage:

- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp` direct calls at
  the prepared helper oracle verify lhs/rhs facts and non-fused fail-closed
  behavior.
- The same test file compares prepared, BIR, and Route 7 fused producer facts
  for select/folded constants, immediates, rhs-only availability, unavailable
  paths, wrong-key/wrong-role validation, unavailable route indexes, missing
  producers, duplicate references, stale owner blocks, and production fixture
  parity. Retain these test-only/helper-oracle surfaces.
- Test helper
  `prepared_and_bir_comparison_operand_producer_match(...)` is test-only oracle
  support. Retain.

Step 2 candidate versus retained surfaces:

- Candidate for private pass-context migration: the selected AArch64
  comparison-lowering identity read currently inside the local
  `find_prepared_fused_compare_operand_producer_facts(context,
  branch_condition)` wrapper after public prepared fallback lookup.
- Retained public prepared surfaces: exported prepared structs, the public
  per-operand and facts helpers, prepared lookup aggregate infrastructure,
  helper-oracle tests, BIR/Route 7 aggregate lookup APIs, AArch64 branch target
  policy, printer/debug rows, wrappers/dispatch, x86 target policy, and all
  expected strings.

## Suggested Next

Execute Step 2 from `plan.md`: extract a private agreement reader for only the
selected Route 7/prepared-agreement fused compare operand producer identity
read. Keep the public prepared helper as fallback for absent route data,
invalid references, duplicates/conflicts, mismatch, unfused cases, and
non-agreement.

## Watchouts

- Do not edit implementation files, tests, docs, or source ideas during Step 1.
- Keep branch target policy, suffix mapping, fused legality, hazards,
  emitted-register state, final assembler order, printer/debug rows, wrappers,
  helper-oracle strings, and expected strings out of route authority.
- Treat unsupported downgrades, baseline refreshes, helper renames, wrapper
  moves, timeout masking, and expectation rewrites as non-progress.
- Preserve prepared fallback for absent-route, invalid-reference,
  duplicate/conflict, mismatch, unfused, and non-agreement paths.
- Do not migrate `fused_compare_uses_selected_operand(...)` or BIR/Route 7
  aggregate lookup APIs in Step 2; they are retained branch-support and
  aggregate/oracle surfaces.
- Do not change helper-oracle names, statuses, failure-mode strings, machine
  printer rows, branch-control expected strings, or x86 wrapper behavior.

## Proof

Docs/inventory-only packet; no build or ctest execution was required and no
`test_after.log` was created.

Discovery command run:

```sh
ctest --test-dir build -N -R 'backend_prepared_lookup_helper|backend_aarch64_branch_control_lowering|backend_aarch64_machine_printer'
```

`ctest -N` showed exactly these three tests for the regex:

- `backend_aarch64_machine_printer`
- `backend_aarch64_branch_control_lowering`
- `backend_prepared_lookup_helper`

Supervisor-selected implementation proof route to use for later code-changing
packets:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_prepared_lookup_helper|backend_aarch64_branch_control_lowering|backend_aarch64_machine_printer'
```
