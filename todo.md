# x86_64 Backend Bring-Up After BIR Cutover

Status: Active
Source Idea: ideas/open/44_x86_64_backend_bringup_after_bir_cutover.md
Source Plan: plan.md

## Current Active Item

- Step 2 low-risk `memory.cpp` matcher cleanup for the active cleanup-first runbook
- current exact slice:
  replace the adjacent compact global-struct compare families that still rely
  on rendered LLVM text:
  `try_lower_minimal_global_anonymous_struct_field_compare_zero_return_module`,
  `try_lower_minimal_global_named_two_field_struct_designated_init_compare_zero_return_module`,
  and
  `try_lower_minimal_global_named_int_pointer_struct_designated_init_compare_zero_return_module`
  with structural instruction matching, plus a narrow lowering test that proves
  at least one route no longer depends on exact SSA temp naming

## Next Slice

- continue the `memory.cpp` cleanup queue with the remaining exact-text compare
  ladders now concentrated around
  `try_lower_minimal_local_string_literal_char_compare_ladder_zero_return_module`
  and the adjacent global/nested aggregate compare families that still depend
  on `print_llvm()`
- leave the simple `try_lower_minimal_single_global_i32_zero_return_module`
  seam parked until after the larger remaining compare families stop depending
  on rendered LLVM text
- after the remaining `memory.cpp` families shrink further, inventory the two
  surviving `calls.cpp` exact-text seams and the lone `lir_to_bir.cpp` matcher
- only return to new source-backed x86 case recovery after the active matcher
  family stops expanding

## Current Iteration Notes

- the active runbook has been reset from bounded case recovery to
  legacy-lowering cleanup
- already-landed matcher cleanups remain valid, but they are now treated as the
  start of a broader shrink-the-legacy-surface lane rather than as incidental
  debt paydown between new testcase recoveries
- broad-suite red buckets still exist outside this cleanup lane, so targeted
  validation remains the default guard unless a broader checkpoint is
  explicitly needed
- remaining exact-text inventory after this slice:
  one simple `memory.cpp` global-zero-return seam, nine larger `memory.cpp`
  compare/aggregate families, two `calls.cpp` exact-text routes, and one
  `lir_to_bir.cpp` legacy matcher
- the local single-field struct family no longer depends on rendered LLVM text:
  `try_lower_minimal_local_struct_shadow_store_compare_two_zero_return_module`,
  `try_lower_minimal_local_single_field_struct_store_load_zero_return_module`,
  and
  `try_lower_minimal_local_paired_single_field_struct_compare_sub_zero_return_module`
- targeted validation passed with
  `./build/backend_bir_tests local_struct_shadow_store_compare_two`,
  `./build/backend_bir_tests local_single_field_struct_store_load_zero`, and
  `./build/backend_bir_tests local_paired_single_field_struct_compare_sub_zero`
- broad validation checkpoint:
  `ctest --test-dir build -j --output-on-failure > test_fail_after.log`
  completed, but the monotonic regression guard against `test_fail_before.log`
  failed because the historical baseline is stale relative to the current tree
  (`2849 -> 2904` total tests) and reports unrelated new failing RISCV/runtime
  buckets plus a `backend_bir_tests` >30s timeout flag
- the compact global-struct compare family no longer depends on rendered LLVM
  text:
  `try_lower_minimal_global_anonymous_struct_field_compare_zero_return_module`,
  `try_lower_minimal_global_named_two_field_struct_designated_init_compare_zero_return_module`,
  and
  `try_lower_minimal_global_named_int_pointer_struct_designated_init_compare_zero_return_module`
- added a narrow lowering guard that renames the entry SSA temps for the global
  anonymous-struct compare route so the cleanup is no longer coupled to exact
  `print_llvm()` temp naming
- targeted validation passed with:
  `./build/backend_bir_tests test_bir_lowering_accepts_global_anonymous_struct_field_compare_zero_return_module`
  `./build/backend_bir_tests test_bir_lowering_accepts_global_anonymous_struct_field_compare_zero_return_module_with_renamed_entry_temps`
  `./build/backend_bir_tests test_bir_lowering_accepts_global_named_two_field_struct_designated_init_compare_zero_return_module`
  `./build/backend_bir_tests test_bir_lowering_accepts_global_named_int_pointer_struct_designated_init_compare_zero_return_module`
  and
  `ctest --test-dir build -R 'backend_codegen_route_x86_64_c_testsuite_0004(7|8|9)_' --output-on-failure`
- broad validation checkpoint refreshed on the current tree:
  `test_fail_after.log` now reports `94% tests passed, 181 tests failed out of 2904`
  versus the stale historical `test_fail_before.log` summary
  `94% tests passed, 179 tests failed out of 2849`; the after-log still keeps
  owned `00047`/`00048`/`00049` source and x86 route coverage green, but the
  broad log remains unsuitable for monotonic pass-count comparison because the
  baseline predates the current suite shape and `backend_bir_tests` now
  reproduces an unrelated full-binary crash path in this tree

## Recently Completed

- reset the active runbook to a cleanup-first plan that treats
  `try_lower_to_bir_legacy(...)` and `.phase = "legacy-lowering"` paths as
  shrink targets rather than normal expansion targets
- reset execution state so future slices start from the remaining legacy
  matcher inventory instead of the previous post-`00080.c` recovery queue
- replaced the local single-field struct family in
  `src/backend/lowering/lir_to_bir/memory.cpp` with structural instruction
  matching and a shared local-struct GEP helper, removing three adjacent
  `print_llvm()` / `kExpectedModule` seams from the active memory cleanup queue
- replaced the compact global-struct compare family in
  `src/backend/lowering/lir_to_bir/memory.cpp` with structural instruction
  matching and added a renamed-SSA lowering test guard, removing three more
  adjacent `print_llvm()` / `kExpectedModule` seams from the active memory
  cleanup queue
