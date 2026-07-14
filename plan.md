# LIR-To-New-BIR Container And Import Completeness Runbook

Status: Active
Source Idea: ideas/open/734_lir_to_new_bir_container_completeness.md
Resumed from: closed idea 747 direct-branch successor handoff (`cebc0a3bf`)

## Purpose

Resume the bounded target-independent Raw-BIR receiver route without repeating
the accepted Steps 5.2 through 6.2. Receive exactly one direct unconditional
branch using the producer-published structural successor identity.

## Goal

Import each structured-authority LIR fact into one verified Raw-BIR module
without loss or partial publication. Never recover a fact from presentation.

## Core Rule

Every admitted row maps existing typed LIR authority directly to a typed
Raw-BIR container, importer path, verifier rule, and transactional proof.
`LirBr.successor` is the sole direct-edge authority; `target_label` is display
only.

## Read First

- `ideas/open/734_lir_to_new_bir_container_completeness.md`
- `ideas/closed/747_lir_direct_branch_successor_identity_publication.md`
- `docs/lir_structured_identity/handoff_to_734.md`
- Raw-BIR terminator builders, views, verifier, and LIR importer

## Landed Progress

- Steps 1 through 3: coverage foundation plus typed module/type/value, global,
  string, extern, symbol, initializer, specialization, intrinsic, and direct
  scalar return-signature receipt.
- Steps 4.1 through 4.5: selected-global Store/Load/GEP/Return, authorized
  parameter signatures, linkage/elision metadata, and direct void Call receipt.
- Steps 5.2 through 5.3.5: selected direct call and ordinary-value receiver
  rows through normalized i32 `Mul` (`ea4b63135`).
- Steps 6.1 and 6.2: selected i32 and i64 output-only inline-assembly binding
  and Store rows (`ad82d1456`, `37014f013`). Do not repeat these rows.
- Closed idea 747 published and verified the direct successor carrier in
  `cebc0a3bf`; its focused producer proof passed.

## Non-Goals

- no LIR redesign, label-text recovery, target interpretation, ABI placement,
  canonicalization, allocation, MIR, emission, assembler work, or legacy-BIR
  revival
- no conditional, switch, indirect, phi, local/body-parameter, or unselected
  inline-assembly receipt

## Execution Rules

1. Implement exactly one handoff row or explicitly shared typed seam per packet.
2. Add container, importer, reachable Raw-BIR verification, and transactional
   positive/negative proof together.
3. Resolve the direct destination only through `LirBr.successor`; names and
   rendering are diagnostics only after structured authority exists.
4. Preserve full-module rollback for every malformed or unsupported form.
5. Record a separate producer initiative for any required authority gap.

## Ordered Steps

### Step 6.3 - Receive the checked direct `LirBr` successor

Goal: receive the exact idea-747 handoff row as one typed Raw-BIR direct jump
without widening CFG receipt.

Primary targets:

- typed Raw-BIR direct-jump destination/terminator builder and view
- reachable Raw-BIR verifier and LIR-to-Raw-BIR terminator dispatch
- focused backend receiver coverage plus the producer identity regression

Actions:

- map only a direct unconditional `LirBr` whose `successor` is a valid,
  current-function `LirBlockId` to one typed Raw-BIR direct-jump destination;
  do not parse, look up, or repair the destination using `target_label`
- preserve exact same-function destination ownership and block identity through
  importer and reachable Raw-BIR verification; reject missing, invalid,
  duplicate/cross-owner, or incoherent successor authority with no partial
  module publication
- prove a positive direct jump plus neighboring malformed successor,
  ownership, destination, and rollback cases. Keep conditional, switch,
  indirect, phi, and all presentation-derived CFG forms fail-closed

Completion check:

- a fresh build and supervisor-selected focused backend receiver proof, with
  `frontend_lir_call_type_ref` as the producer regression neighbor, show one
  transactional typed direct jump using `LirBlockId` only.

### Step 7 - Integrate the dispatcher, verifier and build boundary

Goal: prove landed families form one production importer with no partial state.

Completion check:

- broader proof passes and every accepted row has typed authority and receipt.

### Step 8 - Prove lossless completeness and transactional publication

Goal: close only after exhaustive matrix coverage and accepted full proof.

Completion check:

- every source acceptance criterion is callable and evidenced without
  unsupported rows being claimed as complete.
