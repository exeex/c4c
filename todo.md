Status: Active
Source Idea Path: ideas/open/597_pointer_address_semantic_model_research.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Classify Semantic Authority And Fact Roles

# Current Packet

## Just Finished

Completed `plan.md` Step 3: Classify Semantic Authority And Fact Roles.

Filled
`docs/pointer_address_semantic_model_research/02_semantic_authority_and_fact_classes.md`
with authority decisions for every Step 2 inventoried family. The document
separates semantic authority from verifier/support facts, target-consume
facts, route proofs, and diagnostic-only artifacts, and explicitly classifies
stack-home completeness, local-memory layout, target operand shape, and
relocation materialization as insufficient authority by themselves.

Decisions recorded: branch pointer stack-source operands are semantic
freshness authority only for the exact selected branch stack-load source use;
local-array and global static semantic GEP `Available` records are semantic
address-derivation authority; global symbol memory access authority is limited
to exact symbol-backed memory access/range use; frame-slot addressing,
frame/global address materialization, relocation materialization, raw
local-array derivation records, and target-local operand shape are support,
target-consume, route-proof, or diagnostic facts. Pointer base-plus-offset
homes and pointer-value indirect memory accesses remain deferred pending a
first owner and proof surface for use-specific pointer freshness.

## Suggested Next

Step 4: Define Fail-Closed Rules.

## Watchouts

- This is a research route; do not edit implementation files, tests,
  expectations, unsupported markers, allowlists, runtime behavior, or harness
  behavior.
- Treat idea 591 as downstream Prepared MIR view work that consumes this
  semantic model, not as the owner of unresolved pointer/address semantics.
- Treat ideas 592, 593, 594, and 596 as narrow branch pointer stack-source
  evidence, not global pointer/address semantic closure.
- Step 4 should define fail-closed behavior for support-only evidence:
  stack-home-only, local-layout-only, relocation-only, target-shape-only,
  range-only, stale-base, wrong-use, and diagnostic-only routes.
- Deferred families should remain deferred unless Step 4 can name a first
  owner and proof surface. Do not let pointer base-plus-offset homes or
  pointer-value memory range helpers become pointer freshness authority without
  a selected use-specific authority.

## Proof

Docs/todo-only packet. Validation command:
`git diff --check` passed with no output. `test_after.log` was not updated for
this packet.
