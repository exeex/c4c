Status: Active
Source Idea: ideas/open/54_x86_backend_c_testsuite_capability_families.md
Source Plan: plan.md

# Current Packet

## Just Finished

Completed `plan.md` Steps 2-5 for one bounded semantic local-memory sublane:
loaded local array-pointer addressing through a local pointer slot. The slice
preserves repeated local aggregate provenance across pointer store/load in
`lir_to_bir`, adds a matching `backend_x86_handoff_boundary` LIR route
fixture, and moves `c_testsuite_x86_backend_src_00130_c` onto the x86 backend
path. The `x86_backend` checkpoint improved from `57/220` passed to `58/220`
passed.

## Suggested Next

Pick the next in-route semantic local-memory packet explicitly instead of
quietly broadening scope. The cleanest next options are either another
pointer/aggregate local-addressing neighbor near `00130`, if one exists, or a
separate `i16` local-slot lane around `00086` and `00111`; keep that second
lane separate because it needs width support rather than more pointer
provenance.

## Watchouts

- Do not weaken `x86_backend` expectations to accept fallback LLVM IR.
- Do not add testcase-named shortcuts or rendered-text recognizers.
- Do not silently reuse the completed pointer-backed same-module global lane as
  the active route.
- The fresh checkpoint still shows adjacent families that must stay separated:
  `57` single-function-prepared-module failures, `12` bootstrap-global
  failures (`00040/00045/00119/00214`), about `10` scalar-control-flow
  semantic failures (`00051/00124/00218`), plus smaller direct-return and
  single-block emitter buckets.
- Do not flatten loaded aggregate-array pointers into scalar-array base math
  when the next `gep` still needs repeated aggregate stride from the original
  storage object; this slice depends on keeping `storage_type_text`,
  `base_byte_offset`, and repeated leaf coverage aligned.
- The new handoff fixture proves a guard-style route. A direct `return p[1][3]
  != 2;` shape still sits outside the current x86 emitter contract and should
  not be mistaken for a regression in this slice.
- Keep loop-heavy, runtime-heavy, global-heavy, `i16` local-slot, and
  multi-function prepared-module work out of this lane unless the next packet
  names that route explicitly.

## Proof

Baseline capture before implementation:
`cmake --build --preset default && ctest --test-dir build -L x86_backend --output-on-failure | tee test_before.log`

Narrow proof during the packet:
`ctest --test-dir build -R '^backend_x86_handoff_boundary$' --output-on-failure`
`ctest --test-dir build -R '^c_testsuite_x86_backend_src_00130_c$' --output-on-failure`

Checkpoint proof and canonical after-log:
`cmake --build --preset default && ctest --test-dir build -L x86_backend --output-on-failure | tee test_after.log`

Regression guard:
`test_before.log` vs `test_after.log` passed with `+1` resolved test
(`c_testsuite_x86_backend_src_00130_c`) and no new failures.
