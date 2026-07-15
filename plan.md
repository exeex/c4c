# LIR Call and Signature Type Mirror Convergence Runbook

Status: Active
Source Idea: ideas/open/761_lir_call_signature_type_mirror_convergence.md
Activated from: completed composite `LirTypeRef` model in closed idea 763;
761 is the dependency-ordered next owner of call/signature type authority.

## Purpose

Converge selected LIR call and signature paths on structured type carriers, so
rendered fragments remain output or explicit compatibility data rather than
semantic authority.

## Core Rule

When structured type refs or structured argument/signature data exist, use
them as authority. Render text only at output boundaries; do not parse or let
preformatted type fragments override structured facts.

## Read First

- `ideas/open/761_lir_call_signature_type_mirror_convergence.md`
- `src/codegen/lir/ir.hpp`
- `src/codegen/lir/call_args.hpp`
- `src/codegen/lir/call_args_ops.hpp`
- `src/codegen/lir/hir_to_lir/call/`
- `src/codegen/lir/verify.cpp`
- `src/codegen/lir/print.cpp`

## Non-Goals

- Remove every compatibility string or build a full aggregate/vector/function
  type tree.
- Parse inline-assembly templates or constraints as type authority.
- Change Raw-BIR, target lowering, MIR, or LLVM emission outside rendering
  from selected structured call/signature carriers.
- Absorb value identity, PHI/CFG, pointer authority, or module declaration
  work owned by other open ideas.

## Ordered Steps

### Step 1 - Map selected call/signature authority seams

Goal: identify one bounded first conversion path where structured call or
signature type data is complete and a legacy text mirror can be made clearly
non-authoritative.

Actions:

- inspect the named call/signature fields, their producers, verifier checks,
  printer use, and backend consumers;
- distinguish output/compatibility text from semantic input and keep raw
  fallbacks explicit;
- select the smallest supported conversion path and add a focused test showing
  misleading text cannot override the structured type facts.

Completion check: the chosen path, authoritative carrier, compatibility
boundary, and focused proof target are explicit without changing unrelated
call, switch, or inline-assembly behavior.

### Step 2 - Converge the selected typed call/signature path

Goal: produce and consume typed refs directly for the selected path, rendering
legacy text only where the boundary still requires it.

Actions:

- migrate the selected `LirCallSignature`, `LirCallArg`, or `LirCallOp` seam
  without broadening into unrelated mirrors;
- ensure structured facts win over stale text wherever both are present;
- retain an explicit fallback for incomplete/raw data and preserve LLVM
  emission behavior.

Completion check: no selected semantic consumer treats its legacy type string
as authority when the structured carrier is available.

### Step 3 - Address selector and inline-assembly boundaries

Goal: make the selected switch selector or inline-assembly ordinary-value
typing derive from structured state while preserving intentionally opaque asm
text and constraints.

Actions:

- convert only the next justified bounded seam from the Step 1 audit;
- keep asm template/constraint strings opaque and compatibility fallbacks
  explicit;
- add nearby verifier/printer/backend coverage for mismatched text versus
  authoritative typed state.

Completion check: the chosen boundary has no raw-string type authority gap and
does not parse opaque assembly text.

### Step 4 - Prove the bounded convergence

Goal: validate the selected conversions and decide whether the source criteria
are complete or require an in-scope runbook repair.

Actions:

- run fresh build plus focused LIR/frontend/backend tests for each packet;
- include cases where misleading strings are rejected or ignored when typed
  state is present;
- obtain supervisor-selected broader proof and regression guard before closure.

Completion check: focused tests demonstrate structured authority, compatibility
fallbacks remain explicit, and no contract is weakened or unrelated subsystem
is changed.
