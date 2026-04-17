Status: Active
Source Idea: ideas/open/54_x86_backend_c_testsuite_capability_families.md
Source Plan: plan.md

# Current Packet

## Just Finished

Completed `plan.md` Steps 3-5 for one bounded single-function prepared-module
lane by admitting a same-module direct helper-call route where `main`
directly returns one extra helper that already fits the existing minimal i32
single-function emitter shapes. The handoff boundary now covers the helper
call lane proved by `00100.c` and `00116.c`, and the truthful
`x86_backend` checkpoint moved from `57/220` passed and `163/220` failed to
`59/220` passed and `161/220` failed.

## Suggested Next

Start a fresh Step 1/2 packet inside the remaining single-function
prepared-module bucket to choose the next bounded neighbor after the landed
helper-call lane, rather than broadening blindly into two-parameter helpers,
void-return helpers, stdio-heavy routes, or indirect/variadic calls.

## Watchouts

- Do not weaken `x86_backend` expectations to accept fallback LLVM IR.
- Do not add testcase-named shortcuts or rendered-text recognizers.
- Do not silently reuse the completed pointer-backed same-module global lane as
  the active route.
- Treat the broader single-function prepared-module bucket as larger than the
  landed lane: keep indirect/variadic multi-function routes like `00189.c`
  out of scope.
- Keep stdio-heavy prepared-module neighbors such as `00131.c` and `00154.c`
  separate from the direct helper-call route.
- Keep two-parameter helper calls such as `00021.c` and void-return helper
  calls such as `00080.c` as separate next-lane candidates until Step 1/2
  chooses one explicitly.
- Keep bootstrap scalar globals (`00045.c`) and scalar-control-flow blockers
  (`00051.c`) as adjacent but separate families.

## Proof

Ran the narrow proof
`cmake --build --preset default && (ctest --test-dir build -j --output-on-failure -R '^(backend_lir_to_bir_notes|backend_x86_handoff_boundary|c_testsuite_x86_backend_src_00100_c|c_testsuite_x86_backend_src_00116_c)$' || true) > test_after.log 2>&1`
and all four checks passed. Then reran
`cmake --build --preset default && (ctest --test-dir build -L x86_backend --output-on-failure || true) > test_after.log 2>&1`
for the truthful checkpoint; `test_after.log` now records `59/220` passed and
`161/220` failed. The shell command remains success-forced because the label
still has open failures.
