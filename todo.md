Status: Active
Source Idea Path: ideas/open/428_rv64_call_adjacent_scalar_inline_asm_materialization.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Lower Prepared Scalar Call Results

# Current Packet

## Just Finished

Step 1 classified the representative call-adjacent rows and selected the first
coherent scalar implementation packet without changing implementation files.

Chosen first implementation target: prepared scalar GPR call result
publication after direct calls, starting with register-source to
register-destination results whose prepared call plan has coherent
`result_bank=gpr`, `source_storage=register`, `destination_storage=register`,
ABI source register `a0`, a prepared destination value id/home, and no
aggregate/F128 payload.

Primary owned files for that packet:

- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `src/backend/mir/riscv/codegen/prepared_call_emit.cpp`
- `tests/backend/mir/backend_riscv_object_emission_test.cpp`

Representative accepted evidence class:

- `src/pr40657.c`: ordinary same-module/direct call results such as
  `foo()`, `abort()`, and `exit()` carry `result value_bank=gpr
  source_storage=register destination_storage=register ... source_reg=a0 ...
  dest_reg=t0`, matching the missing GPR register-result publication route.
- `src/pr45695.c`: calls to `f(...)` carry coherent scalar GPR result facts
  from `a0` to `s1`; this row also has a separate frame-slot scalar argument
  issue, so it is representative for the result class but not a one-row proof.
- `src/pr56982.c`: `_setjmp`, `g`, and `main -> f` carry coherent scalar GPR
  result facts, with symbol/immediate arguments and preservation kept as
  adjacent evidence rather than first-packet scope.

Representative rejected/deferred evidence classes:

- `src/pr38533.c`: inline asm rows expose repeated
  `missing_operand0_home` prepared-carrier facts, so accepting them would
  require missing prepared inline-asm metadata rather than RV64 scalar result
  publication.
- `src/pr45695.c`: inline asm has
  `missing fact=inst#1:tied_input_output_home_mismatch`; the frame-slot scalar
  argument facts with `missing_frame_slot_arg_publication=yes` are a later
  scalar call-argument packet.
- `src/pr49279.c`: pointer-return inline asm also has
  `tied_input_output_home_mismatch` and pointer/address-adjacent memory work,
  so it is out of the first scalar call-result packet.
- `src/pr40657.c`, `src/pr49279.c`, and `src/pr56982.c` contain frame-slot
  address or symbol-address call arguments; keep those out unless already
  represented as coherent scalar result facts.

## Suggested Next

Execute Step 2: implement prepared scalar GPR call result publication for
register-source to register-destination results in the object route, with
focused backend tests for coherent GPR results and fail-closed tests for
missing destination homes, non-GPR banks, aggregate/pointer-address payloads,
and F128.

## Watchouts

- Do not implement inline-asm carrier repair in the first packet; rows with
  `missing_operand0_home` or `tied_input_output_home_mismatch` are not coherent
  prepared inline-asm facts.
- Keep frame-slot scalar call-argument publication, frame-slot address
  arguments, symbol-address arguments, and prior-preservation argument routing
  out of the first scalar result packet unless the supervisor explicitly
  chooses Step 3 instead.
- Keep aggregate `sret`/`byval`, F128 helper-call behavior, missing prepared
  metadata reconstruction, pointer/address publication, and
  expectation/accounting changes out of this plan.
- Do not key the route to representative filenames such as `src/pr38533.c`.

## Proof

Step 1 proof is `git diff --check`; no implementation proof was run because
this packet changed only `todo.md`.

Later implementation proof command to use exactly:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1
```
