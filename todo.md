Status: Active
Source Idea Path: ideas/open/517_residual_scalar_f32_f64_cast_object_lowering.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Add Focused Backend Coverage

# Current Packet

## Just Finished

Step 4 - Add Focused Backend Coverage completed for the explicit prepared
FPR-register-source to GPR-register-destination F32/F64-to-I32/I64 cast
boundary.

Coverage added:

- Expanded the semantic object-emission fixture for prepared FP-to-integer
  casts to cover all accepted register-home combinations:
  `FPToSI`/`FPToUI` from `F32`/`F64` to `I32`/`I64`.
- Asserted the exact emitted RTZ encodings for `fcvt.w.s`, `fcvt.wu.s`,
  `fcvt.l.s`, `fcvt.lu.s`, `fcvt.w.d`, `fcvt.wu.d`, `fcvt.l.d`, and
  `fcvt.lu.d`, each followed by `ret`.
- Added adjacent fail-closed semantic fixtures for missing source home, missing
  destination home, GPR source home for an FPR source, FPR destination home for
  a GPR destination, unsupported I16 destination width, F128 source,
  FP-to-integer-shaped bitcast, and typed F64-immediate `FPTrunc` to F32.
- Preserved the `920618-1.c` split through the typed F64-immediate `FPTrunc`
  fixture and the owner-specific `unsupported_floating_cast` diagnostic,
  without naming the gcc_torture row or inventing FP-immediate materialization.
- Recorded F128 as a fail-closed helper-family quarantine at the existing
  generic unsupported-instruction boundary rather than treating it as part of
  the F32/F64 scalar-cast diagnostic surface.

Implementation status:

- No implementation files were touched in Step 4. The focused coverage passed
  against the Step 3 behavior.

## Suggested Next

Delegate Step 5 - Validate And Handoff. Use the existing Step 3 and Step 4
proof logs to decide whether this idea is ready for supervisor/plan-owner
handoff, while keeping the residual non-parallel multi-source stack-destination
boundary separate.

## Watchouts

The accepted gcc_torture representatives still expose the separate ambiguous
multi-source stack-destination authority boundary after the scalar cast gate.
Treat that as residual work outside this scalar cast slice. Keep
`src/920618-1.c` split until a producer/prepared FP-immediate materialization
contract exists. Keep F128/helper ABI, local-memory, aggregate/byval,
stack-frame, branch/select, call/return, and `conversion.c` work out of this
route.

## Proof

Proof is captured in `test_after.log`:

- `cmake --build --preset default`
- `ctest --test-dir build -j --output-on-failure -R
  '^backend_riscv_object_emission$|^backend_'`
- `git diff --check --
  tests/backend/mir/backend_riscv_object_emission_test.cpp todo.md`
