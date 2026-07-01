# Stack-Passed Parameter-Home Publication Runbook

Status: Active
Source Idea: ideas/open/512_stack_passed_parameter_home_publication.md

## Purpose

Publish explicit prepared ABI homes for stack-passed parameters so RV64 object
emission consumes producer authority instead of reconstructing call-frame
storage.

## Goal

Teach the producer/prealloc contract to expose stack-passed parameter storage
kind, offsets, widths, alignments, and caller/callee agreement for ordinary-C
calls beyond the register argument set.

## Core Rule

Publish stack-passed parameter homes before RV64 consumes them. Do not infer
parameter-home offsets from argument indexes, ABI folklore, testcase shape, or
source parameter names in RV64.

## Read First

- ideas/open/512_stack_passed_parameter_home_publication.md
- docs/rv64_gcc_torture_post_contract/infrastructure_bucket_evidence.md
- docs/rv64_gcc_torture_post_contract/global_stack_frame_infrastructure_review.md
- build/agent_state/424_step2_infrastructure_classification/20001017-1/
- src/backend/prealloc/
- src/backend/mir/riscv/codegen/object_emission.cpp
- tests/backend/

## Current Targets

- Primary row: `src/20001017-1.c`
- Remaining rows: `src/20010518-1.c`, `src/pr27073.c`, `src/pr69447.c`
- Failure bucket: `unsupported_param_home`
- Owning layer: producer ABI/prealloc contract
- Known missing authority:
  - callee homes for `%p.B`, `%p.fdB`, `%p.b`, `%p.C`, and `%p.fdC` are
    `kind=none` or `encoding=none`
  - caller stack arguments 8-12 have stack-slot bindings without destination
    offsets or register/bank homes
  - RV64 lacks explicit stack-passed parameter offsets, widths, alignment, and
    storage-kind authority

## Non-Goals

- Do not infer stack argument homes in RV64 from argument index, ABI formulas,
  source call shape, parameter names, or the representative gcc_torture row.
- Do not combine this producer publication with broad RV64 call lowering,
  varargs, F128, aggregate ABI, dynamic stack work, or unrelated ABI repairs.
- Do not weaken unsupported markers, expected outputs, allowlists, runtime
  comparison, scan accounting, or default test contracts.
- Do not claim printer-only output, diagnostics renames, or classification
  edits as parameter-home publication progress.

## Working Model

- The call ABI/prealloc producer should own stack-passed parameter homes for
  ordinary-C calls that exceed the register argument set.
- Prepared facts must expose storage kind, destination stack offset, width,
  alignment, and enough caller/callee contract identity for RV64 object
  emission to consume the homes directly.
- RV64 should continue rejecting missing, ambiguous, or offsetless homes with a
  diagnostic tied to `unsupported_param_home` instead of synthesizing them.
- Nearby ABI feature families such as varargs, F128, aggregate passing, and
  dynamic stack shapes should remain separate follow-up work unless the source
  idea explicitly admits them.

## Execution Rules

- Start from evidence: rebuild the representative dumps and confirm the
  current producer facts before editing code.
- Keep implementation centered in producer/prealloc ABI publication. Touch RV64
  only as needed to consume the already-published contract or preserve
  fail-closed diagnostics.
- Add focused ordinary-C coverage for stack-passed arguments that proves the
  prepared contract through dumps, object codegen, and runtime or backend
  behavior where appropriate.
- For code-changing steps, run a build, focused representative proof, focused
  stack-parameter coverage, and an appropriate backend subset.

## Step 1: Rebuild Stack-Parameter Evidence

Goal: Reconfirm the representative failure and the missing caller/callee
parameter-home facts before editing code.

Actions:

- Inspect the idea 424 handoff docs and the
  `build/agent_state/424_step2_infrastructure_classification/20001017-1/`
  artifacts.
- Run or reuse focused dumps for `src/20001017-1.c` as needed to show the
  current prepared call/parameter facts and RV64 rejection.
- Record caller stack arguments, callee parameter homes, storage kinds,
  destination offsets, widths, alignments, and the exact rejection point in
  `todo.md`.
- Check whether current `HEAD` already contains partial stack-parameter-home
  publication that must be preserved.

Completion check:

- `todo.md` identifies the missing prepared parameter-home facts, the current
  unsupported diagnostic, the producer path that should publish homes, and any
  existing partial support.

## Step 2: Trace Producer ABI Home Publication

Goal: Locate the minimal producer/prealloc path that should publish
stack-passed parameter homes for ordinary-C calls.

Actions:

- Trace call-argument and parameter-home facts from frontend/lowering through
  prealloc preparation into the records consumed by RV64.
- Identify where register homes are already published and where stack homes
  lose storage kind, destination offset, width, alignment, or caller/callee
  agreement.
- Identify fail-closed variants that must remain rejected, including missing
  offsets, ambiguous homes, unsupported varargs, F128, aggregate ABI, dynamic
  stack cases, and unrelated call shapes.
- Record intended helper boundaries and blast radius in `todo.md`.

Completion check:

- `todo.md` names the producer entry point, the prepared fact path, the
  helpers to add or adjust, and unsupported variants that must stay
  fail-closed.

## Step 3: Publish Stack-Passed Parameter Homes

Goal: Add producer ABI/prealloc publication for coherent stack-passed
parameter homes without moving inference into RV64.

Actions:

- Publish explicit prepared homes for stack-passed caller arguments and callee
  parameters when the ordinary-C ABI contract is known.
- Include storage kind, destination stack offset, width, alignment, and
  caller/callee agreement in the prepared records.
- Keep missing or ambiguous stack-home authority fail-closed with precise
  diagnostics.
- Add focused producer/prepared contract tests that prove homes are present
  and malformed variants reject.

Completion check:

- `cmake --build build --target c4cll` passes.
- Focused tests prove stack-passed homes are published from producer authority
  and missing or ambiguous homes still reject.
- `todo.md` records the final publication boundary.

## Step 4: Publish Ordinary-C ABI Metadata For Stack Parameters

Goal: Publish the missing ordinary-C call/formal ABI metadata needed by the
representative route before RV64 consumption.

Actions:

- Trace why `src/20001017-1.c` reaches producer/prealloc without explicit
  ordinary stack call-argument `passed_on_stack` offsets and without fixed
  formal `param.abi->passed_on_stack` metadata.
- Publish that metadata in the frontend/lowering/prealloc producer path that
  owns ordinary-C ABI facts; do not recreate the withdrawn index/name fallback.
- Keep byval, sret, memory aggregate, varargs, F128, dynamic stack, and
  ambiguous-formal paths fail-closed unless they already carry explicit
  authority.
- Prove `src/20001017-1.c` now exposes caller stack offsets and callee
  stack-slot homes for the ordinary stack-passed parameters in prepared dumps.
- Preserve existing explicit-ABI synthetic coverage from Step 3.

Completion check:

- `cmake --build build --target c4cll` passes.
- Focused dumps show the representative row now has explicit caller stack
  offsets and fixed-formal stack homes from producer ABI metadata.
- Focused tests prove the producer metadata path and fail-closed variants.
- `todo.md` records the producer boundary and any still-missing ABI authority.

## Step 5: Consume Published Homes Through RV64

Goal: Confirm RV64 object emission consumes the prepared homes directly and
advances the representative ordinary-C route.

Actions:

- Route RV64 consumption through the prepared home records and preserve the
  rejection path for unsupported or incomplete homes.
- Verify RV64 does not derive offsets from argument index, parameter name, or
  source call shape.
- Prove `src/20001017-1.c` or a focused equivalent advances past the old
  `unsupported_param_home` rejection when producer facts are coherent.
- Record emitted call-frame and callee-home shapes in `todo.md`.

Completion check:

- Focused representative proof advances past the old parameter-home rejection.
- Focused backend tests show RV64 consumes prepared stack homes without
  reconstructing them.
- `git diff --check` passes for touched files.

## Step 6: Reclassify Remaining Param-Home Rows

Goal: Check the remaining bucket rows and separate covered ordinary-C cases
from narrower follow-up features.

Actions:

- Probe `src/20010518-1.c`, `src/pr27073.c`, and `src/pr69447.c`.
- Classify each as consuming the new contract, exposing a narrower producer
  gap, or staying fail-closed for an explicitly out-of-scope ABI feature.
- Add or update focused ordinary-C backend coverage when a row exposes a
  supported same-contract shape.
- Record any required follow-up idea candidates in `todo.md` rather than
  expanding this plan.

Completion check:

- `todo.md` contains the row-by-row reclassification and any follow-up
  recommendation.
- Focused coverage covers at least one stack-passed parameter path beyond a
  named-case-only proof.

## Step 7: Validate And Handoff

Goal: Prove the slice and prepare the source idea for closure or a narrowed
follow-up if unsupported ABI shapes remain.

Actions:

- Run the build, focused representative proof, focused stack-parameter
  coverage, and `ctest --test-dir build -j --output-on-failure -R "^backend_"`.
- Confirm no expectation, unsupported-marker, allowlist, runtime-comparison, or
  scan-accounting downgrade was used.
- Confirm RV64 does not reconstruct stack homes from argument indexes or ABI
  formulas.
- Record remaining non-goal failures such as varargs, F128, aggregate ABI,
  dynamic stack work, or unrelated RV64 consumer gaps in `todo.md`.

Completion check:

- Backend validation passes.
- `todo.md` contains the final proof summary and closure or follow-up
  recommendation.
