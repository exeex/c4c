# LIR To BIR Global Pointer Aggregate Projection Todo

Status: Active
Source Idea Path: ideas/open/298_lir_to_bir_global_pointer_aggregate_projection.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair Global And Pointer Projection Admission

# Current Packet

## Just Finished

Step 2 first packet implemented semantic admission for the diagnosed simpler
projection forms:

- global/static-local scalar array GEPs now create dynamic global scalar-array
  accesses and support dynamic scalar-array stores through finite
  load/select/store materialization
- pointer-parameter and pointer-value scalar GEPs now lower typed runtime
  pointer arithmetic and preserve pointer provenance for following loads/stores
- raw `i8` pointer increments now preserve pointer provenance after pointer
  arithmetic

Focused result classification from `test_after.log`:

- `00209` passes the delegated AArch64 backend subset.
- `00176`, `00181`, and `00182` no longer fail at the old
  `gep local-memory semantic family` admission point. They now reach the
  AArch64 machine-node printer and fail on `opcode=store: symbol store value is
  not a register or immediate operand`.

## Suggested Next

Supervisor should decide whether the new AArch64 symbol-store printer residual
belongs in a separate machine-printer owner or a continuation packet. For this
idea, the next coherent projection packet is Step 3 aggregate/bootstrap
admission for `00195`, `00205`, and `00204`, with `00216` still reserved as the
Step 4 boundary check.

## Watchouts

- The delegated subset is not fully green: `00176`, `00181`, and `00182` moved
  into an AArch64 printer residual after projection admission. Do not count
  those as runtime-ready backend passes.
- The dynamic global scalar-array store path emits finite load/select/store
  BIR over the declared array extent. The new printer residual appears after
  that semantic lowering succeeds.
- `00216` should not be used as the first implementation driver because it
  combines pointer-parameter aggregate projection with flexible-array and
  broad aggregate-initializer coverage.
- Do not absorb unrelated runtime nonzero/mismatch buckets, standalone timeout
  `00220`, expectation changes, runner changes, or unsupported downgrades into
  this projection owner.

## Proof

Proof command run exactly:

```bash
cmake --build build --target c4cll && ctest --test-dir build -R 'c_testsuite_aarch64_backend_src_(00176|00181|00182|00209)_c$' -j --output-on-failure | tee test_after.log
```

Result: build succeeded; subset was 1/4 passing. `00209` passed. `00176`,
`00181`, and `00182` failed after semantic lowering reached the AArch64
machine-node printer, all with `opcode=store: symbol store value is not a
register or immediate operand`. Proof log: `test_after.log`.
