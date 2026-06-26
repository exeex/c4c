Status: Active
Source Idea Path: ideas/open/401_rv64_object_route_scalar_and_floating_edge_lowering.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Generalize Scalar Compare/Trunc Lowering

# Current Packet

## Just Finished

Step 2 - Generalize Scalar Compare/Trunc Lowering classified the three
remaining generic RV64 object-route instruction-fragment failures and completed
the in-scope reusable compare follow-up.

Implemented semantic branch-predicate normalization reuse for ordinary select
materialization: `sle`, `ugt`, and `ule` now normalize through the same helper
surface as fused compare branches before selecting RV64 branch funct3/operands.
Focused backend coverage now includes a fused `sle` branch and an ordinary
`sle` select with a stack-published result.

Boundary classification from prepared BIR/probe:

- `src/compare-2.c`: fixed; the failing boundary was ordinary select lowering
  for `%t11 = bir.select sle i32 %p.x, %p.y, i32 %t7, 0`, not compare-result
  publication. The select result is stack-homed, and `sle` can lower as swapped
  `sge`; the delegated probe now reports `pass`.
- `src/int-compare.c`: still outside Step 2. The first reproduced boundary is
  wide rematerialized-immediate producer admission around `INT_MIN`
  construction (`bir.sub i32 0, 2147483647` then subtract `1`). The values are
  prepared as rematerializable immediates with no destination home, while
  `prepared_pure_instruction_is_rematerialized_immediate` only skips immediates
  fitting signed 12-bit. This is a producer/rematerialization admission gap, not
  scalar compare/trunc lowering.
- `src/pr48973-2.c`: still outside Step 2. The prepared bitfield/global chain
  reaches `bir.ashr i32 ..., 31` for sign extraction after global bitfield load;
  RV64 object binary lowering supports `shl`/`lshr` but not `ashr`. This is a
  scalar shift/bitfield-global lowering gap, not scalar compare/trunc lowering.

## Suggested Next

Next coherent packet should leave Step 2 compare/trunc and split the two
remaining generic failures into separate route work: one for wide
rematerialized-immediate pure producers with no destination home, and one for
RV64 object `ashr`/bitfield-global scalar lowering.

## Watchouts

- `ideas/closed/403_prepared_i16_formal_abi_publication.md` is complete; do not
  reopen `I16` formal ABI publication in this object-route plan.
- Do not special-case `src/20000313-1.c`, `src/20000403-1.c`,
  `src/int-compare.c`, `src/compare-2.c`, `src/pr48973-2.c`, function names, or
  constants.
- Split missing prepared producer facts into a separate idea instead of
  compensating inside RV64 object emission.
- The remaining generic diagnostics are not scalar compare-result publication
  blockers anymore; `src/compare-2.c` now passes through reusable predicate
  normalization, while `src/int-compare.c` and `src/pr48973-2.c` are separate
  producer/shift lowering families.
- The Step 1 floating representative remains separable `SIToFP i32 0 ->
  double` work and was intentionally not included in this scalar packet.

## Proof

Ran the supervisor-selected Step 2 remaining-generic proof exactly:

```bash
cmake --build --preset default && mkdir -p build/agent_state && printf '%s\n' 'src/int-compare.c' 'src/compare-2.c' 'src/pr48973-2.c' > build/agent_state/401_step2_remaining_generic.allowlist.txt && { ALLOWLIST=build/agent_state/401_step2_remaining_generic.allowlist.txt STOP_ON_FAILURE=0 VERBOSE_FAILURES=1 scripts/check_progress_rv64_gcc_c_torture_backend.sh > build/agent_state/401_step2_remaining_generic_probe.log 2>&1 || true; } && rg -n 'unsupported_scalar_compare_trunc|unsupported_instruction_fragment|unsupported RV64 object lowering|\[rv64-gcc-torture\]|pass\t|fail\t|RV64_C4C_OBJ|error:' build/agent_state/401_step2_remaining_generic_probe.log && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log
```

Result: build succeeded; probe log
`build/agent_state/401_step2_remaining_generic_probe.log` reports total=3,
passed=1 (`src/compare-2.c`), failed=2 (`src/int-compare.c`,
`src/pr48973-2.c`); backend CTest passed and is recorded in `test_after.log`.
