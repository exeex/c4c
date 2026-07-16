Status: Active
Source Idea Path: ideas/open/846_lir_family_overloaded_verifier_dispatch_printer.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Decide Next 846 Packet Or 847 Handoff

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

Started the Step 2/Step 3 implementation packet locally after a stalled
executor delegation. The code scope remains limited to the selected
`LirAbsOp.int_type` verifier/printer consumer.

Completed the selected `LirAbsOp.int_type` packet:

- Added `render_integer_type_ref`, a bounded integer-family helper that checks
  native `LirTypeRef` kind/width authority and renders from structured width
  instead of reclassifying display text.
- Migrated only the `LirAbsOp.int_type` verifier and printer callsites to the
  bounded helper.
- Added focused stale-display coverage proving scalar abs verification and
  printing use native integer width authority, not mutable display text.

Completed Step 5 inventory for the next 846 packet. 846 still owns generic
family consumers, so the next selected surface is `LirSelectOp.type_str`
verifier/printer:

- `src/codegen/lir/verify.cpp` still verifies the selected field with
  `require_module_type_ref(mod, op->type_str, "LirSelectOp.type_str")`.
- `src/codegen/lir/lir_printer.cpp` still renders the selected field with
  `require_type_ref(op->type_str, "LirSelectOp.type_str")`.
- `verify_select_op_authority` already treats `op.type_str.kind() ==
  LirTypeKind::Integer` as the scalar integer authority claim and rejects a
  native scalar select result when the selected type is not integer.
- Focused scalar select coverage exists in
  `tests/frontend/frontend_lir_call_type_ref_test.cpp`, with backend BIR
  receipt/rejection coverage in `backend_lir_to_bir_interface`.

Completed the selected `LirSelectOp.type_str` packet:

- Reused `render_integer_type_ref` for the select verifier/printer type field.
- Migrated only the `LirSelectOp.type_str` verifier and printer callsites.
- Added focused stale-display coverage proving scalar select printing uses
  native integer width authority rather than mutable display text.

## Suggested Next

Execute `plan.md` Step 5: inventory remaining universal classifier, renderer,
mutable semantic string, and implicit conversion callers. Classify the next
exact 846-owned consumer or record the handoff/blocker decision if no 846
consumer is ready.

## Watchouts

Do not delete universal-model APIs yet; idea 847 owns terminal deletion after
846 gates are accepted. Do not parse display text, runtime text, diagnostics,
or printer output as semantic state. Do not absorb producer/store construction,
Raw-BIR receipt, 734 receiver repair, or 797 convergence.

Non-goals for the next packet: no broad verifier/printer rewrite, no generic
helper deletion unless no semantic callers remain, no dispatch migration beyond
one selected surface, no producer/carrier repair, and no expectation downgrade
or testcase-shaped special case. Do not migrate `LirCmpOp`, `LirBinOp`, vector,
aggregate, load/store, or cast type refs in the select packet.

## Proof

Step 1 proof is source trace and lifecycle state only. Planning-file check:
`git diff --check`.

Completed Step 2 through Step 4 proof:

- `cmake --build build`
- `ctest --test-dir build -R '^frontend_lir_call_type_ref$' --output-on-failure`
- `ctest --test-dir build -R '^backend_lir_to_bir_interface$' --output-on-failure`
- `git diff --check`

Step 5 trace selected `LirSelectOp.type_str` for the next bounded packet.
Completed select packet proof:

- `cmake --build build`
- `ctest --test-dir build -R '^frontend_lir_call_type_ref$' --output-on-failure`
- `ctest --test-dir build -R '^backend_lir_to_bir_interface$' --output-on-failure`
- `git diff --check`

Next Step 5 decision is trace/lifecycle state unless it selects and records
another bounded implementation packet.
