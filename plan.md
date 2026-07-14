# LIR Selected Memcpy Current-Function Pointer/Object/Lifetime Authority Runbook

Status: Active
Source Idea: ideas/open/749_lir_selected_memcpy_current_function_pointer_object_lifetime_authority.md
Activated from: blocked idea 748 Step 1

## Purpose

Supply the one missing current-function pointer-definition/object/lifetime
authority that idea 748 must use before it can populate the selected memcpy.

## Goal

Give exactly the selected fixed aggregate byval parameter pointer and
destination alloca typed current-function identities, local objects, ownership,
and live-at-site lifetime facts.

## Core Rule

Typed definitions and local object/lifetime relations are the only authority.
Textual operands, rendered LLVM, and test naming may show parity but must never
identify a pointer, object, owner, or lifetime.

## Read First

- `ideas/open/749_lir_selected_memcpy_current_function_pointer_object_lifetime_authority.md`
- `ideas/open/748_lir_memcpy_selected_pointer_object_authority_publication.md`
- `src/codegen/lir/hir_to_lir/lvalue.cpp:279-281`
- `src/codegen/lir/ir.hpp` and `src/codegen/lir/verify.cpp`

## Non-Goals

- Do not alter `LirMemcpyOp`, populate its fields, or add memcpy verifier or
  publication behavior.
- Do not touch Raw-BIR/importer code, generic pointer/object infrastructure,
  other pointer/object families, or any other memcpy producer.

## Execution Rules

1. Keep the scope to the exact byval parameter pointer and destination alloca
   at `lvalue.cpp:279-281`.
2. Make owner and lifetime facts current-function and live at the selected site;
   do not infer them from spelling or a test fixture.
3. Verify the prerequisite authority independently of memcpy publication.
4. Prove fresh build, focused authority coverage, then the supervisor-selected
   regression/broader checks before handing the route back to idea 748.

## Ordered Steps

### Step 1 - Define bounded current-function pointer and object authority

Goal: add only the typed definitions, local object IDs, owner relation, and
live-lifetime representation necessary for the selected pointer pair.

Primary targets:

- LIR definition/object schema and current-function ownership resolution

Actions:

- identify the smallest typed carriers that make a parameter pointer and
  alloca resolve as current-function definitions;
- represent local object identity, owning `LirFunction`, and live-at-site
  lifetime without creating a generic pointer/object framework; and
- keep all unselected families absent or fail-closed.

Completion check:

- a selected typed definition can resolve to one local object, owner, and
  lifetime relation without text-derived authority.

### Step 2 - Populate and validate the selected producer authority

Goal: make `emit_lval_dispatch` produce valid prerequisite authority for only
the fixed aggregate byval parameter materialization row.

Primary target:

- `src/codegen/lir/hir_to_lir/lvalue.cpp:279-281`

Actions:

- populate typed parameter-pointer and destination-alloca definitions and
  their local object/owner/live-lifetime bindings;
- make authoritative-use validation reject missing, invalid, cross-function,
  mismatched, and non-live selected bindings; and
- add focused same-feature coverage without using test names or rendered text
  as the semantic decision rule.

Completion check:

- the selected pair resolves through typed current-function authority and each
  nearby malformed authority boundary fails closed.

### Step 3 - Prove and hand off to the blocked memcpy publication route

Goal: establish accepted prerequisite evidence and make the parent resumption
mechanical.

Actions:

- run a fresh build and focused definition/object/lifetime proof;
- provide the supervisor the exact proof command and changed authority contract
  for matching regression/broader validation; and
- record the selected fields and accepted proof in the handoff, then request
  reactivation of idea 748 at Step 1 for the exact `lvalue.cpp:279-281` retry.

Completion check:

- idea 748 can populate its memcpy fields solely from a named, verified typed
  authority contract without this blocker changing any memcpy behavior.
