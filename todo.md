Status: Active
Source Idea Path: ideas/open/846_lir_family_overloaded_verifier_dispatch_printer.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add The Bounded Family Overload

# Current Packet

## Just Finished

Completed `plan.md` Step 1 by selecting one exact generic family consumer:
`LirAbsOp.int_type` verifier/printer.

Selected evidence:
- `src/codegen/lir/verify.cpp`: `verify_inst` currently calls
  `require_module_type_ref(mod, op->int_type, "LirAbsOp.int_type")` for
  `LirAbsOp`; `verify_abs_op_authority` already requires integer semantic
  authority when native result authority is present.
- `src/codegen/lir/lir_printer.cpp`: the `LirAbs` printer currently calls
  `require_type_ref(op->int_type, "LirAbsOp.int_type")` and emits
  `@llvm.abs.<type>` generically.
- Nearby focused same-feature coverage already exists around scalar abs
  authority in `tests/frontend/frontend_lir_call_type_ref_test.cpp`, plus
  backend BIR abs receipt/rejection tests.

Required overload: a scalar integer/integer type-family type-ref requirement
for `LirAbsOp.int_type`, using native semantic family authority and ownership
checks rather than display-text parsing or rendered-printer output.

## Suggested Next

Execute `plan.md` Step 2 for the selected `LirAbsOp.int_type` surface: add the
bounded scalar integer type-ref requirement needed by the verifier/printer path,
then prepare Step 3 to migrate only the selected `LirAbsOp.int_type` generic
callsites.

Expected code-changing proof candidates:
- Fresh build for the touched LIR verifier/printer code.
- Focused frontend same-feature coverage in
  `tests/frontend/frontend_lir_call_type_ref_test.cpp` around scalar abs
  authority.
- Backend BIR abs receipt/rejection tests that prove valid receipt and
  malformed/wrong-family rejection for abs authority.
- `git diff --check`.

## Watchouts

Do not delete universal-model APIs yet; idea 847 owns terminal deletion after
846 gates are accepted. Do not parse display text, runtime text, diagnostics,
or printer output as semantic state. Do not absorb producer/store construction,
Raw-BIR receipt, 734 receiver repair, or 797 convergence.

Non-goals for the next packet: no broad verifier/printer rewrite, no generic
helper deletion unless no semantic callers remain, no dispatch migration beyond
the selected abs integer type-ref surface, no producer/carrier repair, and no
expectation downgrade or testcase-shaped special case.

## Proof

Step 1 proof is source trace and lifecycle state only. Planning-file check:
`git diff --check`.

Code-changing Step 2 through Step 4 packets need a fresh build, focused
same-feature abs authority tests, backend BIR abs receipt/rejection coverage,
and broader proof if shared verifier or printer infrastructure changes.
