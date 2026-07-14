# LIR Selected Memcpy Current-Function Pointer/Object/Lifetime Authority

Status: Open (active prerequisite blocker for idea 748)
Type: bounded LIR definition/object/lifetime authority repair
Blocked Consumer: `ideas/open/748_lir_memcpy_selected_pointer_object_authority_publication.md`

## Goal

Establish the minimum typed current-function definition, local-object owner,
and live-lifetime authority required for the selected PL byval-parameter memcpy
to identify its source parameter pointer and destination alloca without using
display text.

## Why This Exists

The selected producer is `src/codegen/lir/hir_to_lir/lvalue.cpp:279-281`,
fixed aggregate byval parameter materialization in `emit_lval_dispatch`. Its
non-volatile `%lv.param.<param> <- %p.<param>` memcpy needs typed identities
for the parameter pointer and destination alloca plus current-function local
objects and lifetimes. `LirMemcpyOp`, `LirAllocaOp`, and parameter values are
text-only today; LIR has no `LirObjectId` model and
`LirFunction::stack_objects` is not populated. Existing ownership checks can
only accept authoritative uses that resolve to current-function definitions.

This is a prerequisite authority route, not permission to publish, consume, or
verify a memcpy operation.

## In Scope

- Define the smallest typed LIR schema needed to represent a current-function
  pointer definition and its local object ID, owner, and live lifetime.
- Populate that authority only for the selected fixed aggregate byval parameter
  pointer and its selected destination alloca in `emit_lval_dispatch` at
  `src/codegen/lir/hir_to_lir/lvalue.cpp:279-281`.
- Bind each selected typed pointer definition to its distinct local object, the
  same owning `LirFunction`, and a lifetime that is live at the selected site;
  make these facts available to the existing authoritative-use resolution path.
- Add only definition/object/lifetime validation and focused nearby coverage
  needed to reject missing, invalid, cross-function, mismatched, or non-live
  selected authority before its later use by idea 748.
- Publish a durable handoff naming the exact definition/object/lifetime fields,
  selected producer, proof, and return point for idea 748.

## Out Of Scope

- Populating `LirMemcpyOp` fields, changing its schema, or adding memcpy
  verifier/publication behavior; idea 748 owns all of that after this handoff.
- Raw-BIR containers, importer changes, Raw-BIR verification, receiver tests,
  canonical BIR, allocation, MIR, target lowering, or emission.
- Generic expansion to other pointer, parameter, alloca, stack-object, global,
  load/store/GEP, va-list, aggregate/vector, or memory families.
- Any other memcpy producer, volatile memcpy, dynamic-size memcpy, alias or
  overlap semantics, or recovery from operand strings, rendered LLVM, test
  names, or a parallel text table.

## Acceptance Criteria

- The exact selected byval parameter pointer and destination alloca have typed
  current-function definitions with non-invalid local object IDs, one owning
  function, and live-at-site lifetime relations.
- The authoritative-use path can resolve those selected definitions and reject
  absent, foreign, mismatched, or dead authority without parsing presentation.
- Focused same-feature coverage proves both selected authority publication and
  neighboring rejection boundaries, with no expectation downgrade or
  testcase-specific semantic rule.
- The durable handoff lets idea 748 reactivate at Step 1 and retry exactly
  `src/codegen/lir/hir_to_lir/lvalue.cpp:279-281`; 748 alone then populates and
  verifies the selected `LirMemcpyOp` fields.

## Handoff To Idea 748

After supervisor acceptance, reactivate
`ideas/open/748_lir_memcpy_selected_pointer_object_authority_publication.md`
at Step 1, `Define and populate the selected memcpy typed authority`. Retry
only the fixed aggregate byval parameter materialization row at
`src/codegen/lir/hir_to_lir/lvalue.cpp:279-281`, sourcing its parameter-pointer
and destination-alloca identities and object/lifetime relations exclusively
from this handoff. Do not make the blocker itself populate or verify memcpy.

## Reviewer Reject Signals

- Reject text parsing, rendered-IR matching, test-name matching, or a parallel
  string table offered as pointer definition, object, owner, or lifetime
  authority.
- Reject an ID-only change that cannot resolve a selected current-function
  definition and bind it to a local object, owner, and live-at-site lifetime.
- Reject a generic pointer/object framework, an additional pointer/object
  family, a second memcpy producer, or any Raw-BIR/importer work as claimed
  progress for this bounded prerequisite.
- Reject memcpy field publication or memcpy-specific verifier changes here;
  those are owned by idea 748 and must remain at its return point.
- Reject expectation downgrades, helper-only renames, or focused tests that do
  not exercise missing, foreign, mismatched, and non-live authority while
  claiming capability.
