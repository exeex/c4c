Status: Active
Source Idea Path: ideas/open/517_residual_scalar_f32_f64_cast_object_lowering.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement Or Split The Accepted Boundary

# Current Packet

## Just Finished

Step 3 - Implement Or Split The Accepted Boundary completed for the explicit
prepared FPR-register-source to GPR-register-destination F32/F64-to-I32/I64
cast boundary.

Implemented behavior:

- Added RV64 object lowering for `FPToSI`/`FPToUI` when the prepared cast has
  an `F32` or `F64` source with a coherent FPR register home and an `I32` or
  `I64` destination with a coherent GPR register home.
- Encoded the hardware conversion family as `fcvt.{w,wu,l,lu}.{s,d}` using the
  same RISC-V floating opcode path as the existing FPR-width and int-to-FPR
  casts, with RTZ (`rm=1`) rounding for FP-to-integer truncation semantics.
- Kept unsupported or incomplete shapes fail-closed by returning no fragment
  when opcode, scalar type, source home, source bank, destination home, or
  destination bank does not match the Step 2 contract.
- Updated the existing `unsupported_floating_cast` diagnostic text, and the
  existing exact-string backend expectation, so the diagnostic describes the
  newly accepted FPR-register-source floating-to-integer boundary.
- Added focused backend object-emission assertions for `fcvt.l.d a0, fa0, rtz`
  and `fcvt.wu.s a0, fa0, rtz` so the rounding-mode field is covered before
  acceptance.

Representative outcomes before final captured proof:

- `tests/c/external/gcc_torture/src/cvt-1.c` advances past the old
  `unsupported_floating_cast` gate and now stops at
  `prepared_consumer_category=ambiguous_non_parallel_multi_source_stack_destination`.
- `tests/c/external/gcc_torture/src/pr66233.c` advances past the old
  `unsupported_floating_cast` gate and now stops at
  `prepared_consumer_category=ambiguous_non_parallel_multi_source_stack_destination`.
- `tests/c/external/gcc_torture/src/920618-1.c` remains split/fail-closed at
  `unsupported_floating_cast` with no object emitted; no FP-immediate
  materialization was invented.

## Suggested Next

Delegate Step 4 - Add Focused Backend Coverage. Add semantic object-emission
fixtures for the accepted `FPToSI`/`FPToUI` register-home boundary and
fail-closed fixtures for unsupported or incomplete prepared facts.

## Watchouts

The two accepted gcc_torture representatives no longer prove end-to-end object
emission because they now expose a separate ambiguous multi-source
stack-destination authority boundary. Treat that as residual work outside this
scalar cast slice. Keep `src/920618-1.c` split until a producer/prepared
FP-immediate materialization contract exists. Keep F128/helper ABI,
local-memory, aggregate/byval, stack-frame, branch/select, call/return, and
`conversion.c` work out of this route.

## Proof

Proof is captured in `test_after.log`:

- `cmake --build --preset default`
- focused `c4cll --codegen obj` probes for `src/cvt-1.c`, `src/pr66233.c`, and
  `src/920618-1.c`
- `ctest --test-dir build -j --output-on-failure -R '^backend_'`
- `git diff --check -- src/backend/mir/riscv/codegen/object_emission.cpp
  tests/backend/mir/backend_riscv_object_emission_test.cpp todo.md`
