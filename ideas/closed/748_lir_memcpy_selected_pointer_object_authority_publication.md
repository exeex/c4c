# LIR Memcpy Selected Pointer/Object Authority Publication

Status: Open (active producer blocker for idea 734)
Type: bounded LIR producer/schema/verifier authority repair
Blocked Consumer: `ideas/open/734_lir_to_new_bir_container_completeness.md`

## Goal

Publish one selected, valid current-function `LirMemcpyOp` as a structured
pointer/object operation: destination, source, and size must have typed
identity and each pointer operand must name its owning object and lifetime
contract.  The receiver must be able to consume that single row without
parsing operand display text.

## Why This Exists

Idea 734 is accepted through Step 7.19 (`565be6932`) but its earliest
remaining valid matrix row is `LirMemcpyOp`.  The current carrier has three
`LirOperand` fields whose authority is monostate/text-only.  The existing
verifier checks operand kind but cannot establish current-function use
identity, object ownership, or lifetime.  That is a producer/schema boundary,
not permission for Raw-BIR to recover facts from presentation.

## Selected Row and In Scope

- Select exactly one non-volatile `LirMemcpyOp` emitted by the existing PL
  `emit_lval_dispatch` producer, with a current-function destination object,
  current-function source object, and a native constant i64 size.  Record the
  exact focused fixture/source location in the handoff; do not substitute a
  different producer merely because its display strings look similar.
- Add the minimum typed carrier needed on that operation (or a directly owned
  typed descriptor): destination and source `LirValueId`s, a typed size
  authority (`LirValueId` or `LirIntegerImmediate`), and explicit typed
  destination/source object IDs whose owners are the same `LirFunction`.
- Publish a precise pointer/object/lifetime relation for both operands: each
  operand is a live address of its declared selected object at the memcpy site;
  object IDs are non-invalid, distinct where the selected semantics require
  it, and cannot be borrowed from another function or a dead/out-of-scope
  object.  Compatibility operand text remains display-only.
- Make the reachable LIR verifier reject absent, invalid, duplicate or
  cross-function value/object IDs, type/kind disagreement, incoherent
  value-to-object association, non-i64/nonpositive selected size authority,
  and lifetime/owner violations before any consumer may rely on the row.
- Add focused producer/verifier coverage for the selected valid row and nearby
  malformed authority cases, including transactional failure with no partial
  selected-row publication.
- Publish a durable handoff for idea 734 naming the exact fields, selected
  producer, object/lifetime ownership rules, accepted focused proof, and
  fail-closed boundary.

## Out Of Scope

- Raw-BIR containers, importer dispatch, Raw-BIR verification, receiver tests,
  target lowering, canonical BIR, allocation, MIR, or emission.
- Any second `LirMemcpyOp`, volatile memcpy, dynamic-size form, alias analysis,
  overlap semantics, generic pointer/object model, stack/alloca family,
  globals, parameters, va-list, memset, loads/stores/GEPs, aggregate/vector,
  CFG, or opaque inline-assembly authority.
- Recovering a value, object, size, lifetime, or ownership relation from
  `LirOperand::str()`, rendered LLVM, test names, or a parallel text table.

## Acceptance Criteria

- One selected PL memcpy row carries typed current-function destination,
  source, and size authority plus explicit typed object IDs and live ownership
  relations for both pointers.
- The LIR verifier makes that row fail closed for missing, invalid,
  cross-function, mismatched, and lifetime-invalid authority, with no
  presentation fallback.
- Focused producer/verifier proof covers the valid row and neighboring failure
  boundary, without weakening existing contracts or naming a testcase as the
  implementation rule.
- The handoff lets idea 734 resume at repaired Step 7.20 to receive only this
  published row.  It does not claim Raw-BIR receipt or general memory support.

## Handoff To Idea 734

After acceptance, reactivate
`ideas/open/734_lir_to_new_bir_container_completeness.md` at `Step 7.20`.
That step may receive only this handoff's selected `LirMemcpyOp` into a typed
Raw-BIR container, importer dispatch, reachable Raw-BIR verifier ownership,
and positive/negative transactional proof.  All other memory/object and
adjacent families remain separate and fail closed.

## Closure Record: selected producer contract published

Lifecycle disposition: capability complete; this source is closed after its
selected producer/schema/verifier boundary was accepted. The consumer must
reactivate `ideas/open/734_lir_to_new_bir_container_completeness.md` at
`Step 7.20 - Receive the selected LirMemcpyOp authority row` and may receive
exactly the row described here. This record does not authorize Raw-BIR receipt,
importer work, or any other memory family.

Selected producer and contract:

- The sole producer is the fixed aggregate byval parameter materialization in
  `src/codegen/lir/hir_to_lir/lvalue.cpp`, in `emit_lval_dispatch`: the
  non-volatile `%lv.param.<param> <- %p.<param>` memcpy. No other memcpy row
  is selected.
- `LirMemcpyOp::selected_authority` is optional solely so unselected memcpy
  rows retain their prior non-authoritative behavior. For the selected row it
  carries destination and source `LirValueId`s, an i64
  `LirIntegerImmediate` size, destination/source `LirObjectId`s, each object
  owner, and destination/source live-at-site facts.
- Destination is the current function's `destination_alloca`; source is its
  `byval_parameter`. Both pointer values and objects are valid and distinct,
  their types are `ptr`, their owners equal the current function's
  `link_name_id`, and both are live at the selected site. The positive size is
  native i64. Display operands remain compatibility spelling only and are
  never semantic input.

Fail-closed verifier boundary:

- When `LirFunction::selected_memcpy_pointer_authority` exists, exactly one
  `LirMemcpyOp::selected_authority` must exist. The verifier rejects absent or
  duplicate selected descriptors, a selected descriptor without the
  current-function pointer carrier, volatile selection, invalid or mismatched
  pointer IDs, invalid/mismatched objects, cross-function or mismatched
  owners, non-live facts, non-`ptr` pointer authority, non-i64 or nonpositive
  size, and any incompatible pointer/object relation.
- The verifier neither parses nor compares display text, preserves unselected
  memcpy rows, and rejects malformed selected authority before a consumer can
  rely on it.

Accepted implementation and proof:

- `6a12cddab` — publishes the selected typed authority at the fixed producer.
- `dac9c8f81` — verifies the selected authority and its malformed boundaries.
- Focused build/backend proof:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`
  passed 5/5. The supervisor's monotonic canonical before/after regression
  guard also passed (5/5 in each log, no new failures).
- Broader supervisor proof after `dac9c8f81`:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure`
  passed 3034/3034.

Out of scope remains every other memcpy or memory/object family, including
volatile and dynamic-size memcpy, builtin memcpy, stack/alloca beyond this
selected relation, globals, parameters outside this byval row, va-list,
memset, loads, stores, GEPs, aggregate/vector, CFG, Raw-BIR, importer,
lowering, and presentation-derived authority.

## Resumption Record: current-function pointer-definition prerequisite

Lifecycle decision: switched to separate blocker
`ideas/closed/749_lir_selected_memcpy_current_function_pointer_object_lifetime_authority.md`
(then open; now closed and accepted).
This source has no accepted implementation progress and no completed runbook
step. It was interrupted at Step 1, `Define and populate the selected memcpy
typed authority`, before any code or focused proof ran.

The selected PL producer is
`src/codegen/lir/hir_to_lir/lvalue.cpp:279-281`: fixed aggregate byval
parameter materialization in `emit_lval_dispatch`, producing the non-volatile
`%lv.param.<param> <- %p.<param>` memcpy. `LirMemcpyOp` and the involved
alloca/parameter carriers currently provide text only; there is no populated
current-function object/lifetime authority from which this idea can publish
valid memcpy fields. Creating that typed definition/object/lifetime authority
is outside this source's selected-memcpy publication and verifier scope.

Return point after blocker acceptance: reactivate this source at Step 1 and
retry exactly the `lvalue.cpp:279-281` selected row. Populate its memcpy fields
only from the blocker-published typed current-function pointer definitions,
local object IDs, owners, and live-lifetime relation; then resume the existing
Step 2 verifier boundary and Step 3 handoff sequence.

Last accepted baseline: root `test_before.log` records
`ctest --test-dir build -j --output-on-failure -R '^backend_'` passing 4/4.
Outgoing activation commit: `4adb768ab`. There is no Step 1 proof and no
implementation commit to preserve.

## Resumption Update: accepted blocker 749 handoff

Lifecycle decision: prerequisite blocker
`ideas/closed/749_lir_selected_memcpy_current_function_pointer_object_lifetime_authority.md`
is closed as complete, and this source is resumed at Step 1, `Define and
populate the selected memcpy typed authority`.

Accepted blocker commits:

- `7e4f8c9d6` - Step 1 Add selected memcpy pointer authority.
- `457e78757` - Step 2 Populate selected byval memcpy authority.

Accepted blocker proof:

- `cmake --preset default && cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' | tee test_after.log`
- Result: backend subset 5/5 passed, including
  `backend_lir_selected_pointer_authority`.
- Supervisor regression guard passed with `--allow-non-decreasing-passed`
  against canonical `test_before.log` and `test_after.log`; both logs recorded
  5/5 with no new failing tests.

Authority contract available to this source:

- `LirFunction::selected_memcpy_pointer_authority` is the sole selected
  carrier.
- It contains `byval_parameter` and `destination_alloca`
  `LirCurrentFunctionPointerDefinition` entries.
- Each entry carries a valid `LirValueId`, `ptr` `LirTypeRef`, distinct
  `LirObjectId` allocated via `LirFunction::alloc_object()`, `object_owner`
  equal to the selected current `LirFunction::link_name_id`, role
  `ByvalParameter` or `DestinationAlloca`, and
  `live_at_selected_site=true`.
- It is populated only by the fixed aggregate byval parameter materialization
  producer in `src/codegen/lir/hir_to_lir/lvalue.cpp` at the selected memcpy
  site.

Current return point: execute Step 1 against exactly the selected
`lvalue.cpp` producer. Populate destination/source pointer, object, and
lifetime facts for the selected `LirMemcpyOp` only from the accepted 749
authority contract. This source owns only the selected `LirMemcpyOp`
schema/publication/verifier boundary; builtin memcpy, Raw-BIR, and other
memcpy producers remain out of scope.

## Reviewer Reject Signals

- Reject text parsing, rendered-IR comparison, name lookup, or testcase-shaped
  matching offered as operand, object, size, or lifetime authority.
- Reject a carrier that has value IDs but leaves object owner/lifetime
  unverifiable, or an object table that is not bound to the selected operands.
- Reject widening to a generic pointer/object framework, a second memcpy row,
  stack/va-list/memset/load/store/GEP work, or any Raw-BIR receiver change.
- Reject expectation downgrades, helper-only refactors, or tests that omit
  missing/invalid/cross-owner/lifetime rejection while claiming capability.
- Reject publication that permits partial selected-row state after verifier
  failure or that retains the old monostate/text path as semantic fallback.
