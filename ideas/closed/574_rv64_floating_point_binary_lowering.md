# RV64 Floating-Point Binary Lowering

Status: Closed
Type: Capability repair
Parent: `ideas/open/570_rv64_unsupported_instruction_fragment_owner_diagnostics.md`
Owning Layer: RV64 object lowering for floating-point binary instructions

## Goal

Implement the RV64 object-route lowering needed for scalar floating-point BIR
binary operations, starting with the double-precision division shape exposed by
the 570 diagnostics.

## Why This Exists

The Step 3 diagnostics from the 570 runbook identified `src/20000605-1.c` as
a distinct floating-point binary owner family:

- `function=render_image_rgb_a`
- `instruction_kind=BinaryInst`
- `owner=double %t5`
- first unsupported instruction: `double %t5 = bir.sdiv double 1.0, %t4`

The first unsupported instruction appears before later floating-point truncation
or cast operations, so this idea should start with binary FP lowering rather
than cast, runtime comparison, or broad F128 quarantine work.

Evidence:

- `build/agent_state/570_unsupported_instruction_fragment_diagnostics/classification.tsv`
- `build/agent_state/570_unsupported_instruction_fragment_diagnostics/src_20000605-1.c/dump-prepared-bir.txt`
- `build/agent_state/570_unsupported_instruction_fragment_diagnostics/src_20000605-1.c/object-route.log`

## In Scope

- RV64 object emission for scalar double floating-point binary operations,
  beginning with the observed division operation.
- Value materialization and result publication for the FP binary owner.
- Focused backend tests for double FP binary lowering and unsupported FP
  binary diagnostics.
- Recording any follow-up split if the route proves the blocker is an
  instruction-selection issue rather than object emission.

## Out Of Scope

- F128 quarantine or F128 lowering.
- Floating-point casts, truncation, comparisons, libcall policy, or runtime
  mismatch work unless they become the next first unsupported owner after FP
  binary lowering is proven.
- Inline asm, call ABI, select, or pointer arithmetic work.
- Changing expected runtime output or unsupported markers.

## Acceptance Criteria

- The observed double FP binary operation no longer reaches the generic
  unsupported instruction fallback, or it fails with a narrower FP-binary
  diagnostic that names the unsupported operation and type.
- Focused tests prove the supported double FP binary shape and fail-closed
  behavior for at least one unsupported FP binary form.
- The proof does not rely on `src/20000605-1.c` filename matching.
- Any later cast/runtime failure is recorded as a separate follow-up instead
  of being mixed into this idea.

## Closure Note

Closed after RV64 prepared scalar FP binary lowering advanced the representative
route past the original `owner=double %t5` double division and the later
`owner=float %t10` float multiply `BinaryInst` owner.

Completed work:

- F64 prepared FP binary lowering through FPR homes for supported hardware
  operations.
- F64 immediate operand materialization through existing RV64 immediate loading
  plus `fmv.d.x`.
- F32/F64 type-parametric hardware `add`, `sub`, `mul`, and `div` lowering
  with focused object-emission coverage.
- Fail-closed coverage for F128 and unsupported FP remainder forms.

Representative `src/20000605-1.c` now advances to a distinct prepared
move-bundle classifier blocker:

```text
prepared_consumer_category=ambiguous_non_parallel_multi_source_stack_destination
```

That blocker is outside this FP binary source idea and has been split into
`ideas/open/579_rv64_prepared_stack_destination_move_bundle_authority.md`.

Close-time backend guard:

```sh
cmake --build --preset default &&
ctest --test-dir build -j --output-on-failure -R '^backend_'
```

Result: `346/346` backend tests passing before and after; regression guard
passed with `--allow-non-decreasing-passed`.

## Reviewer Reject Signals

- Reject a slice that routes this case into F128 work without evidence that the
  first unsupported operation is F128.
- Reject claiming FP binary capability from diagnostic-only message changes,
  expectation edits, unsupported-marker changes, or allowlist changes.
- Reject testcase-shaped matching for `render_image_rgb_a`,
  `src/20000605-1.c`, or the exact `%t5` value name.
- Reject broad floating-point rewrites that do not include focused proof for
  scalar double binary result publication.
- Reject mixing later cast, truncation, or runtime-output repairs into the same
  completion claim.
