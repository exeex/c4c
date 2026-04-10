# x86_64 Backend Bring-Up After BIR Cutover

Status: Active
Source Idea: ideas/open/44_x86_64_backend_bringup_after_bir_cutover.md
Source Plan: plan.md

## Current Active Item

- Step 2 low-risk `memory.cpp` matcher cleanup for the active cleanup-first runbook
- current exact slice:
  continue removing adjacent `memory.cpp` exact-text matcher families after the
  local single-field struct batch, starting with the next compare-heavy
  aggregate/string routes instead of widening x86 case recovery

## Next Slice

- continue the `memory.cpp` cleanup queue with the remaining exact-text compare
  ladders now concentrated around
  `try_lower_minimal_local_string_literal_char_compare_ladder_zero_return_module`
  and the adjacent global/nested aggregate compare families
- leave the simple `try_lower_minimal_single_global_i32_zero_return_module`
  seam parked until after the larger adjacent families stop depending on
  rendered LLVM text
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
