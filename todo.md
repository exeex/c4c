Status: Active
Source Idea Path: ideas/open/256_aarch64_comparison_markdown_shard_implementation_redistribution.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Comparison Ownership

# Current Packet

## Just Finished

Step 1 inventory completed for comparison-family ownership.

Safe first-move candidates:

- `make_prepared_unconditional_branch_record` and
  `make_prepared_conditional_branch_record` currently live in
  `instruction.cpp`/`instruction.hpp`, but they build only branch-control
  records from prepared branch facts and are already consumed by
  `comparison.cpp` terminator lowering. They are the narrowest
  comparison-family relocation target.
- The small `branch_record_error` helper can move with the prepared branch
  record builders if needed to keep the moved code self-contained.
- `i128_compare_record_error`, `i128_compare_signedness_from_predicate`,
  `i128_compare_high_word_semantics_from_predicate`, and
  `make_prepared_i128_compare_record` are comparison-family candidates, but
  they are embedded in the broader i128 pair/shift/helper cluster and should
  not be the first move.
- `machine_printer.cpp` owns comparison-specific spelling in
  `print_i128_compare` and branch materialized-bool spelling in
  `print_branch`; those are later printer-ownership candidates, not Step 2's
  first packet.
- `dispatch.cpp` is currently routing glue for prepared branch terminators via
  `lower_prepared_branch_terminator`,
  `lower_prepared_conditional_branch_terminator`, and successor helpers.

Broad or neutral helpers that should remain outside comparison ownership for
the first move:

- enum/string-name functions such as `branch_condition_form_name`,
  `branch_compare_candidate_kind_name`, `prepared_branch_record_error_name`,
  `i128_compare_signedness_name`, and
  `i128_compare_high_word_semantics_name`
- broad instruction constructors and validators including
  `make_branch_instruction`, `make_i128_compare_instruction`,
  `branch_instruction_selection_status`, and `i128_compare_selection_status`
- shared operand and value-home helpers including `find_named_value_home`,
  `same_bir_value`, `make_compare_value_record`,
  `make_prepared_branch_target`, and prepared scalar/register conversion
  helpers
- i128 pair/shared helpers such as `make_i128_pair_operand_record`,
  `is_compare_predicate`, and the non-compare i128 shift/runtime-helper
  record builders
- generic dispatch mechanics in `dispatch.cpp` and generic assembly emission
  plumbing in `machine_printer.cpp`

## Suggested Next

Narrow first implementation packet: move only the prepared branch record
builders from `instruction.cpp`/`instruction.hpp` into
`comparison.cpp`/`comparison.hpp`, preserving declarations, diagnostics,
selection status, and branch-control behavior. Keep all broad instruction
constructors, name helpers, operand helpers, dispatch routing, printer code,
tests, and `comparison.md` unchanged.

## Watchouts

- Preserve comparison behavior while changing ownership.
- Do not downgrade tests, add testcase-shaped shortcuts, or expand comparison
  semantics beyond behavior-preserving relocation.
- Do not delete `src/backend/mir/aarch64/codegen/comparison.md` until durable
  content has been reconciled into compiled owners or proven already covered.
- The first move should not drag i128 compare or printer spelling with it; that
  would widen the packet across the i128 pair and printer surfaces.
- If the prepared branch builders require a local helper move, keep it limited
  to branch-record error construction; do not move broad enum-name or operand
  helper APIs.

## Proof

docs/inventory-only packet; no build or tests run.

Supervisor-selected proof command for the first code-moving packet:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_aarch64_(branch_compare_contract|branch_compare_records|compare_branch_candidate_records|prepared_branch_records|machine_printer)_test'
```
