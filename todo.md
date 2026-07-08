Status: Active
Source Idea Path: ideas/open/597_pointer_address_semantic_model_research.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Define Fail-Closed Rules

# Current Packet

## Just Finished

Completed `plan.md` Step 4: Define Fail-Closed Rules.

Filled
`docs/pointer_address_semantic_model_research/03_fail_closed_rules.md`
with shared failure-mode definitions, per-family fail-closed behavior, current
code authority-risk notes, and later implementation test shapes. The document
defines missing, ambiguous, stale, wrong-value, wrong-use, stack-home-only,
local-layout-only, relocation-less, relocation-only, range-only,
target-shape-only, and diagnostic-only failures.

Rules recorded: support, route-proof, target-consume, and diagnostic facts
must not upgrade themselves into semantic authority. Branch stack-source and
semantic GEP paths appear to fail closed for the surveyed authority dimensions.
No proven diagnostic-as-authority acceptance was found in this packet.
Pointer-value memory access, global symbol memory access,
relocation/materialization, local/global memory layout, and target operand
consumption remain deferred support-as-authority risks where later
implementation work should prove consumers do not infer freshness from
support-only facts.

## Suggested Next

Step 5: Position Closed Evidence And The Prepared MIR Boundary.

## Watchouts

- This is a research route; do not edit implementation files, tests,
  expectations, unsupported markers, allowlists, runtime behavior, or harness
  behavior.
- Treat idea 591 as downstream Prepared MIR view work that consumes this
  semantic model, not as the owner of unresolved pointer/address semantics.
- Treat ideas 592, 593, 594, and 596 as narrow branch pointer stack-source
  evidence, not global pointer/address semantic closure.
- Step 5 should position closed evidence without reopening closed branch
  pointer stack-source work or claiming global pointer/address semantic
  closure from it.
- Preserve the distinction between selected freshness authority, semantic GEP
  address-derivation authority, exact symbol-backed memory-access authority,
  and support-only facts such as relocation, stack homes, range/layout, and
  target operand shape.
- Deferred risks are not proven current bugs. Keep later recommendations split
  by first owning layer and proof surface.

## Proof

Docs/todo-only packet. Validation command:
`git diff --check` passed with no output. `test_after.log` was not updated for
this packet.
