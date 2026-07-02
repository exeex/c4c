Status: Active
Source Idea Path: ideas/open/561_bir_bootstrap_global_data_shape_handoff_support.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add Focused Global Data-Shape Handoff Coverage

# Current Packet

## Just Finished

Completed plan Step 2 coverage for the retained bootstrap/global data-shape
handoff boundary. Added focused BIR coverage for string-backed nested
`char` array globals shaped like `[2 x [3 x i8]]` and
`[2 x [2 x [5 x i8]]]`, asserting that module/global lowering publishes
linear byte-addressable `i8` storage facts, element counts, storage sizes,
and initializer byte order without naming `strlen-2.c` or RV64 diagnostics.

The positive coverage required the minimal BIR handoff repair in the
integer-array initializer helper used by `globals.cpp`: byte-string
initializers now strip typed `[N x i8]` wrappers and are accepted inside
nested integer-array recursion, so nested string-backed `char` array globals
can lower through `lower_minimal_global_impl` as coherent BIR global storage.

Supervisor-run representative proof for Step 2 confirmed the retained BIR
handoff moved: `src/strlen-2.c` no longer fails at the bootstrap
global-data-shape handoff diagnostic. Its current first residual is
`semantic lir_to_bir function 'test_array_ref_2_2_5' failed in gep
local-memory semantic family`, which is not prepared/RV64 global-data handoff.
The other reconciliation rows remain downstream `unsupported_global_data`.

## Suggested Next

Ask for lifecycle judgment before further implementation: the retained
bootstrap/global data-shape handoff moved for `src/strlen-2.c`, and the new
first owner appears to be a GEP local-memory residual rather than prepared/RV64
global-data handoff.

## Watchouts

- This packet touched `src/backend/bir/lir_to_bir/global_initializers.cpp`,
  the integer-array initializer helper called by `globals.cpp`, because the
  coverage failure was below `lower_minimal_global_impl` rather than in
  `module.cpp` itself.
- `src/strlen-2.c` now exposes a `gep local-memory semantic family` residual
  after the BIR handoff repair. Do not treat that as Step 3 prepared/RV64
  global-data consumption without lifecycle review.
- Keep this route in BIR bootstrap/global data-shape handoff ownership.
- Do not fold prepared global-data layout, RV64 object-route global lowering,
  stack-frame support, move-bundle classification, or F128 work into this
  source idea without a lifecycle split.
- Do not count these related rows as exact `semantic lir_to_bir` producer rows.
- The scalar-global representative and the intrinsic/aggregate-backed
  representatives in this packet already moved to downstream
  `unsupported_global_data`; use them as reconciliation rows, not as evidence
  for widening Step 2 into prepared/RV64 global-data consumption.
- Scalar globals, already-supported linear integer arrays, and downstream
  aggregate/global memory consumers do not all share one Step 2 repair
  boundary. The coherent retained BIR boundary is string-backed byte-address
  global storage for nested char arrays; prepared/RV64 global-memory facts
  require a separate lifecycle owner if the supervisor wants to pursue them.

## Proof

- Focused pre-proof:
  `cmake --build --preset default --target backend_lir_to_bir_notes_test && ./build/tests/backend/bir/backend_lir_to_bir_notes_test`
- Delegated proof:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`
- Supervisor representative proof:
  `printf '%s\n' src/strlen-2.c src/20000703-1.c src/20041218-1.c src/20140212-1.c > build/agent_state/561_step2_global_data_shape_after.allowlist && ALLOWLIST=build/agent_state/561_step2_global_data_shape_after.allowlist VERBOSE_FAILURES=1 scripts/check_progress_rv64_gcc_c_torture_backend.sh > build/agent_state/561_step2_global_data_shape_after.log 2>&1`
- Representative proof exit status: `1`, acceptable for residual
  classification. `src/strlen-2.c` moved off bootstrap/global data-shape
  handoff and now fails in `gep local-memory semantic family`; the other rows
  remain downstream `unsupported_global_data`.
- `test_after.log` is the canonical proof log path for this packet.
- Representative proof log:
  `build/agent_state/561_step2_global_data_shape_after.log`.
