# LIR To BIR Global Pointer Aggregate Projection Todo

Status: Active
Source Idea Path: ideas/open/298_lir_to_bir_global_pointer_aggregate_projection.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Decide The 00216 Boundary

# Current Packet

## Just Finished

Step 4 inspected and repaired the `00216` boundary case. The old blocker was
the first `foo` pointer-parameter projection:

```llvm
%t381 = getelementptr %struct.W, ptr %p.w, i32 0, i32 0
```

`%struct.W` is a flexible-array wrapper lowered as
`{ %struct.V, [0 x %struct.S] }`. The aggregate layout parser rejected the
zero-length array layer, so the fixed prefix had no valid layout and the
pointer-parameter GEP failed in `gep local-memory semantic family`.

The semantic repair admits `[0 x T]` aggregate array layers as zero-size arrays
with element alignment, preserving fixed-prefix struct projection. Focused
coverage was added for projecting the `%struct.W` fixed prefix through
`backend_prepare_structured_context`.

Focused result classification from `test_after.log`:

- `00216` moved past the old `gep local-memory semantic family` projection
  admission blocker in `foo`; it now fails later in `foo` at
  `load local-memory semantic family`.
- `00209` still passes the delegated AArch64 backend subset.
- `00176`, `00181`, and `00182` preserve the Step 2 AArch64 symbol-store
  printer residual.
- `00195` remains a runtime mismatch.
- `00205` remains the scalar `logical_shift_right` printer operand residual.
- `00204` remains the later `myprintf` local GEP residual.

## Suggested Next

The next coherent packet is Step 5 residual classification/proof for this
owner: record that no focused case remains behind the old projection admission
blocker, and decide whether the new `00216` load-local-memory residual belongs
with the already-known post-admission residual buckets or needs a separate
owner.

## Watchouts

- `00216` is now a post-GEP `load local-memory semantic family` residual, not
  a projection-admission residual. Avoid stretching the projection owner into
  aggregate load semantics without supervisor approval.
- The delegated subset is not fully green. `00176`, `00181`, and `00182`
  remain in the prior AArch64 symbol-store printer residual; `00195` remains a
  runtime mismatch; `00205` remains a scalar printer residual; `00204` remains
  a `myprintf` local GEP residual.
- The zero-length array layout change is semantic and general: it admits
  flexible-array wrappers with fixed prefixes, not a `00216` shortcut.
- Do not absorb unrelated runtime nonzero/mismatch buckets, standalone timeout
  `00220`, expectation changes, runner changes, or unsupported downgrades into
  this projection owner.

## Proof

Proof command run exactly:

```bash
cmake --build build --target c4cll && ctest --test-dir build -R '^(backend_lir_to_bir_notes|c_testsuite_aarch64_backend_src_(00176|00181|00182|00209|00195|00205|00204|00216)_c)$' -j --output-on-failure | tee test_after.log
```

Result: build succeeded; subset was 2/9 passing. `backend_lir_to_bir_notes`
and `00209` passed. `00216` moved from the old `gep local-memory semantic
family` projection admission blocker to `load local-memory semantic family` in
`foo`. `00176`, `00181`, `00182`, `00195`, `00205`, and `00204` preserved the
existing residual classifications. Proof log: `test_after.log`.

Additional focused unit check:

```bash
ctest --test-dir build -R '^backend_prepare_structured_context$' --output-on-failure
```

Result: passed.
