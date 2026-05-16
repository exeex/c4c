Status: Active
Source Idea Path: ideas/open/256_aarch64_comparison_markdown_shard_implementation_redistribution.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Route Dispatch Through The Comparison Owner

# Current Packet

## Just Finished

Step 3 routed the remaining comparison-family dispatch body through the
comparison owner.

`dispatch.cpp` already routed unconditional branch, conditional branch,
compare-branch, and materialized-bool branch behavior through `comparison.cpp`.
The remaining embedded comparison-family lowering was the I128 compare arm
inside `lower_i128_pair_operation_instruction`; that arm now delegates to
`lower_prepared_i128_compare_instruction` in `comparison.cpp`, with the public
declaration in `comparison.hpp`.

## Suggested Next

Supervisor can review the Step 3 routing slice for acceptance or delegate the
next plan-step packet.

## Watchouts

- No tests, expectations, printer code, `comparison.md`, `plan.md`, source
  idea, or `test_before.log` were changed.
- The generic I128 shift/runtime/helper paths remain in `dispatch.cpp`; only
  the I128 compare case moved because it is the comparison-family body in this
  packet.

## Proof

Supervisor-selected proof command was run exactly and wrote `test_after.log`:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_aarch64_(branch_compare_contract|branch_compare_records|compare_branch_candidate_records|prepared_branch_records|machine_printer)$' > test_after.log 2>&1
```

Build completed and CTest passed 5/5 tests:

- `backend_aarch64_branch_compare_contract`
- `backend_aarch64_branch_compare_records`
- `backend_aarch64_compare_branch_candidate_records`
- `backend_aarch64_prepared_branch_records`
- `backend_aarch64_machine_printer`

Supervisor also ran the broader AArch64 backend regression guard before and
after this slice:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_aarch64_'
```

`check_monotonic_regression.py --allow-non-decreasing-passed` accepted the
before/after comparison with before=27/27 PASS and after=27/27 PASS.
