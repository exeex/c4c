Status: Active
Source Idea Path: ideas/open/116_bir_layout_dual_path_coverage_and_dump_guard.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Cover Aggregate Call ABI And Variadic Paths

# Current Packet

## Just Finished

Completed `plan.md` Step 4 aggregate call ABI and variadic coverage inventory
without changing backend behavior.

Existing semantic BIR route tests already cover sret aggregate returns,
byval/byref aggregate parameters, direct and indirect aggregate
function-pointer calls, scalar and aggregate variadic arguments, indirect
variadic calls, `va_arg` for scalar and aggregate values, and `va_copy`.
The HFA-like mixed payload fixture remains runtime-level coverage because the
semantic BIR route does not expose stable target HFA classification for that
case.

## Suggested Next

Advance to `plan.md` Step 5 with one focused dump-guard packet for an existing
layout-sensitive aggregate path whose semantic BIR output already exposes
stable facts, such as aggregate `sret`/`byval` call ABI or aggregate `va_arg`
copy offsets.

## Watchouts

- Keep legacy `type_decls` available as fallback and parity evidence.
- Do not remove legacy parsing helpers during this runbook.
- Treat `--dump-bir` tests as guards for lowered BIR facts, not as a BIR
  printer render-context migration.
- Do not downgrade expectations or add named-case shortcuts.
- Do not force HFA-like coverage into a semantic BIR text check unless the
  route starts exposing stable target HFA classification facts.
- The Step 4 inventory found no obvious missing focused semantic BIR route
  guard among the delegated aggregate call ABI and variadic families.

## Proof

Proof command run:
`{ cmake --build build-backend && ctest --test-dir build-backend -j --output-on-failure -R '^(frontend_lir_|backend_codegen_route_x86_64_(byval_member_array_params|aggregate_return_pair|aggregate_param_return_pair|indirect_aggregate_param_return_pair|aggregate_param_return_pair_fn_param|variadic_sum2|indirect_variadic_sum2|variadic_double_bytes|variadic_pair_second|indirect_variadic_pair_second|variadic_va_copy_accumulate|byval_helper_mixed_hfa_payload)_observe_semantic_bir|backend_runtime_byval_helper_mixed_hfa_payload)$'; } > test_after.log 2>&1`

Result: passed. Build completed and 12/12 selected tests passed.
Proof log: `test_after.log`.
