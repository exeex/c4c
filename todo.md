# X86 Backend Failure Recovery

Status: Active
Source Idea: ideas/open/45_fix_x86_backend_fails.md
Source Plan: plan.md

## Current Active Item

- accepted reset:
  the x86 `try_emit_minimal_*` / `try_emit_direct_*` helper route has been
  removed, including `src/backend/x86/codegen/direct_dispatch.cpp` and the
  sibling `direct_*.cpp` special-case emitters
- accepted reset:
  `src/backend/x86/codegen/emit.cpp` now no longer routes public x86 probing
  through deleted special-case helpers
- accepted reset proof:
  `cmake --build build -j2` passes after the special-case deletion and test
  cleanup
- current triage:
  `ctest --test-dir build -j --output-on-failure -R '^(c_testsuite_x86_backend_src_00030_c|c_testsuite_x86_backend_src_00031_c|c_testsuite_x86_backend_src_00094_c)$'`
  fails `00030.c`, `00031.c`, and `00094.c` with the same unsupported x86
  direct-LIR diagnostic, so the runbook is reset onto that shared translated
  failure mode
- completed packet:
  shared lowering now preserves trivial constant-return modules with unused
  globals, and the x86 translated path now emits trivial constant-return BIR
  modules, so `00030.c`, `00031.c`, and `00094.c` no longer fall through the
  unsupported direct-LIR diagnostic after the special-case route deletion
- completed packet proof:
  `cmake --build build -j2 && ctest --test-dir build -j --output-on-failure -R '^(c_testsuite_x86_backend_src_00030_c|c_testsuite_x86_backend_src_00031_c|c_testsuite_x86_backend_src_00094_c)$'`
  passes; proof log: `test_after.log`
- invalidated baseline:
  prior claims that `00030.c`, `00031.c`, `00040.c`, `00051.c`, `00081.c`
  through `00094.c`, or the related runtime helper families were accepted via
  x86 special-case emitters are no longer authoritative after the route reset

## Immediate Target

- reprove the next adjacent translated x86 band after the restored reset
  subset: `00040.c`, `00051.c`, and `00081.c` through `00094.c`
- keep the next repair in shared lowering or translated x86 owners
- do not recreate any testcase-shaped dispatcher or helper seam

## Done Condition For The Active Packet

- the next adjacent translated x86 reproving band is explicit in `todo.md`
- newly claimed recoveries are freshly reproved on the translated route
- the winning diffs continue to live in general lowering or translated x86
  codegen, not in a new x86 special-case route

## Parked While This Packet Is Active

- reproving the former `00040.c` / `00051.c` / `00081.c` through `00094.c`
  bands after the translated baseline is restored
- broader x86 runtime regressions beyond the reset subset
- any idea close claim for `ideas/open/45_fix_x86_backend_fails.md`
