Status: Active
Source Idea: ideas/open/54_x86_backend_c_testsuite_capability_families.md
Source Plan: plan.md

# Current Packet

## Just Finished

Completed `plan.md` Steps 2-5 for one bounded `i16` local-slot semantic lane:
added honest BIR `i16` support through local-memory lowering, admitted bounded
`i16`/`i64` stack-slot layout in the x86 prepared-module path, and taught the
x86 emitter the matching local-slot update/return families proved by
`backend_x86_handoff_boundary`. The slice moves
`c_testsuite_x86_backend_src_00086_c` and
`c_testsuite_x86_backend_src_00111_c` onto the x86 backend path. The
`x86_backend` checkpoint improved from `58/220` passed to `60/220` passed.

## Suggested Next

Keep the route on the same width-focused semantic family and probe the next
adjacent `i16` local-slot neighbors before broadening into generic
scalar-cast or control-flow work. The next honest packet should either widen
the newly admitted `short` stack-slot lane to one nearby same-family cluster
or stop and record that the remaining `i16` failures actually sit in a
different initiative.

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
- This packet admits bounded `i16` local-slot update/return shapes, not
  arbitrary `i16` arithmetic or generic multi-slot width support. Keep
  loop-heavy, runtime-heavy, global-heavy, wide scalar-cast, and
  multi-function prepared-module work out of this lane unless the next packet
  names that route explicitly.

## Proof

Baseline capture before implementation:
`cmake --build --preset default && ctest --test-dir build --output-on-failure -R '^(backend_lir_to_bir_notes|backend_x86_handoff_boundary|c_testsuite_x86_backend_src_00086_c|c_testsuite_x86_backend_src_00111_c)$' | tee test_before.log`

Narrow proof during the packet:
`cmake --build --preset default && ctest --test-dir build --output-on-failure -R '^(backend_lir_to_bir_notes|backend_x86_handoff_boundary|c_testsuite_x86_backend_src_00086_c|c_testsuite_x86_backend_src_00111_c)$' | tee test_after.log`

Checkpoint proof:
`ctest --test-dir build -L x86_backend`

Regression guard:
`test_before.log` vs `test_after.log` passed with `+2` resolved tests
(`c_testsuite_x86_backend_src_00086_c` and
`c_testsuite_x86_backend_src_00111_c`) and no new failures on the delegated
subset. The broader `x86_backend` checkpoint moved from `58/220` to `60/220`
passed.
