Status: Active
Source Idea Path: ideas/open/554_out_of_ssa_parallel_copy_move_bundle_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Reproduce And Map Producer Facts

# Current Packet

## Just Finished

Step 1 (`Reproduce And Map Producer Facts`) mapped the producer path for the
current `src/960209-1.c` missing-bundle row without semantic repair.

AST-backed tracing found the value-location producer path:
`src/backend/prealloc/out_of_ssa.cpp::make_parallel_copy_bundle` publishes
`PreparedParallelCopyBundle` edge coordinates and execution block labels;
`src/backend/prealloc/regalloc/phi_moves.cpp::append_phi_move_resolution`
iterates those bundles, obtains the execution block through
`published_prepared_parallel_copy_execution_block_index`, and emits
`PreparedMoveResolution` records with
`PreparedMoveAuthorityKind::OutOfSsaParallelCopy` plus predecessor/successor
labels; `src/backend/prealloc/regalloc.cpp::append_prepared_move_bundle`
groups those move resolutions onto `PreparedValueLocationFunction::move_bundles`.

Prepared-BIR inspection for `tests/c/external/gcc_torture/src/960209-1.c`
showed the concrete first bad producer fact. Prepared control flow publishes
four parallel-copy bundles for `f`, including
`tern.else.end.37 -> tern.end.38` with execution block `tern.else.end.37`.
Prepared value locations publish only three out-of-SSA move bundles:
`tern.then.end.35 -> tern.end.38`,
`tern.then.end.51 -> tern.end.54`, and
`tern.else.end.53 -> tern.end.54`. The missing edge is the immediate-source
copy `0 -> %t42` for `tern.else.end.37 -> tern.end.38`.

The likely minimal repair surface is producer-side immediate parallel-copy
publication: `phi_moves.cpp::append_immediate_i32_move_resolution_record`
currently returns unless the source is an immediate `I32`, but this row needs
an integer immediate `I64` zero to become an
`OutOfSsaParallelCopy` move bundle with the already-known execution block and
edge labels. RV64 materialization in
`object_emission.cpp::fragment_for_prepared_out_of_ssa_moves` has the same
`I32` immediate acceptance shape, so the follow-up repair should keep producer
publication and materialization type support aligned.

## Suggested Next

Execute Step 2 by generalizing out-of-SSA immediate parallel-copy publication
for scalar integer immediates beyond `I32`, preserving the existing
phase/authority/execution-block/predecessor/successor coordinate contract and
adding focused coverage for the missing `i64 0 -> phi/select` edge shape.

## Watchouts

- Do not infer missing move bundles in the prepared consumer.
- Do not special-case `src/960209-1.c`, block label 20, predecessor 20,
  successor 19, or block index 15.
- Preserve the structured missing-bundle diagnostic while repairing producer
  publication facts.
- The current aggregate diagnostic reports zero execution-block matches because
  the exact immediate-source edge bundle is absent, not because an existing
  bundle has only swapped labels.
- Prepared-BIR dump command used for mapping:
  `./build/c4cll --dump-prepared-bir --target riscv64-unknown-linux-gnu tests/c/external/gcc_torture/src/960209-1.c`.

## Proof

Delegated proof command was run exactly:

```sh
{ echo '== cmake --build --preset default =='; cmake --build --preset default; echo '== ALLOWLIST=build/agent_state/553_step1_evidence_gap.allowlist scripts/check_progress_rv64_gcc_c_torture_backend.sh =='; ALLOWLIST=build/agent_state/553_step1_evidence_gap.allowlist scripts/check_progress_rv64_gcc_c_torture_backend.sh; } > test_after.log 2>&1
```

Build was up to date. The focused scan exited `1` because `src/960209-1.c`
still fails as expected with `[RV64_C4C_OBJ_COMPILE_FAIL]` and
`prepared_consumer_category=missing_move_bundle`. Canonical proof log:
`test_after.log`; case log:
`build/rv64_gcc_c_torture_backend/src_960209-1.c/case.log`.
