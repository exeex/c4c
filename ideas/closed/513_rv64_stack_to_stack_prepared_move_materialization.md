# RV64 Stack-To-Stack Prepared Move Materialization

Source Parent: ideas/closed/512_stack_passed_parameter_home_publication.md
Owning Layer: RV64 object emission consuming prepared move-bundle authority
Status: Closed

## Goal

Materialize generic RV64 stack-to-stack prepared move bundles when source and
target stack homes are explicitly published and coherent.

## Why This Exists

Idea 512 closed the stack-passed parameter-home contract. The remaining rows
from that bucket no longer fail at `unsupported_param_home`; they now advance
to `unsupported_move_bundle_target_shape` for stack-to-stack value moves:

- `src/20010518-1.c`
- `src/pr27073.c`
- `src/pr69447.c`

Those failures are not parameter-home publication gaps. They are a separate
RV64 move-bundle materialization problem that should consume prepared move
authority rather than being folded back into stack-parameter home publication.

## In Scope

- Reproduce and inspect the prepared move bundles for the listed rows.
- Identify the minimal stack-to-stack source and destination shapes that carry
  explicit prepared authority.
- Teach RV64 object emission to materialize those coherent stack-to-stack
  moves through an appropriate scratch or load/store sequence.
- Preserve fail-closed diagnostics for missing, ambiguous, contradictory, or
  unsupported prepared move facts.
- Add focused backend coverage for accepted stack-to-stack moves and malformed
  adjacent cases.

## Out Of Scope

- Reopening stack-passed parameter-home publication from idea 512.
- Inferring move sources or destinations from testcase names, source shapes,
  raw instruction order, argument indexes, or ABI folklore.
- Broad move-bundle support unrelated to the observed stack-to-stack value
  move family.
- Varargs, F128, aggregate ABI, dynamic stack, callee-saved FPR frame emission,
  or unrelated RV64 instruction lowering.
- Expectation, unsupported-marker, allowlist, runtime-comparison, or scan
  accounting changes claimed as capability progress.

## Acceptance Criteria

- The representative stack-to-stack rows no longer fail at
  `unsupported_move_bundle_target_shape` for the owned prepared move family.
- RV64 materialization consumes explicit prepared move-bundle, source-home, and
  destination-home facts.
- Missing or malformed prepared facts remain fail-closed with precise
  diagnostics.
- Focused backend coverage proves at least one accepted stack-to-stack
  materialization path plus adjacent reject cases.
- Validation includes a fresh build, focused representative proof, focused
  backend coverage, and the relevant backend subset.

## Closure Summary

Closed after implementing the coherent same-scalar stack-slot to stack-slot
RV64 materializer in object emission and adding focused accepted/reject
coverage.

The accepted shape is intentionally narrow: one before-instruction value move
from a prepared stack-slot source to a prepared stack-slot destination, with
same scalar type, coherent GPR frame-slot storage facts when present, and
explicit prepared homes. The materializer emits the RV64 load/store sequence
through a scratch GPR and leaves malformed, missing-authority, unsupported-bank,
multi-move, and conversion-adjacent cases fail-closed.

Step 5 reclassified the original representative rows:

- `src/20010518-1.c` still reaches `unsupported_move_bundle_target_shape`, but
  its first failing bundle is a multi-move register-source bundle targeting a
  stack value, not the coherent stack-slot to stack-slot shape.
- `src/pr27073.c` still reaches `unsupported_move_bundle_target_shape`, but
  its first failing move sources `%p.count` from register `a4` into a stack
  destination despite the reason string.
- `src/pr69447.c` still reaches `unsupported_move_bundle_target_shape`, but
  its first failing coherent-looking move has a source storage-plan entry
  `frame_slot bank=none` and is conversion-adjacent (`i16` to `i64`), so
  accepting it would infer bank or conversion semantics outside this idea.

Follow-up ideas now own those separate blockers:

- `ideas/open/514_rv64_register_source_stack_destination_move_bundles.md`
- `ideas/open/515_rv64_bankless_conversion_adjacent_stack_slot_moves.md`

Close-time validation:

- `cmake --build build --target c4cll`: passed.
- `cmake --build build --target backend_riscv_object_emission_test`: passed.
- `ctest --test-dir build -j --output-on-failure -R 'backend_riscv_object_emission|stack_to_stack|move_bundle'`: passed, 1/1.
- `ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`: passed, 345/345.
- `.codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log`: passed; before 331/331, after 345/345, no new failures.

## Reviewer Reject Signals

Reject any route or slice that:

- special-cases `20010518-1.c`, `pr27073.c`, `pr69447.c`, or their local names
  instead of admitting a semantic prepared move shape
- re-derives stack source or destination locations from source order, argument
  index, ABI formulas, or textual instruction matching
- silently drops a prepared move or weakens the diagnostic instead of
  materializing it
- changes expectations, unsupported markers, allowlists, runtime comparison,
  or pass/fail accounting without repairing materialization
- combines this work with broad varargs, F128, aggregate, dynamic-stack, or
  unrelated RV64 lowering repairs
- leaves the same `unsupported_move_bundle_target_shape` failure mode for the
  owned stack-to-stack move family behind a renamed check
