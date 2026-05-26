Status: Active
Source Idea Path: ideas/open/18_aarch64_cts_00181_runtime_regression_reopen.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Repair the semantic lowering rule

# Current Packet

## Just Finished

Step 3 repaired the nested pointer-index materialization hazard. The local
value-publication path now has a scratch-write hazard fact and recognizes
same-block power-of-two multiply scales as shiftable, so nested RHS
materialization is not incorrectly classified as harmless when it can clobber
the outer target.

The final live producer for the failing `Move` sequence was the edge/select
chain publication copier, not the ordinary same-block value materializer. Its
binary edge publication path now lowers `mul` by a non-negative power-of-two
immediate as `lsl` before the generic RHS scratch path can overwrite the
already-materialized pointer base. Generated `Move` now keeps `source`/`dest`
in `x9`, emits `lsl x10, x10, #2`, and then performs `add x9, x9, x10`; the
stale `mov x9, #4` / `mul x10, x10, x9` clobber is gone.

## Suggested Next

Supervisor can review and commit this Step 3 code slice, then choose whether
the active plan needs broader AArch64 validation or can advance to the next
planned packet.

## Watchouts

- Do not special-case `00181`, Tower of Hanoi symbols, or one exact global
  array shape.
- Do not weaken or disable c_testsuite expectations.
- Do not restore broad target-local fallback selection as a substitute for
  complete prepared facts.
- Keep unrelated AArch64 calls, dispatch, and c_testsuite cleanup outside this
  route.
- The direct global-array references in `PrintAll` and `main` use `adrp/add`
  forms and the binary contains local `.data` symbols for `A`, `B`, and `C`;
  avoid starting with global storage unless Step 2 contradicts this evidence.
- In `Move`, generated lines around the first failing pattern are
  `build/c_testsuite_aarch64_backend/src/00181.c.s` lines 429-435 for
  `source[i]` and 477-483 for `dest[j]`.
- The evidence points at register clobbering during nested value
  materialization. A repair that only restores broad target-local fallback
  source reconstruction would be route drift.
- Keep the fix semantic: the same hazard can occur whenever an outer add/sub
  keeps a pointer base in the target register while a nested RHS expression
  uses that same register as its scratch.
- Ownership expanded beyond the original delegated code file because the exact
  stale producer was `dispatch_edge_copies.cpp` edge/select-chain publication.
  The expansion is still within the same AArch64 value-publication failure
  family and did not touch expectations or unrelated lowering routes.

## Proof

Ran exactly:

```sh
cmake --build --preset default
ctest --test-dir build -j --output-on-failure -R '^c_testsuite_aarch64_backend_src_00181_c$'
```

Combined output is captured in `test_after.log`. The build completed after
recompiling and relinking the affected backend targets. The targeted CTest
subset ran one test and passed `c_testsuite_aarch64_backend_src_00181_c`.
