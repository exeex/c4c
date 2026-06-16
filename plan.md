# Post-286 Prepared Boundary Interface Cleanup Runbook

Status: Active
Source Idea: ideas/open/287_post_286_prepared_boundary_interface_cleanup.md

## Purpose

Convert the post-286 interface smells into a bounded cleanup route for prepared
boundary ownership.

## Goal

Produce a concrete boundary map for call argument ABI suffix facts, AArch64
variadic HFA lane expansion, and opaque pointer byte-offset provenance, then
implement at most one narrow cleanup packet if the audit proves it is safe.

## Core Rule

Do not reopen the completed 286 stdarg capability work or broaden into a general
AArch64 ABI rewrite. This runbook is about ownership boundaries, structured
carriers, and fail-closed prepared behavior.

## Read First

- `ideas/open/287_post_286_prepared_boundary_interface_cleanup.md`
- Existing post-286 code around `LirCallArg`, `LirTypeRef`, `CallArgAbi`,
  semantic call lowering, prepared call plans, aggregate aliases, local
  aggregate slots, and opaque pointer provenance.
- The 286 proof subset named in the source idea before accepting any code
  change as complete.

## Current Targets

- Call argument type identity and ABI suffixes such as `alignstack`.
- Byval/carrier array layout and struct identity crossing the LIR/BIR/prepared
  boundary.
- AArch64 variadic HFA carrier-array lane metadata and expansion ownership.
- Opaque pointer provenance and byte-offset admission for prepared
  `memory_accesses`.
- `strip_call_arg_abi_type_suffix` as a concrete string-carrier smell to remove
  or quarantine only when a structured owner exists.

## Non-Goals

- Do not rewrite AArch64 call ABI classification, variadic entry planning, or
  prepared call plans wholesale.
- Do not migrate retained prepared/prealloc policy surfaces into BIR merely
  because 286 now passes.
- Do not delete public prepared lookup surfaces without a matching route/BIR
  agreement and fail-closed proof.
- Do not claim progress through expectation-only edits, helper renames, or
  code motion that leaves the same string-carrier or target-policy coupling.
- Do not weaken unsupported, mismatch, prepared-only, or target-policy
  fail-closed behavior.

## Working Model

- Treat rendered call argument strings as legacy compatibility, not as the
  desired owner for ABI suffix facts.
- Prefer a structured `LirTypeRef`, `LirCallArg`, or adjacent call-argument
  metadata carrier for facts such as `alignstack`, byval/carrier shape, and
  struct identity.
- Treat AArch64 variadic HFA carrier-array lane expansion as target ABI policy
  unless the audit proves semantic BIR can own it without target coupling.
- Treat opaque pointer byte-offset admission as unsafe to broaden without a
  structured base-object extent or byte-range contract.
- Generate follow-up ideas rather than silently expanding this runbook when a
  required cleanup is too large for one narrow packet.

## Execution Rules

- Keep each step behavior-preserving until a specific code packet is selected.
- Prefer audit notes and boundary decisions in `todo.md` during execution; edit
  this runbook only for real route changes.
- If implementing code, choose only one self-contained cleanup packet and prove
  the 286 subset stays green.
- If no safe code packet exists, close the runbook by generating follow-up
  ideas with ownership, files, proof commands, and reviewer reject signals.
- Any code packet must include focused build or test proof and must not rely on
  expectation rewrites as evidence of capability progress.

## Step 1: Inventory Post-286 Carriers and Bridges

Goal: Build a concrete map of the facts that currently cross the LIR,
semantic BIR, prepared/prealloc, and target ABI boundaries.

Primary Targets:

- `LirCallArg`
- `LirTypeRef`
- call argument rendered strings
- aggregate aliases and local aggregate slots
- `CallArgAbi`
- prepared call plans
- opaque pointer provenance

Actions:

- Inspect the call argument path from LIR into semantic BIR and prepared
  handoff.
- Identify where rendered text carries ABI suffixes such as `alignstack`.
- Locate the AArch64 variadic HFA carrier-array lane expansion logic and the
  data it consumes.
- Locate the opaque pointer byte-offset admission rule, including the
  `allow_opaque_ptr_base && stored_type == I8` policy.
- Record findings in `todo.md` with file/function references.

Completion Check:

- `todo.md` contains a boundary inventory for all three surfaces: call
  argument type/ABI suffixes, AArch64 variadic HFA lane expansion, and opaque
  pointer byte-offset provenance.

## Step 2: Classify Each Interface Smell

Goal: Decide whether each observed smell is a safe local cleanup, needs a
structured carrier first, should remain documented prepared/target policy, or
requires a separate follow-up idea.

Primary Targets:

- The Step 1 inventory.
- The source idea's in-scope and out-of-scope boundaries.

Actions:

- Classify call argument rendered-string suffix use.
- Classify AArch64 HFA lane expansion ownership.
- Classify opaque pointer byte-offset provenance.
- Reject classifications that merely rename helpers or hide the same smell
  behind a new facade.

Completion Check:

- `todo.md` records a classification for each of the three surfaces and names
  any candidate narrow cleanup packet.

## Step 3: Decide Call Argument ABI Suffix Ownership

Goal: Draft the preferred owner for call argument ABI suffix facts and identify
the smallest safe route away from semantic string parsing.

Primary Targets:

- `LirCallArg`
- `LirTypeRef`
- adjacent call-argument metadata carriers, if present or justified
- `strip_call_arg_abi_type_suffix`

Actions:

- Decide which layer stores `alignstack`, byval/carrier shape, and struct
  identity.
- Decide which layer may continue consuming rendered text as legacy
  compatibility.
- If a narrow cleanup is safe, define the exact implementation packet and proof
  subset.
- If a structured carrier is needed first, draft a follow-up idea instead of
  forcing the cleanup into this runbook.

Completion Check:

- `todo.md` contains a call-argument boundary decision and either a bounded code
  packet or follow-up idea text.

## Step 4: Decide AArch64 Variadic HFA Lane Expansion Boundary

Goal: Determine whether semantic BIR should keep assembling HFA lane records or
delegate to a narrower target ABI helper with structured input/output.

Primary Targets:

- Semantic call lowering.
- AArch64 variadic HFA carrier-array handling.
- Local aggregate slot and alias metadata used by lane expansion.

Actions:

- Identify target-specific assumptions in current lane expansion.
- Decide whether a target ABI helper should return lane values or ABI records.
- Preserve the existing 286 behavior unless a narrow helper extraction is
  demonstrably behavior-preserving and well proven.
- Generate a follow-up idea if the helper boundary is too large for this
  cleanup.

Completion Check:

- `todo.md` contains the HFA lane expansion boundary decision and any helper
  extraction or follow-up route.

## Step 5: Decide Opaque Pointer Byte-Offset Provenance Boundary

Goal: Determine whether prepared `memory_accesses` cleanup needs an explicit
base-object extent or byte-range carrier before further demotion can proceed.

Primary Targets:

- Opaque pointer provenance policy.
- Byte-offset admission logic.
- Prepared `memory_accesses` fail-closed behavior.

Actions:

- Inspect the current `allow_opaque_ptr_base && stored_type == I8` rule.
- Decide whether it is only a compatibility rule or a valid semantic carrier.
- Identify what structured extent/range facts would be needed to replace or
  narrow it.
- Generate a follow-up idea if the carrier work is separate from this runbook.

Completion Check:

- `todo.md` contains an opaque pointer provenance boundary decision and any
  required follow-up idea text.

## Step 6: Execute One Narrow Cleanup or Generate Follow-Ups

Goal: Finish the runbook without broadening scope.

Primary Targets:

- The single cleanup packet selected by Steps 2 through 5, if any.
- Otherwise, the follow-up ideas required by the boundary decisions.

Actions:

- If a self-contained cleanup exists, implement only that packet.
- Keep the change focused on removing or quarantining one real string-carrier
  or policy-coupling path.
- Run focused proof for the touched area plus the existing 286 proof subset:
  `backend_lir_to_bir_notes`,
  `backend_cli_dump_bir_00204_stdarg_movi_zext_immediate_fold`, and
  `backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication`.
- If no safe code change exists, create follow-up ideas with clear ownership,
  files, proof commands, and reviewer reject signals.

Completion Check:

- The closure note or final `todo.md` state contains a concrete boundary map
  for all three exposed surfaces.
- Any code change has fresh focused proof and does not downgrade expectations.
- Any deferred implementation is represented by follow-up ideas rather than
  hidden inside this runbook.
