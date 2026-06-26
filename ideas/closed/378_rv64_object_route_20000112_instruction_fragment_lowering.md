# RV64 Object Route 20000112 Instruction Fragment Lowering

Status: Closed
Type: Repair idea
Parent: `ideas/open/354_rv64_gcc_torture_prepared_module_shape_classification.md`
Split From: `ideas/closed/369_rv64_object_route_terminator_fragment_lowering.md`

## Goal

Audit and repair the next RV64 object-route prepared instruction-fragment
blocker exposed by `src/20000112-1.c` after idea 369's terminator-fragment
repairs.

## Why This Exists

Idea 369 repaired the two in-scope terminator-fragment blockers found in the
representative reruns:

- fused `sgt i32 %reg, %reg` conditional branch lowering for
  `src/20000224-1.c`
- fused pointer-null `ne ptr %reg, 0` conditional branch lowering for
  `src/20000112-1.c`

After those repairs, the Step 5 representative rerun shows:

- `src/20000224-1.c`: passes the RV64 gcc-torture backend object route
- `src/20000112-1.c`: advances to a distinct ordinary instruction-fragment
  owner:

```text
unsupported_instruction_fragment: BIR instruction requires unsupported RV64 object lowering
```

This is no longer a terminator-fragment problem, so it needs a separate repair
contract instead of expanding idea 369.

## In Scope

- Audit the first prepared instruction fragment rejected by the RV64 object
  route for `src/20000112-1.c`.
- Improve diagnostics only as needed to identify the semantic instruction
  shape; do not claim diagnostic-only work as capability progress.
- Implement semantic RV64 object emission for the first supportable prepared
  instruction-fragment shape using prepared instruction, operand, type, value
  home, and publication facts.
- Add focused backend object-emission tests for the admitted shape and nearby
  rejected instruction-fragment shapes.
- Rerun `src/20000112-1.c` and record whether it passes or advances to a
  documented distinct next owner.

## Out of Scope

- Reopening idea 369 terminator-fragment lowering, branch-target handling, or
  CFG reconstruction.
- Reopening data sections, globals, strings, relocations, byval aggregate
  homes, aggregate `va_arg`, non-register parameter homes, or unrelated
  call-argument ownership.
- Broad assembler/object-route replacement unrelated to the audited
  instruction-fragment shape.
- Inferring behavior from testcase names, source syntax, hard-coded value ids,
  instruction indexes, block numbering accidents, or prepared-BIR text
  matching.

## Acceptance Criteria

- The first unsupported ordinary prepared instruction-fragment shape for
  `src/20000112-1.c` is documented from prepared/object-route evidence.
- Focused RV64 object-emission tests prove any admitted instruction-fragment
  lowering.
- Unsupported adjacent instruction shapes keep precise fail-closed diagnostics.
- `src/20000112-1.c` is rerun and either passes or advances to a documented
  distinct next owner because of semantic instruction-fragment repair.
- Existing focused backend object-emission and prepared-contract coverage
  remains green.

## Evidence

- `test_before.log`: rolled-forward Step 5 representative rerun from idea 369.
- `build/agent_state/369_step5_terminator_representatives.runner.log`
- `build/rv64_gcc_c_torture_backend/src_20000112-1.c/case.log`
- `build/rv64_gcc_c_torture_backend/src_20000224-1.c/case.log`

## Completion Note

Closed after the audited unsupported ordinary instruction fragment for
`src/20000112-1.c` advanced:

```text
%t8 = bir.zext i32 %t7 to i32
```

The same-width integer `ZExt` RV64 object-route lowering was implemented with
focused object-emission coverage and fail-closed adjacent cast coverage. The
representative now compiles and links far enough to run under qemu, where it
fails as a distinct runtime mismatch:

```text
RV64_BACKEND_RUNTIME_MISMATCH
clang_exit=0 c4c_exit=Segmentation fault
```

That remaining failure is outside this idea's instruction-fragment
compile-blocker scope and is handed off to
`ideas/open/379_rv64_object_route_20000112_runtime_join_publication.md`.

Close-time regression guard passed against the accepted full-suite baseline:
`3353 passed, 0 failed` before and after.

## Reviewer Reject Signals

- Reject testcase-name shortcuts for `src/20000112-1.c` or its exact source
  expression.
- Reject hard-coded value ids, frame slots, registers, instruction indexes,
  block names, or prepared-BIR text matching not derived from semantic
  prepared instruction metadata.
- Reject changes that only rename `unsupported_instruction_fragment`, weaken
  diagnostics, rewrite expectations, or change allowlists while leaving the
  same instruction-fragment shape unsupported.
- Reject broad terminator, CFG, globals, strings, aggregate ABI, byval,
  non-register parameter-home, data-section, or call-argument rewrites inside
  this idea.
- Reject unsupported-test downgrades, weaker test contracts, or
  classification-only changes claimed as RV64 object lowering capability.
