Status: Active
Source Idea Path: ideas/open/18_aarch64_cts_00181_runtime_regression_reopen.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Trace the regression against prior repair assumptions

# Current Packet

## Just Finished

Step 2 traced the current `Move` failure against the prior `00181` repair
assumptions without implementation or expectation edits. The stale assumption
is that the current value-publication register hazard check only needs to know
whether a nested operand may read the outer target/scratch registers. That was
sufficient for the earlier idea 13 closure, where `00181` passed after the
pointer/index arithmetic repair, but it is no longer sufficient for the current
nested materialization path.

Current failing fact: the selected/prepared pointer base is present and usable
(`source` in `x0`, `dest` in `x1`), but the outer pointer-add publication does
not preserve that selected base while materializing the RHS byte offset. In
`dispatch_value_materialization.cpp`, the outer add materializes the LHS into
target `x9`, then materializes RHS `i * 4` into scratch `x10` with nested
scratch `x9`. The multiply path emits `mov x9, #4` for the RHS immediate and
therefore overwrites the already-materialized pointer base before the outer
`add x9, x9, x10`. Generated `Move` consequently loads from `4 + i * 4`
instead of `source + i * 4`, and repeats the same pattern for `dest[j]`.

Owning repair surface: `src/backend/mir/aarch64/codegen/dispatch_value_materialization.cpp`
should own the fix in the nested binary add/mul materialization hazard logic.
The related helper `value_publication_may_read_register_index` in
`src/backend/mir/aarch64/codegen/dispatch_publication.cpp` is read-only
hazard analysis today; the missing or incorrect fact is a clobber/write fact
for nested materialization scratch use, not a missing global, call-boundary, or
absent-selection fallback fact.

## Suggested Next

Execute Step 3 from `plan.md`: repair the semantic AArch64 value-publication
lowering so nested RHS materialization cannot clobber an already materialized
pointer base in the outer add/sub result register. Add focused coverage for the
same nested pointer-index addressing shape before relying on `00181` alone.

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

## Proof

Ran exactly:

```sh
cmake --build --preset default
ctest --test-dir build -j --output-on-failure -R '^c_testsuite_aarch64_backend_src_00181_c$'
```

Combined output is captured in `test_after.log`. Build was up to date
(`ninja: no work to do`); the targeted CTest subset ran one test and failed
`c_testsuite_aarch64_backend_src_00181_c` with `[RUNTIME_NONZERO]` /
`exit=Segmentation fault`. This proof is sufficient for the trace-only Step 2
packet; it intentionally does not prove a fix.
