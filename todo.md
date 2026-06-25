Status: Active
Source Idea Path: ideas/open/358_rv64_object_route_abi_width_edges.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Mixed Edge Boundaries

# Current Packet

## Just Finished

Completed Step 1 audit of the five RV64 mixed ABI/width edge representatives.
The fresh temporary allowlist run produced 3 passing cases and 2 current
unsupported boundaries:

- `src/20010119-1.c`: passes the RV64 object route.
- `src/20001203-1.c`: fails at
  `unsupported_local_memory_access: RV64 object route supports only 32-bit and 64-bit prepared local memory accesses`.
- `src/20030216-1.c`: passes the RV64 object route.
- `src/20030125-1.c`: fails at
  `unsupported_instruction_fragment: BIR instruction requires unsupported RV64 object lowering`.
- `src/920410-1.c`: passes the RV64 object route.

## Suggested Next

Execute Step 3's first semantic slice: add supported narrow local-memory width
lowering for prepared frame-slot base-plus-offset local accesses, starting from
the existing `i16` local-store fixture and the `src/20001203-1.c` boundary.
Owned files should be:

- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `tests/backend/mir/backend_riscv_object_emission_test.cpp`
- `todo.md`

Suggested narrow proof:

```sh
cmake --build build --target backend_riscv_object_emission_test && ctest --test-dir build -R '^backend_riscv_object_emission$' --output-on-failure > test_after.log
```

## Watchouts

- Do not route vararg or `va_arg` cases back into this plan.
- Do not hard-code testcase names or source patterns.
- Split upstream missing prepared semantics into a separate idea instead of
  repairing them in RV64 object emission.
- `src/20030125-1.c` should not be the first implementation packet. It still
  falls through a generic unsupported instruction diagnostic, so the next work
  on that case should first classify the prepared floating/math instruction
  shape and decide whether it is target FPR ABI work or a separate prepared
  semantics gap.
- No audited case needs a new split idea yet. Reconsider this if the
  `src/20030125-1.c` classification shows missing upstream prepared call/FPR
  facts rather than target-consumable RV64 object facts.

## Proof

Audit rerun:

```sh
tmp=$(mktemp); printf '%s\n' src/20010119-1.c src/20001203-1.c src/20030216-1.c src/20030125-1.c src/920410-1.c > "$tmp"; out=$(mktemp); CASE_TIMEOUT_SEC=20 ALLOWLIST="$tmp" scripts/check_progress_rv64_gcc_c_torture_backend.sh > "$out"; rc=$?; tail -120 "$out"; rm -f "$tmp" "$out"; exit $rc
```

The audit command exited `1` because two representatives are still unsupported.
Representative logs:

- `build/rv64_gcc_c_torture_backend/src_20010119-1.c/case.log`
- `build/rv64_gcc_c_torture_backend/src_20001203-1.c/case.log`
- `build/rv64_gcc_c_torture_backend/src_20030216-1.c/case.log`
- `build/rv64_gcc_c_torture_backend/src_20030125-1.c/case.log`
- `build/rv64_gcc_c_torture_backend/src_920410-1.c/case.log`

Delegated proof:

```sh
git diff --check -- todo.md
```
