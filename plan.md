# Function Pointer Signature Type Identity Runbook

Status: Active
Source Idea: ideas/open/181_function_pointer_signature_type_identity.md

## Purpose

Function pointer identity must be carried by structured signature facts, not by
rendered type spelling, partial `TypeSpec` shape, or an `is_fn_ptr` flag.

## Goal

Audit function-pointer signature carriers and repair one bounded path so return
type, parameter type refs, variadic state, and nominal identities decide
semantic equality for that path.

## Core Rule

Preserve rendered function-pointer text for display, diagnostics, and emitted
syntax; do not let it become semantic identity when structured metadata is
available.

## Read First

- `ideas/open/181_function_pointer_signature_type_identity.md`
- Function pointer parser `Node` fields and `TypeSpec` carriers
- Sema canonical function type/signature structures
- HIR `FnPtrSig` and call inference surfaces
- LIR call and type-ref lowering surfaces
- BIR/backend indirect-call ABI metadata

## Current Scope

- Inventory function-pointer signature identity surfaces across parser, sema,
  HIR, LIR, BIR, and backend preparation.
- Select one bounded high-risk path, preferably indirect-call result/argument
  inference or function-pointer parameter passing.
- Carry structured signature identity through that selected path.
- Add focused tests for collision-prone signatures where rendered-compatible or
  same-shaped function pointer spellings differ by structured parameter or
  record identity.
- Validate with targeted frontend/HIR/LIR/backend coverage for the selected
  path.

## Non-Goals

- Do not rewrite all function-pointer parsing and lowering.
- Do not replace all `TypeSpec` carriers with canonical types.
- Do not change final emitted function-pointer syntax for cosmetic reasons.
- Do not weaken indirect-call or ABI tests.
- Do not special-case one named typedef or test fixture.

## Working Model

- Syntax carriers may retain spellings and parser shape.
- Resolved signature identity must include structured return type, ordered
  parameter type facts, variadic or unspecified state, and nominal type
  identity where records or typedef-backed entities are involved.
- Missing structured metadata in the selected path should fail closed or be
  documented as an explicit compatibility boundary.
- Text rendering remains an output concern.

## Execution Rules

- Prefer semantic signature propagation or comparison helpers over
  testcase-shaped matching.
- Keep each code-changing step behavior-scoped and prove it with a build plus
  the narrow tests relevant to the touched frontend/HIR/LIR/backend surface.
- Escalate to broader validation when the selected path crosses more than one
  lowering layer or changes shared type-comparison behavior.
- Any remaining independent type-identity concern belongs in a new
  `ideas/open/` file instead of being silently absorbed into this runbook.

## Ordered Steps

### Step 1: Inventory Function-Pointer Signature Carriers

Goal: identify where function-pointer signature identity is currently stored,
copied, compared, rendered, or lowered.

Actions:

- Inspect parser `Node` fields and `TypeSpec` function-pointer representation.
- Inspect sema canonical function or pointer-to-function signature data.
- Inspect HIR `FnPtrSig`, call inference, and argument/result typing.
- Inspect LIR calls, `LirTypeRef`, and any function-pointer signature bridge.
- Inspect BIR/backend indirect-call ABI metadata.
- Mark which surfaces already have structured facts and which still rely on
  spelling, partial shape, or flag-only identity.

Completion Check:

- `todo.md` records the selected bounded path, the identity surfaces it crosses,
  and the exact first implementation target.

### Step 2: Select and Bound One Repair Path

Goal: choose the smallest path that can demonstrate structured function-pointer
signature identity without becoming a full rewrite.

Actions:

- Prefer indirect-call result/argument inference or function-pointer parameter
  passing if the inventory confirms a spelling or partial-shape identity risk.
- Define the structured signature facts needed by the path: return type,
  parameter list, variadic/unspecified state, and nominal type identities.
- Define the compatibility behavior for missing metadata.
- Identify focused tests that can distinguish rendered-compatible or same-shaped
  function pointer spellings with different structured identities.

Completion Check:

- The selected path and test plan are narrow enough for an executor packet and
  do not require broad parser or type-system replacement.

### Step 3: Propagate Structured Signature Identity

Goal: ensure the selected path carries structured signature facts to the point
where semantic identity is decided.

Actions:

- Add or reuse structured signature fields/helpers on the selected path.
- Preserve existing rendered spelling for diagnostics and output.
- Avoid adding name-based shortcuts, fixture-specific branches, or display-text
  equality as a fallback for metadata-rich inputs.
- Ensure missing or mismatched structured metadata fails closed or reaches a
  clearly named compatibility boundary.

Completion Check:

- The selected path no longer depends on rendered signature text, partial
  `TypeSpec` shape, or `is_fn_ptr` alone for metadata-rich signature identity.
- A fresh build or compile proof covers the touched code.

### Step 4: Add Collision-Prone Coverage

Goal: prove the repaired path distinguishes structured signature identity where
spelling or shape alone would collide.

Actions:

- Add focused tests for function-pointer signatures whose rendered-compatible
  or same-shaped syntax differs by parameter, return, variadic, typedef, or
  record identity relevant to the selected path.
- Include the affected frontend/HIR/LIR/backend observation points rather than
  only printed call text.
- Keep existing supported-path tests at least as strong as before.

Completion Check:

- New or updated tests fail on the old spelling/shape identity behavior and pass
  with structured signature facts.

### Step 5: Validate and Record Boundaries

Goal: finish the slice with proof and clear remaining boundaries.

Actions:

- Run the narrow proof command chosen by the supervisor for the touched path.
- Run broader validation if shared type-comparison or multi-layer lowering
  behavior changed.
- Record in `todo.md` which function-pointer path is now structured and which
  adjacent function-pointer surfaces remain outside this runbook.

Completion Check:

- `todo.md` contains the proof result and boundary notes needed for the
  supervisor to decide whether the source idea is complete or needs a follow-up
  plan.
