Status: Active
Source Idea Path: ideas/open/02_aarch64_calls_emission_consolidation.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Rehome Non-Emission Call Spelling or Effects

# Current Packet

## Just Finished

Step 4 in `plan.md` audited `calls_printing.cpp` call-boundary printing and
effect ownership. A small non-emission ownership move was completed:
`effects_from_prepared_call_clobbers()` and its prepared-clobber conversion
helper moved from `calls_printing.cpp` to the shared AArch64 instruction/effect
layer in `instruction.cpp`, with the public declaration moved from `calls.hpp`
to `instruction.hpp`.

This keeps printable call spelling in `calls_printing.cpp` while putting the
machine-effect construction used by normal calls plus f128/i128 runtime helper
boundaries with the rest of target MIR effect construction.

## Suggested Next

Supervisor should review the Step 4 slice and decide whether the active plan
needs another narrow call-boundary ownership packet or plan-owner lifecycle
handling.

## Watchouts

- The moved helper intentionally covers only clobber effects. Preserved-value
  publication effects remain local to `calls_printing.cpp` because they are only
  used by `make_call_instruction()` and remain gated by
  `publish_prepared_call_preserve_effects()`.
- AST-backed checks confirmed the moved public symbol now resolves in
  `instruction.cpp`; cross-translation-unit callers remain the normal call,
  f128 helper, and i128 helper record builders.
- No tests or expectations were weakened.

## Proof

`bash -lc '{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(backend_aarch64_machine_printer|backend_aarch64_call_boundary_owner|backend_call_boundary_effect_plan|backend_x86_call_boundary_effect_ordering)$"; } > test_after.log 2>&1'`

Delegated proof passed: 4/4 selected tests passed after the build.
`git diff --check` passed.
Proof log path: `test_after.log`.
