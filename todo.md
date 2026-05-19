# LIR To BIR Global Pointer Aggregate Projection Todo

Status: Active
Source Idea Path: ideas/open/298_lir_to_bir_global_pointer_aggregate_projection.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Repair Aggregate And Bootstrap Projection Admission

# Current Packet

## Just Finished

Step 3 implemented aggregate/bootstrap projection admission for the next
focused targets:

- direct global dynamic aggregate element GEPs now create
  `DynamicGlobalAggregateArrayAccess` records instead of failing before
  aggregate member projection
- dynamic global aggregate subobject GEPs over scalar arrays now create nested
  `DynamicGlobalScalarArrayAccess` records using the existing finite
  outer/inner select materialization path
- aggregate global bootstrap now admits `fp128` as the LIR spelling for BIR
  `F128`, and `0xL...` `fp128` aggregate initializers lower to byte-backed
  global payloads

Focused result classification from `test_after.log`:

- `00209` still passes the delegated AArch64 backend subset.
- `00176`, `00181`, and `00182` preserve the Step 2 movement and still fail in
  the AArch64 symbol-store printer residual.
- `00195` no longer fails at the old `gep local-memory semantic family`
  admission point; it reaches the runner and now fails with a runtime mismatch.
- `00205` no longer fails at the old `gep local-memory semantic family`
  admission point; it reaches the AArch64 machine-node printer and now fails on
  a scalar `logical_shift_right` printer operand contract.
- `00204` no longer fails at bootstrap/global aggregate admission; it now fails
  later in semantic lowering at function `myprintf` with `gep local-memory
  semantic family`.

## Suggested Next

The next coherent packet is Step 4 boundary/classification: inspect `00216`
and decide whether it shares the repaired pointer-parameter/flexible-array
aggregate projection rule or should split. If the supervisor continues Step 3
first, the smallest continuation is the new `00204` `myprintf` local GEP
residual.

## Watchouts

- The delegated subset is not fully green. `00176`, `00181`, and `00182`
  remain in the prior AArch64 symbol-store printer residual; this packet did
  not attempt that owner.
- `00195` now reaches runtime but produces the wrong aggregate-member values,
  so the next owner should inspect dynamic global scalar load/store
  materialization or AArch64 global memory emission before treating it as a
  projection-admission failure.
- `00205` now reaches a scalar `logical_shift_right` printer residual after
  aggregate projection admission succeeds.
- `00204` bootstrap/global aggregate admission is repaired; the remaining
  `myprintf` local GEP failure is narrower and should be checked against the
  Step 4 `00216` boundary before broadening this packet.
- Do not absorb unrelated runtime nonzero/mismatch buckets, standalone timeout
  `00220`, expectation changes, runner changes, or unsupported downgrades into
  this projection owner.

## Proof

Proof command run exactly:

```bash
cmake --build build --target c4cll && ctest --test-dir build -R '^(backend_lir_to_bir_notes|c_testsuite_aarch64_backend_src_(00176|00181|00182|00209|00195|00205|00204)_c)$' -j --output-on-failure | tee test_after.log
```

Result: build succeeded; subset was 2/8 passing. `backend_lir_to_bir_notes`
and `00209` passed. `00176`, `00181`, and `00182` preserved the Step 2
AArch64 symbol-store printer residual. `00195`, `00205`, and `00204` all moved
past their old aggregate/bootstrap projection admission blockers and are now
classified as runtime mismatch, scalar printer residual, and `myprintf` local
GEP residual respectively. Proof log: `test_after.log`.
