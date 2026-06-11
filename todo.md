Status: Active
Source Idea Path: ideas/open/190_full_suite_baseline_99_percent_regression_attribution.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Route The Repair Or Baseline Decision

# Current Packet

## Just Finished

Completed `plan.md` Step 4, "Route The Repair Or Baseline Decision", by
turning the Step 3 first-bad evidence into a focused repair route.

Implicated boundary:

- Route 3 owns semantic BIR memory/source identity: same-block global-load
  producer identity, result value identity, semantic global-symbol base,
  address-space, volatile bit, and BIR instruction index.
- AArch64/prepared still owns target policy: `PreparedMemoryAccess`,
  `PreparedAddress`, link-name spelling, byte offset, size/align,
  base-plus-offset legality, GOT/direct policy, scratch register choice, and
  final `ldr` materialization.

Why `c8346c7b` can make the three tests fail while parent `cc4d9e40` passes:

- The parent FP path used
  `prepare::find_prepared_same_block_scalar_producer` plus
  `prepare::find_prepared_same_block_global_load_access` to recover the
  prepared FP global-load access and then emitted the AArch64 load through
  `emit_prepared_fp_global_load_to_register`.
- `c8346c7b` replaced that prepared semantic lookup with
  `mir::find_bir_same_block_global_load_access_identity` and, in prepared
  mode, returns `false` when the Route 3 identity is absent or when the
  Route-3-selected producer cannot be matched to prepared memory access.
- The integer/publication global-load path in
  `dispatch_value_materialization.cpp` still keeps a prepared same-block
  global-load fallback after the Route 3 attempt; the FP path does not.
- `00119.c` and `00123.c` both compile to a direct `double` global load feeding
  an FP comparison. `00195.c` compiles to many `double` global-array
  load/select/store chains and later reloads for `printf`. These are exactly
  the AArch64 FP same-block global-load materialization shapes touched by
  `c8346c7b`, so a missing/too-narrow Route 3 answer leaves FP registers with
  stale or zero values instead of the prepared global memory value.

Classification:

- `00119`/`00123`/`00195` are deterministic regressions, not baseline noise:
  the parent `cc4d9e40` passed the fixed four-test reproducer, `c8346c7b`
  failed the same three tests, and later tested bad-side commits reproduced the
  same family.
- `00040` remains separate noise-or-history evidence because it passed at every
  tested commit in the Step 3 bracket.

## Suggested Next

Execute a focused implementation packet in the existing plan; no new durable
repair idea is needed yet.

Smallest next action: repair
`src/backend/mir/aarch64/codegen/fp_value_materialization.cpp` so FP
same-block global-load materialization keeps Route 3 as the semantic identity
source when it agrees with prepared memory access, but preserves a fail-closed
prepared semantic fallback equivalent to the integer/publication path when
Route 3 has no answer for a prepared FP global load. Keep
`PreparedMemoryAccess` as the target-addressing/address-policy source and do
not move target address payloads into BIR.

Suggested proof after repair:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_prepared_memory_operand_records|backend_prepared_lookup_helper|c_testsuite_aarch64_backend_src_00119_c|c_testsuite_aarch64_backend_src_00123_c|c_testsuite_aarch64_backend_src_00195_c)$'
```

## Watchouts

- Do not accept `test_baseline.new.log`, weaken c_testsuite expectations, or
  mark these cases unsupported.
- Do not fix this by matching `00119`, `00123`, `00195`, `@x`, or
  `@point_array` by name. The repair should be a general FP same-block
  global-load materialization rule.
- Do not make BIR carry `PreparedAddress`, concrete AArch64 relocation/GOT
  policy, base-plus-offset legality, or final memory operand formation.
- Preserve the Route 3 agreement checks already used by the FP path:
  Route-3-selected producer instruction index must still match the prepared
  memory access before using it as the target load source.
- If the implementation discovers Route 3 itself is missing a valid semantic
  FP global-load identity for ordinary same-block `LoadGlobalInst` results,
  keep that as a Route 3 semantic repair; otherwise prefer a local AArch64 FP
  fallback matching the existing non-FP consumer pattern.
- `00040` should stay outside this repair packet unless fresh evidence ties it
  to the same first-bad boundary.
- Keep `test_baseline.new.log` rejected until a non-regressive full-suite
  candidate exists. Do not weaken c_testsuite expectations.

## Proof

Investigation-only packet; no build or tests were run and no logs were
modified.

Read-only evidence inspected:

```sh
git show --find-renames --patch --unified=120 c8346c7bb052af1ae81c7ca95bee1f71a899ea6d -- src/backend/mir/aarch64/codegen/fp_value_materialization.cpp
git show cc4d9e401bc68d96b377b2b399d6451f3aea6b10:src/backend/mir/aarch64/codegen/fp_value_materialization.cpp
build/c4cll --dump-prepared-bir --mir-focus-function main tests/c/external/c-testsuite/src/00119.c
build/c4cll --dump-prepared-bir --mir-focus-function main tests/c/external/c-testsuite/src/00195.c
```

AST-backed lookups used:

```sh
c4c-clang-tool-ccdb find-definition /workspaces/c4c/src/backend/mir/query.cpp find_bir_same_block_global_load_access_identity build/compile_commands.json
c4c-clang-tool-ccdb find-definition /workspaces/c4c/src/backend/mir/query.cpp find_route3_global_load_access build/compile_commands.json
c4c-clang-tool-ccdb find-definition /workspaces/c4c/src/backend/mir/aarch64/codegen/fp_value_materialization.cpp emit_fp_value_to_register build/compile_commands.json
c4c-clang-tool-ccdb find-definition /workspaces/c4c/src/backend/mir/aarch64/codegen/dispatch_value_materialization.cpp emit_value_publication_to_register build/compile_commands.json
```

Result: Step 4 routed the next action to a focused repair packet, not a
baseline acceptance and not a new idea. Root `test_after.log` was not modified.
