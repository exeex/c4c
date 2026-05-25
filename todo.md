Status: Active
Source Idea Path: ideas/open/08_calls_argument_sources_retirement.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Move Or Redirect Remaining Source Choice To Prepared Facts

# Current Packet

## Just Finished

Step 2 strengthened the earlier grouping-only slice by physically separating
the target-only operand adapters into
`src/backend/mir/aarch64/codegen/calls_operand_adapters.cpp` and adding that
translation unit to `src/backend/CMakeLists.txt`.

The moved target-only register/view/immediate/F128 adapter surface covers:

- `complete_full_width_f128_carrier`,
  `complete_f128_constant_carrier`, and
  `find_prepared_f128_carrier_in_module`.
- `scalar_fp_view_from_register_name`, `scalar_view_from_register_name`,
  `scalar_size_from_register_view`, `register_name_with_expected_view`,
  `scalar_integer_register_view`,
  `scalar_integer_register_view_from_size`, and
  `scalar_integer_type_from_size`.
- `make_register_operand_from_prepared_authority`,
  `make_f128_q_register_operand_from_carrier`, and
  `make_scalar_call_argument_immediate`.

No semantic source-choice logic was moved or tightened. The moved definitions
are byte-for-byte behavior-preserving target operand construction, while the
remaining source-choice helpers `make_frame_slot_call_argument_source`,
`make_frame_slot_call_argument_address_source`, and
`make_local_frame_address_call_argument_source` remain in the frame-slot/local
address cluster for later prepared-fact retirement.

`find_frame_slot_by_id`, frame-slot/local-address source helpers,
stack/aggregate memory helpers, indirect callee, memory return, and
prior-preserved source construction remain in `calls_argument_sources.cpp`.

## Suggested Next

Continue Step 2 with a focused prepared-fact retirement packet for one
documented absent-selection source-choice path, or add a narrow prepared
record/fact needed before such retirement can happen.

## Watchouts

- Keep semantic source-choice authority in prepared call facts, not AArch64
  helper reshuffles.
- Do not edit `ideas/closed/`, `review/`, or non-canonical test logs as part of
  this execution route.
- Treat expectation weakening, duplicate prepared/AArch64 source selection, and
  helper-only moves as route failures.
- Do not fold `find_frame_slot_by_id` into call-specific code; it is a shared
  lookup used by byval aggregate and cast lowering.
- Do not retire `make_frame_slot_call_argument_address_source`,
  `make_frame_slot_call_argument_source`, or
  `make_local_frame_address_call_argument_source` until the missing prepared
  facts for absent-selection local aggregate/address paths are available.
- The target-only adapter TU now exists; future packets should avoid moving
  semantic source-choice scans into it.

## Proof

Passed; proof log preserved at `test_after.log`.

Command:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_instruction_dispatch|backend_aarch64_call_boundary_owner|backend_aarch64_prepared_memory_operand_records|backend_prepare_frame_stack_call_contract|c_testsuite_aarch64_backend_src_00204_c|c_testsuite_aarch64_backend_src_00216_c)$'
```

Result: build succeeded and all 6 selected tests passed.

`git diff --check` also passed.

Supervisor follow-up validation:

- `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`
  passed 162/162.
- Backend regression guard passed with `test_before.log` and `test_after.log`:
  162 passed before, 162 passed after, no new failures.
