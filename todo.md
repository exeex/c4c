Status: Active
Source Idea Path: ideas/open/584_rv64_20000819_runtime_mismatch_after_pointer_publication.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Repair The Runtime Cause

# Current Packet

## Just Finished

Step 4 repaired RV64 object emission for computed global-address call
arguments.

- `src/backend/mir/riscv/codegen/object_emission.cpp` now materializes
  supported computed global call arguments through the existing PC-relative
  address fragment path when the semantic source is `@global + constant_delta`.
- The object route accepts fully prepared
  `source_encoding=ComputedAddress` call-argument plans and also uses the BIR
  `call.arg_sources` computed-address relationship when the prepared call plan
  still exposes the stale register home.
- The repair emits the address into the prepared ABI destination GPR before the
  call and avoids copying stale homes such as `mv a0,s1` for this shape.
- Unsupported computed-address variants without direct global base metadata or
  byte-delta facts continue to fall through to the existing fail-closed object
  route.
- The representative `20000819-1.c` now emits `auipc/addi a0, a+4` before
  `foo` and the RV64 object/runtime route passes.
- Artifacts were written under
  `build/agent_state/584_rv64_20000819_runtime_mismatch_after_pointer_publication/step4/`.

## Suggested Next

Run Step 5: backend validation and closure decision, using the
supervisor-selected backend subset after a fresh build.

## Watchouts

- The fix deliberately does not change call planning in this packet; the object
  route bridges the existing BIR `call.arg_sources` relationship when prepared
  call metadata has not retained the computed-address source.
- Step 5 should verify the broader backend subset before lifecycle closure.

## Proof

Step 4 focused proof wrote `test_after.log`.

- Command:
  `{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_riscv_object_emission$'; } > test_after.log 2>&1`
- Result: passed, `backend_riscv_object_emission` 1/1.
- Representative route artifacts:
  `build/agent_state/584_rv64_20000819_runtime_mismatch_after_pointer_publication/step4/src_20000819-1.c/object-route.log`,
  `dump-prepared-bir.txt`, `c4c-o-objdump-dr.txt`, and
  `build/agent_state/584_rv64_20000819_runtime_mismatch_after_pointer_publication/step4/summary.txt`.
- Representative result: passed; the prior
  `[RV64_BACKEND_RUNTIME_MISMATCH]` / `Subprocess aborted` cause is gone.
