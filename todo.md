Status: Active
Source Idea Path: ideas/open/561_bir_bootstrap_global_data_shape_handoff_support.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add Focused Global Data-Shape Handoff Coverage

# Current Packet

## Just Finished

Completed plan Step 1 inspection for the bootstrap/global data-shape handoff,
then advanced lifecycle state to Step 2.

Representative outcomes from
`build/agent_state/561_step1_global_data_shape.log`:

- `src/strlen-2.c`: still fails before prepared/RV64 consumption at the BIR
  bootstrap global lowering gate:
  `bootstrap lir_to_bir only supports scalar integer/pointer globals, linear
  integer-array globals, and aggregate-backed globals with honest byte-address
  semantics right now`. The source shape is static const multidimensional
  `char` globals initialized from string literals (`a[2][3]`,
  `b[2][2][5]`) plus scalar volatile index globals. The likely missing BIR
  handoff fact is byte-addressable global storage for string-backed nested
  char-array initializers, not a prepared/RV64 consumption fact.
- `src/20000703-1.c`: no longer fails at BIR bootstrap/global data-shape
  handoff; it reaches downstream `unsupported_global_data: RV64 object route
  requires supported prepared global memory facts`.
- `src/20041218-1.c`: no longer fails at BIR bootstrap/global data-shape
  handoff; it reaches downstream `unsupported_global_data: RV64 object route
  requires supported prepared global memory facts`.
- `src/20140212-1.c`: scalar integer/char globals lower past the BIR handoff
  and reach downstream `unsupported_global_data: RV64 object route requires
  supported prepared global memory facts`.

Likely BIR owners for the retained bootstrap boundary:
`src/backend/bir/lir_to_bir/module.cpp` `lower_module`, and
`src/backend/bir/lir_to_bir/globals.cpp` `lower_minimal_global_impl` with
the integer-array and aggregate initializer helpers it calls. The downstream
prepared/RV64 consumer diagnostics are owned by
`src/backend/mir/riscv/codegen/object_emission.cpp` global-data memory access
checks, not this Step 2 BIR bootstrap repair.

## Suggested Next

Execute Step 2 by adding focused BIR coverage for the retained bootstrap
boundary: string-backed byte-address global storage and nested char-array
initializers should publish coherent BIR global storage facts through the
module/global lowering handoff. Keep the tests semantic and shape-based rather
than tied to `strlen-2.c` or RV64 diagnostics.

## Watchouts

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

- Delegated inspection proof:
  `printf '%s\n' src/strlen-2.c src/20000703-1.c src/20041218-1.c src/20140212-1.c > build/agent_state/561_step1_global_data_shape.allowlist && ALLOWLIST=build/agent_state/561_step1_global_data_shape.allowlist VERBOSE_FAILURES=1 scripts/check_progress_rv64_gcc_c_torture_backend.sh > build/agent_state/561_step1_global_data_shape.log 2>&1`
- Exit status: `1`, acceptable for this inspection packet.
- Log paths:
  `build/agent_state/561_step1_global_data_shape.log`,
  `build/rv64_gcc_c_torture_backend/src_strlen-2.c/case.log`,
  `build/rv64_gcc_c_torture_backend/src_20000703-1.c/case.log`,
  `build/rv64_gcc_c_torture_backend/src_20041218-1.c/case.log`,
  `build/rv64_gcc_c_torture_backend/src_20140212-1.c/case.log`.
