Status: Active
Source Idea Path: ideas/open/616_select_publication_source_wiring.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Wire One Proven Select Publication Source Path

# Current Packet

## Just Finished

Step 2, `Wire One Proven Select Publication Source Path`, implemented the RV64
select-publication stack-slot source to GPR consumer path for available
publication intents with explicit source stack slot, offset, and scalar size
facts.

- Changed files:
  `src/backend/mir/riscv/codegen/prepared_edge_publication_emit.hpp`,
  `src/backend/mir/riscv/codegen/prepared_edge_publication_emit.cpp`,
  `src/backend/mir/riscv/codegen/object_emission.cpp`, and
  `tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp`.
- The new admission predicate requires select-carrier publication authority,
  `intent_status=available`, matching scalar source/destination type, explicit
  source stack slot/offset/size, a concrete GPR destination, and no source
  register, immediate, memory, pointer, destination-stack evidence, or pointer
  typed source/destination. Pointer select-publication stack sources remain on
  the existing pointer-specific contract.
- The select-publication diagnostic and predecessor-terminator fragment
  emission now accept this exact source-freshness path, so the prior
  `unsupported_source_stack_offset` owner is no longer the first owner in the
  temporary targeted probe.
- Temporary targeted probe under `/tmp/c4c_616_step2_probe`:
  `src/20000706-1.c`, `src/20000706-2.c`, `src/20000717-5.c`,
  `src/20071213-1.c`, `src/20120427-1.c`, and `src/20120427-2.c`
  now progress to `[RV64_BACKEND_RUNTIME_MISMATCH]`; `src/991216-1.c`
  moves to a non-select-publication `unsupported_move_bundle_target_shape`
  generic move-bundle owner.
- Focused positive/negative coverage was added for explicit I32 stack-source
  select publication, large source offsets, missing source offset with
  alias-like pointer fields, missing source freshness with stack destination,
  destination-legality separation, and the existing pointer large-offset
  fail-closed contract.
- The `4` move-bundle/source-home rows without explicit source freshness remain
  guarded by `UnsupportedSourceHome`; the targeted probe keeps `src/pr45034.c`,
  `src/pr53160.c`, `src/pr58726.c`, and `src/pr59221.c` on
  `select_publication_rejection_reason=intent_status_unsupported_source_home`.
  No source-home positive path, unsupported marker, allowlist, expectation,
  runtime/accounting, branch stack-source, terminator, destination fan-in, or
  generic select lowering files were changed.

## Suggested Next

Run Step 3, `Refresh Residuals And Split Decision`, against the `7`
stack-offset select-publication rows and the `4` source-home move-bundle guard
rows. Confirm which stack-offset rows move past `unsupported_source_stack_offset`
and record any downstream first owner without expanding this plan.

## Watchouts

- Step 2 used a temporary targeted probe and did not mutate canonical torture
  state artifacts; residual row counts still need a fresh Step 3 probe.
- Do not treat alias evidence or destination legality as source freshness.
- Keep branch stack-source, terminator, destination fan-in, runtime,
  expectation, unsupported-marker, allowlist, timeout, and accounting changes
  out of this idea.
- The `4` move-bundle source-home rows have publication evidence but not
  explicit source freshness in the current diagnostics; they are Step 3 guard
  rows, not positives for this path.
- `src/921124-1.c` and `src/920710-1.c` should remain classified under the
  terminator owner until terminator diagnostics expose a different first
  blocker.

## Proof

Focused check:
`cmake --build --preset default --target backend_riscv_object_emission_test backend_riscv_prepared_edge_publication_test && ctest --test-dir build -j --output-on-failure -R '^(backend_riscv_object_emission|backend_riscv_prepared_edge_publication)$'`
passed.

Targeted row probe:
manual runner calls derived from
`tests/backend/cmake/run_rv64_gcc_torture_backend_object_case.cmake`, writing
only under `/tmp/c4c_616_step2_probe`, showed six stack-offset rows moved to
runtime mismatch, `src/991216-1.c` moved to generic move-bundle failure, and
the four source-home guard rows remained `intent_status_unsupported_source_home`.

Delegated proof:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`
passed. `test_after.log` records `100% tests passed, 0 tests failed out of
346`.
