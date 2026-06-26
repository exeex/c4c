# RV64 Object Route Instruction Fragment Lowering

Status: Closed
Type: Repair idea
Parent: `ideas/open/354_rv64_gcc_torture_prepared_module_shape_classification.md`
Split From: `ideas/closed/376_rv64_object_route_prepared_move_bundle_target_shapes.md`

## Goal

Audit and repair the next RV64 object-route prepared instruction-fragment
blocker exposed after prepared move-bundle target-shape support.

## Why This Exists

Idea 376 admitted the first semantic prepared move-bundle target shape needed
by `src/20000217-1.c`: scalar integer stack-slot source to single-width GPR
value destination. Its representative rerun advanced past:

```text
unsupported_move_bundle_target_shape: prepared move bundle requires unsupported RV64 moves
```

The same representative now fails at a distinct ordinary instruction-fragment
gate:

```text
unsupported_instruction_fragment: BIR instruction requires unsupported RV64 object lowering
```

Evidence:

- `build/agent_state/376_step5_20000217-1.runner.log`
- `build/rv64_gcc_c_torture_backend/src_20000217-1.c/case.log`
- `build/agent_state/376_step5_20000217-1.prepared-bir.txt`
- `build/agent_state/376_step5_20000217-1.codegen-obj.log`

The Step 5 prepared-BIR artifact shows remaining ordinary instruction
candidates in `showbug`, including integer casts, arithmetic with stack-slot
publication, local memory access, and the later compare/trunc path. The current
object diagnostic is intentionally generic, so the first packet should audit
the earliest unsupported prepared instruction before implementing a lowering
slice.

## In Scope

- Audit the first prepared instruction-fragment rejected by the RV64 object
  route after idea 376.
- Improve the diagnostic only as needed to identify the semantic instruction
  shape, without claiming diagnostic-only changes as capability progress.
- Implement focused RV64 object emission for the first supportable ordinary
  prepared instruction shape, using prepared instruction, type, operand, and
  home metadata.
- Add focused backend tests for the admitted instruction-fragment shape and
  nearby rejected shapes.
- Rerun `src/20000217-1.c` and record whether it passes or advances to a
  distinct next owner.

## Out of Scope

- Reopening scalar compare/trunc lowering from idea 375.
- Reopening prepared move-bundle target-shape support from idea 376.
- Broad terminator lowering, CFG reconstruction, globals, strings, data
  sections, aggregate `va_arg`, byval aggregate homes, non-register parameter
  homes, or call-argument ownership.
- Implementing unrelated local-memory, publication, or return-ABI behavior
  unless the first audited instruction-fragment shape directly requires it.
- Inferring lowering behavior from testcase names, hard-coded value ids,
  source syntax, instruction indexes, or the exact `src/20000217-1.c`
  expression.

## Acceptance Criteria

- The first unsupported ordinary prepared instruction-fragment shape is
  documented from prepared/object-route evidence.
- Focused RV64 object-emission tests prove any admitted instruction-fragment
  lowering.
- Unsupported adjacent instruction shapes keep precise fail-closed diagnostics.
- `src/20000217-1.c` is rerun and either passes or advances to a documented
  distinct next owner because of semantic instruction-fragment repair.
- Existing focused backend object-emission and prepared-contract coverage
  remains green.

## Completion Note

Closed after the first unsupported instruction fragment was audited as
`main`'s same-module `bir.call i16 showbug(ptr %lv.x, ptr %lv.y)` with scalar
integer ABI-register result publication from `a0` to a stack-slot home. The
route now admits the focused scalar integer stack-slot call-result publication
shape, keeps adjacent unsupported shapes fail-closed, and the single
`src/20000217-1.c` RV64 gcc-torture backend object representative passes.

## Reviewer Reject Signals

- Reject testcase-name shortcuts for `src/20000217-1.c` or its exact source
  expression.
- Reject hard-coded value ids, frame slots, registers, instruction indexes, or
  prepared-BIR text matching not derived from semantic prepared instruction
  metadata.
- Reject changes that only rename `unsupported_instruction_fragment`, weaken
  diagnostics, rewrite expectations, or change allowlists while leaving the
  same instruction-fragment shape unsupported.
- Reject broad terminator, CFG, globals, strings, aggregate ABI, byval,
  non-register parameter-home, or data-section rewrites inside this idea.
- Reject reopening idea 375 compare/trunc or idea 376 move-bundle target-shape
  ownership as claimed progress for this instruction-fragment route.
- Reject unsupported-test downgrades, weaker test contracts, or
  classification-only changes claimed as RV64 object lowering capability.
