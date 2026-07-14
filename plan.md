# LIR-To-New-BIR Container And Import Completeness Runbook

Status: Active
Source Idea: ideas/open/734_lir_to_new_bir_container_completeness.md
Resumed from: closed idea 747 direct-branch successor handoff (`cebc0a3bf`)

## Purpose

Continue the bounded target-independent Raw-BIR receiver route without
repeating accepted work. Receive exactly one ordinary scalar integer comparison
from the producer-published native predicate, type, operand, and result
authority.

## Goal

Import each structured-authority LIR fact into one verified Raw-BIR module
without loss or partial publication. Never recover a fact from presentation.

## Core Rule

Every admitted row maps existing typed LIR authority directly to a typed
Raw-BIR container, importer path, verifier rule, and transactional proof.
Native `LirCmpOp` integer mode, predicate, type ref, operands, and result ID
are the sole authority for the selected compare. `LirBr.successor` remains the
sole direct-edge authority; presentation is display only.

## Read First

- `ideas/open/734_lir_to_new_bir_container_completeness.md`
- `docs/lir_remaining_ordinary_value_identity/handoff_to_734.md`
- `docs/lir_remaining_ordinary_value_identity/authority_matrix.md` (Step-7.2)
- Raw-BIR instruction builders, views, verifier, and LIR importer

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
- Step 6.3: direct `LirBr.successor` receipt as a typed Raw-BIR `JumpTerm`
  (`97efe9c38`), following closed idea 747's producer carrier publication
  (`cebc0a3bf`). Do not repeat this direct-branch row.
- Step 6.4: explicit i32-to-i64 `LirCastOp::SExt` receipt and its exact i64
  Add use (`39518d27a`). Do not repeat this cast row.
- Step 6.5: selected i32 `LirCmpOp::Slt` receipt with the admitted global Load
  lhs and native immediate seven (`03448676f`). Do not repeat or generalize
  this compare row.

## Non-Goals

- no LIR redesign, presentation-text recovery, target interpretation, ABI
  placement, canonicalization, allocation, MIR, emission, assembler work, or
  legacy-BIR revival
- no conditional, switch, indirect, phi, local/body-parameter, unselected
  inline-assembly, or new cast/compare receipt

## Execution Rules

1. Implement exactly one handoff row or explicitly shared typed seam per packet.
2. Add container, importer, reachable Raw-BIR verification, and transactional
   positive/negative proof together.
3. Resolve the selected compare only through native mode/predicate/type and
   current-function source IDs or native immediates; names and rendering are
   diagnostics only after structured authority exists.
4. Preserve full-module rollback for every malformed or unsupported form.
5. Record a separate producer initiative for any required authority gap.

## Ordered Steps

### Step 6.3 - Receive the checked direct `LirBr` successor (complete)

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

- accepted in `97efe9c38`: a fresh build and focused 2/2 backend/producer proof
  showed one transactional typed direct jump using `LirBlockId` only.

### Step 6.4 - Receive the checked explicit i32-to-i64 `SExt` result (complete)

Goal: receive only the earliest unreceived producer-verified ordinary-cast row
from the idea-744 handoff; do not generalize Cast receipt.

Primary targets:

- a typed Raw-BIR cast payload, builder/view, and result registry path
- reachable Raw-BIR verification and LIR-to-Raw-BIR instruction dispatch
- focused backend receiver coverage plus `frontend_lir_call_type_ref`

Actions:

- map only explicit non-pointer, non-vector
  `LirCastOp{result: valid current-function LirValueId, operand: valid
  current-function LirValueId, kind: SExt, from_type: i32, to_type: i64}` to
  one typed Raw-BIR cast result; preserve its exact downstream i64 Add use
- verify strict i32-to-i64 extension direction, source/result ownership and
  uniqueness, instruction-result linkage, native kind/endpoint coherence, and
  full-module rollback for missing, invalid, duplicate, cross-owner, wrong
  endpoint/kind, unresolved-use, or malformed linkage authority
- prove one positive SExt-to-i64-Add chain and neighboring transactional
  failures. Keep no-op, truncation, other widths/kinds, floating, pointer,
  bitcast, vector, aggregate, implicit, and presentation-derived cast forms
  fail-closed

Completion check:

- accepted in `39518d27a`: a fresh build and focused 2/2 backend/producer proof
  showed one verified transactional typed i32-to-i64 `SExt` using structured
  authority only.

### Step 6.5 - Receive the checked i32 `SLT` compare result

Goal: receive only the focused producer-verified ordinary scalar integer
comparison row; do not generalize compare or import its normalization cast.

Primary targets:

- a typed Raw-BIR compare payload, builder/view, and result registry path
- reachable Raw-BIR verification and LIR-to-Raw-BIR instruction dispatch
- focused backend receiver coverage plus `frontend_lir_call_type_ref`

Actions:

- map only the `lir_scalar_compare_result_use_identity` subrow:
  `LirCmpOp{result: valid current-function LirValueId, is_float: false,
  predicate: Slt, type_str: i32, lhs: exact admitted selected-global i32 Load
  result ID, rhs: LirIntegerImmediate{7}}` to one typed Raw-BIR i32 `SLT`
  comparison result
- preserve result source-ID ownership/uniqueness, ordered operand identity and
  immediate range, predicate/mode/type coherence, instruction-result linkage,
  and full-module rollback for missing, invalid, duplicate, cross-owner,
  wrong predicate/mode/type, unresolved operand, or malformed linkage
  authority
- prove a positive compare receipt and neighboring transactional failures.
  Leave the following normalization `LirCastOp{ZExt, i1, i32}` unsupported
  because its result is a monostate compatibility carrier; do not infer or
  repair it from rendering
- keep all other integer predicates/types, floating, pointer, vector, complex,
  logical-helper, builtin, vaarg, statement, CFG, and presentation-derived
  compare forms fail-closed

Completion check:

- a fresh build and supervisor-selected focused backend receiver proof, with
  `frontend_lir_call_type_ref` as the producer regression neighbor, show one
  verified transactional typed i32 `SLT` compare using structured authority
  only.

### Step 7.1 - Prove mixed accepted-row dispatcher transactionality

Goal: cover one production module that dispatches already accepted ordinary
rows in source order and prove that a later malformed admitted row publishes
no partial Raw-BIR module. This is a shared typed seam coverage packet, not a
new LIR receipt.

Primary targets:

- `src/backend/bir/lir_to_bir.cpp` existing instruction dispatcher and
  transactional lowering boundary
- `tests/backend/bir/backend_lir_to_bir_interface_test.cpp` focused mixed-row
  positive and rollback coverage

Actions:

- construct exactly one module using only already admitted forms: the selected
  global i32 Load, normalized i32 Add, explicit i32-to-i64 SExt, and selected
  i32 SLT compare with its current-function Load lhs and native immediate
  seven rhs; assert that the existing importer preserves their source order and
  typed result/use authority without adding a new builder, container, or
  instruction dispatch alternative
- construct a neighboring copy differing only in the final selected compare's
  native authority (for example predicate/mode/type coherence) and assert the
  existing transactional lowering boundary returns the established rejection
  with no Raw-BIR module publication; do not test for partial graph repair
- retain every individual row's existing negative checks. Do not admit the
  compare normalization `LirCastOp{ZExt, i1, i32}`, other compare predicates
  or types, floating/pointer/vector/complex rows, or any currently unsupported
  instruction family

Completion check:

- a fresh build and the focused
  `^backend_lir_to_bir_interface$|^frontend_lir_call_type_ref$` proof pass
  2/2. The backend test demonstrates ordered mixed existing-row receipt and
  full rollback of the one malformed final compare, while the frontend test
  remains the producer-authority regression neighbor.

### Step 8 - Prove lossless completeness and transactional publication

Goal: close only after exhaustive matrix coverage and accepted full proof.

Completion check:

- every source acceptance criterion is callable and evidenced without
  unsupported rows being claimed as complete.
