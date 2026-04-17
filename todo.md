Status: Active
Source Idea: ideas/open/54_x86_backend_c_testsuite_capability_families.md
Source Plan: plan.md

# Current Packet

## Just Finished

Completed `plan.md` Step 3 for the selected local-memory `gep` lane by
extending the x86 prepared-module consumer to accept bounded local-slot
compare-against-immediate guard chains, direct local-slot addressed loads, and
the sign-extension byte lane needed by the cluster. The selected proving
cluster now moves together on the x86 backend path:
`c_testsuite_x86_backend_src_00032_c`,
`c_testsuite_x86_backend_src_00073_c`, and
`c_testsuite_x86_backend_src_00130_c` all pass, and
`tests/backend/backend_x86_handoff_boundary_test.cpp` now carries explicit
boundary fixtures for the new local compare-immediate and byte-addressed
handoff shapes.

## Suggested Next

Start `plan.md` Step 4 by running the `x86_backend` checkpoint again and
measuring the truthful pass-count effect of the completed local-memory `gep`
lane before selecting the next adjacent family packet.

## Watchouts

- Do not weaken `x86_backend` expectations to accept fallback LLVM IR.
- Do not add testcase-named shortcuts or rendered-text recognizers.
- Keep the first slice in the local-memory family with already-supported
  control flow.
- Shared `lir_to_bir` capability growth should explain the result more than any
  x86-local patching.
- The x86 handoff extension stayed in the existing bounded local-slot guard
  route; it did not reopen semantic lowering or widen into unrelated x86
  families.
- The new handoff support is still bounded to parameter-free x86_64 local-slot
  guard chains with direct local loads, addressed local-slot loads, and
  compare-against-immediate returns. Do not silently treat that as general x86
  CFG or arbitrary memory support.
- Leave `00037`, `00042`, `00046`, `00143`, and `00207` out of the next
  packet; Step 4 should measure this completed lane before widening to another
  family.

## Proof

Ran the delegated proof exactly:
`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -R '^backend_x86_handoff_boundary$' --output-on-failure >> test_after.log 2>&1 && ctest --test-dir build -R '^c_testsuite_x86_backend_src_(00032|00073|00130)_c$' --output-on-failure >> test_after.log 2>&1`.
`backend_x86_handoff_boundary` passed, and
`c_testsuite_x86_backend_src_00032_c`,
`c_testsuite_x86_backend_src_00073_c`, and
`c_testsuite_x86_backend_src_00130_c` all passed on the x86 backend path.
Proof log:
`test_after.log`.
