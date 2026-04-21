# Execution State

Status: Active
Source Idea Path: ideas/open/61_call_bundle_and_multi_function_prepared_module_consumption.md
Source Plan Path: plan.md
Current Step ID: 2.1
Current Step Title: Repair The Selected Prepared-Module Or Call-Bundle Seam
Plan Review Counter: 4 / 10
# Current Packet

## Just Finished

Plan Step 2.1 inspected `tests/c/external/c-testsuite/src/00204.c` with
backend-route reductions and isolated the next remaining prepared-function
rejector after the repaired `fa3` / `fa4` helper lane. A reduced prefix ending
at line 40 still emits on the x86 asm route, but adding line 41 flips the same
`minimal i32 return function` prepared-module failure. The first remaining
suspect is `fa_s1(struct s1 a)`, and the failure persists when that function's
body is reduced to `{}`, so the seam is prepared-function emission for by-value
small-aggregate parameters rather than the inner `printf` helper call or any
later `ret` / `stdarg` work.

## Suggested Next

Take the next Step 2.1 packet on prepared-function emission for by-value
aggregate parameter ingress, starting with the reduced `fa_s1(struct s1 a)`
case and then checking whether the same seam covers the adjacent `fa_s*`
family. Keep the packet out of `ret` / `myprintf` / `va_arg` until this
earlier function-signature rejector is repaired.

## Watchouts

- Do not reopen the repaired mixed `>6` helper direct-extern lane from the
  `fa3` / `fa4` packet. The new rejector reproduces before `arg()`, `ret()`,
  or `stdarg()` matter.
- The key route evidence is source-order and body-insensitive: `head -n 40`
  plus a trivial `main` emits on the x86 asm route, `head -n 41` fails with
  the canonical prepared-module fallback, and an empty-body `fa_s1(struct s1
  a) {}` still fails while the same reduced file emits through `--codegen
  llvm`.
- Treat the next seam as prepared-function signature/ingress handling for
  small by-value aggregates, not as another helper-call register-placement
  bug. Avoid speculative `printf`, `va_arg`, or return-path patching until
  this earlier rejector is repaired.

## Proof

Ran the delegated proof command:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_x86_handoff_boundary|c_testsuite_x86_backend_src_00204_c)$'; } > test_after.log 2>&1`.
Current proof matches the final workspace state: `backend_x86_handoff_boundary`
passes, while `c_testsuite_x86_backend_src_00204_c` still fails with the same
`minimal i32 return function` diagnostic. The proving log remains
`test_after.log`. Route observation for this inspection packet used
`./build/c4cll --codegen asm --target x86_64-unknown-linux-gnu ...` reductions
plus one `--codegen llvm` comparison on the reduced `fa_s1` case to confirm the
next rejector sits at `fa_s1`'s prepared aggregate-parameter seam.
