# LIR-To-New-BIR Container And Import Completeness Runbook

Status: Active
Source Idea: ideas/open/734_lir_to_new_bir_container_completeness.md
Resumed from: closed idea 747 direct-branch successor handoff (`cebc0a3bf`)

## Purpose

Continue the bounded target-independent Raw-BIR receiver route without
repeating accepted work. Receive exactly one producer-published builtin-ffs
equality-to-zero comparison result and its exact select condition edge from
native comparison, value, and immediate authority.

## Goal

Import each structured-authority LIR fact into one verified Raw-BIR module
without loss or partial publication. Never recover a fact from presentation.

## Core Rule

Every admitted row maps existing typed LIR authority directly to a typed
Raw-BIR container, importer path, verifier rule, and transactional proof.
The selected builtin-ffs `LirCmpOp` integer Eq/type, fresh result ID, exact
immediate zero, and select condition ID are the sole authority.
Presentation is display only.

## Read First

- `ideas/open/734_lir_to_new_bir_container_completeness.md`
- `docs/lir_remaining_ordinary_value_identity/handoff_to_734.md`
- `docs/lir_remaining_ordinary_value_identity/authority_matrix.md` (Step-7.15)
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
- Step 7.1: mixed accepted-row source-order and full-rollback dispatcher proof
  (`7c259b660`). It added no instruction receipt. Do not widen it.
- Step 7.2: selected i32 `LirAbsOp` receipt, its source-backed result, and its
  exact later i32 Add use (`0c44e810a`). Do not repeat or generalize this Abs
  row.
- Step 7.3: selected i32 `Cttz`-to-Add-one receipt (`2f7055845`). Do not
  repeat or generalize this intrinsic-use row.
- Step 7.4: resolved fixed-void external native-double direct-call result and
  its later typed FAdd use (`2eee4d6dd`). Do not receive FAdd in that packet.
- Step 7.5: ordinary scalar floating double FAdd-to-FMul receipt
  (`af00014d7`). Do not generalize floating binary or literal authority.
- Step 7.6: ordinary scalar floating double OLt comparison receipt with its
  non-materialized exact ZExt use (`c0534153e`). Do not generalize compare.
- Step 7.7: explicit scalar double-to-float FPTrunc receipt and exact later
  float FMul use (`d90f32090`). Do not generalize floating casts.
- Step 7.8: explicit scalar float-to-double FPExt receipt and exact later
  double FMul use (`dff35977c`). Do not repeat or generalize this cast row.
- Step 7.9: explicit scalar signed i32-to-double SIToFP receipt and exact
  later double FMul use (`44f4119a2`). Do not repeat or generalize this cast
  row.
- Step 7.10: explicit scalar unsigned i32-to-double UIToFP receipt and exact
  later double FMul use (`4999fbc6f`). Do not repeat or generalize this cast
  row. The focused backend/producer proof passed 2/2, the matching regression
  guard passed with its allowed non-decreasing fixed-test count, and fresh
  broader `^backend_` proof passed 4/4.
- Steps 7.11 and 7.12: explicit scalar FPToSI and FPToUI receipts with their
  exact later i32 Add uses (`3e45cef35`, `9f8194fd0`). Do not repeat or
  generalize these conversion rows.
- Step 7.13: operand-free typed i64 builtin-ffs select receipt, its exact
  i64-to-i32 Trunc, and later ordinary i32 Add (`f88276157`). The fresh
  focused backend/producer proof passed 2/2, the matching regression guard
  was non-decreasing, and fresh broader `^backend_` proof passed 4/4. Do not
  receive the ffs condition or arms through that completed row.
- Step 7.14: checked i32/i64 builtin-ffs native Add-one receipt and its exact
  select false-arm edge (`b0f4c4a93`). The fresh focused backend/producer
  proof passed 2/2, the matching regression guard was non-decreasing, and
  fresh broader `^backend_` proof passed 4/4. Do not receive the Cttz lhs or
  select condition through that completed row.

## Non-Goals

- no LIR redesign, presentation-text recovery, target interpretation, ABI
  placement, canonicalization, allocation, MIR, emission, assembler work, or
  legacy-BIR revival
- no conditional, switch, indirect, phi, local/body-parameter, unselected
  inline-assembly, cttz-call, select false-arm, or new cast/binary receipt

## Execution Rules

1. Implement exactly one handoff row or explicitly shared typed seam per packet.
2. Add container, importer, reachable Raw-BIR verification, and transactional
   positive/negative proof together.
3. Resolve the selected builtin-ffs equality comparison only through its
   native integer Eq/type, current-function result ID, representable immediate
   zero, and exact select condition ID; names and rendering are diagnostics
   only after structured authority exists.
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

### Step 7.1 - Prove mixed accepted-row dispatcher transactionality (complete)

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

Accepted in `7c259b660`: the fresh focused 2/2 backend/producer proof showed
ordered receipt of existing admitted rows and no publication after the final
compare's malformed authority.

### Step 7.2 - Receive the checked i32 integer Abs result

Goal: receive only the authority-matrix Step-7.4 selected i32 `LirAbsOp` row;
do not generalize absolute-value or call receipt.

Primary targets:

- typed Raw-BIR `Abs` payload, opcode/builder, and immutable view
- reachable Raw-BIR verifier and LIR-to-Raw-BIR instruction dispatch
- focused backend receiver coverage plus `frontend_lir_call_type_ref`

Actions:

- map only `LirAbsOp{result: valid current-function LirValueId, arg: exact
  admitted selected-global i32 Load result ID, int_type: i32}` from the
  `lir_scalar_abs_result_use_identity` subrow to one typed Raw-BIR i32 Abs
  result, preserving its exact later admitted i32 Add use
- verify exact integer type, source/result ownership and uniqueness,
  instruction-result linkage, ordered argument identity, and full-module
  rollback for missing, invalid, duplicate, cross-owner, unresolved-argument,
  wrong-type, or malformed-linkage authority
- prove one positive Abs receipt plus neighboring transactional failures. Keep
  `labs`/`llabs`, immediate inputs, non-i32 types, noninteger/aggregate/vector
  forms, all other builtin/call routes, and presentation-derived authority
  fail-closed

Completion check:

- a fresh build and the focused
  `^backend_lir_to_bir_interface$|^frontend_lir_call_type_ref$` proof pass
  2/2. The backend coverage demonstrates typed i32 Abs receipt and rollback;
  the frontend test remains the producer-authority regression neighbor.

Accepted in `0c44e810a`: the fresh focused 2/2 backend/producer proof showed
one verified transactional typed i32 Abs receipt and its admitted i32 Add use.

### Step 7.3 - Receive the checked ffs `Cttz`-to-Add-one result

Goal: receive only the authority-matrix Step-7.14 shared builtin-ffs
add-one subrow whose lhs is the already admitted native i32 `Cttz` result.
Do not yet receive the following zero comparison or Select.

Primary targets:

- typed Raw-BIR binary payload/builder/view and reachable verification
- LIR-to-Raw-BIR binary dispatch and source-value registry
- focused backend receiver coverage plus `frontend_lir_call_type_ref`

Actions:

- map only the selected `LirBinOp{result: valid current-function LirValueId,
  opcode: Add, type_str: i32, lhs: exact admitted i32 Cttz result ID,
  rhs: LirIntegerImmediate{1}}` from the ffs chain to one typed Raw-BIR i32
  Add result; preserve its exact source ordering and source-ID identity
- extend the existing admitted i32 Add contract only to permit that checked
  intrinsic-result lhs. Require the native Cttz provenance, ordered operand
  identity, representable immediate one, result ownership/uniqueness,
  instruction-result linkage, and full-module rollback for missing, invalid,
  duplicate, cross-owner, wrong-provenance, wrong-opcode/type/immediate, or
  malformed-linkage authority
- prove the positive Cttz-to-Add-one receipt and neighboring transactional
  failures. Keep all other builtin arithmetic, different operands/immediates,
  select/compare receipt, other integer widths/opcodes, floating, pointer,
  vector, aggregate, logical-helper, and presentation-derived forms fail-closed

Completion check:

- a fresh build and the focused
  `^backend_lir_to_bir_interface$|^frontend_lir_call_type_ref$` proof pass
  2/2. The backend coverage demonstrates one verified transactional i32
  Cttz-to-Add-one receipt with no select or compare admission; the frontend
  test remains the producer-authority regression neighbor.

Accepted in `2f7055845`: the fresh focused 2/2 backend/producer proof showed
one verified transactional typed i32 Cttz-to-Add-one receipt. This completed
packet did not admit the following comparison or Select.

### Step 7.4 - Receive the checked fixed-void external double call result

Goal: receive only the authority-matrix Step-7.32 resolved external
`double target(void)` `LirCallOp` result and its exact later `FAdd` use. Do not
receive the FAdd itself or widen floating/direct-call receipt.

Primary targets:

- typed Raw-BIR direct-call payload/builder/view and reachable verification
- LIR-to-Raw-BIR direct-call dispatch and current-function source-value registry
- focused backend receiver coverage plus `frontend_lir_call_type_ref`

Actions:

- map only the Step-7.32
  `LirCallOp{result: valid current-function LirValueId, direct_callee_link_name_id:
  module LinkNameId, return_type: double, callee_signature: exact empty fixed-void
  native-double signature}` to one typed Raw-BIR direct-call result; preserve
  declaration/callee identity, source-result identity, source order, and the
  exact later double `FAdd` lhs use without reading names or rendering
- require exactly one matching module Function declaration, matching native
  floating return and fixed-void signature, result ownership/uniqueness, and
  instruction-result/use linkage; reject missing/duplicate declaration,
  unresolved or cross-owner callee/result/use, signature/type conflict, and
  malformed linkage with no module publication
- prove the positive fixed-void external-double call and its exact `FAdd` use,
  plus neighboring transactional failures. Keep the FAdd receipt, indirect,
  variadic, argument-bearing, ABI-expanded, aggregate/object, nonmatching
  floating, ffs compare/Select, and presentation-derived forms fail-closed

Completion check:

- a fresh build and the focused
  `^backend_lir_to_bir_interface$|^frontend_lir_call_type_ref$` proof pass
  2/2. The backend coverage demonstrates one verified transactional resolved
  fixed-void external-double call result and later typed FAdd use without
  admitting FAdd or broader call forms; the frontend test remains the
  producer-authority regression neighbor.

Accepted in `2eee4d6dd`: the fresh focused 2/2 backend/producer proof showed
one verified transactional resolved fixed-void native-double direct-call result
and its later typed FAdd use without receiving FAdd.

### Step 7.5 - Receive the checked ordinary scalar floating binary result

Goal: receive only the authority-matrix Step-7.5 ordinary scalar floating
double FAdd-to-FMul chain. Do not infer literal values from presentation or
widen scalar binary receipt.

Primary targets:

- typed Raw-BIR floating binary payload, builder/view, and reachable verifier
- LIR-to-Raw-BIR binary dispatch and current-function source-value registry
- focused backend receiver coverage plus `frontend_lir_call_type_ref`

Actions:

- map only the producer-verified ordinary, nonpointer, nonvector scalar
  floating `LirBinOp` double FAdd-to-FMul chain, preserving each native
  opcode/type and exact result/source-ID edge; the FMul lhs must be the FAdd
  result ID
- require valid, unique current-function results; resolved operands; native
  floating opcode/type coherence; instruction-result linkage; and full-module
  rollback for missing, invalid, duplicate, cross-owner, unresolved-use,
  opcode/type-conflict, or malformed-linkage authority
- prove the positive chain plus neighboring transactional failures. Keep
  floating literal authority, integer/nonfloating opcode/type mixes, casts,
  comparisons, complex/vector/pointer/logical-helper/compound/builtin/vaarg/
  statement producers, and presentation-derived forms fail-closed

Completion check:

- a fresh build and focused
  `^backend_lir_to_bir_interface$|^frontend_lir_call_type_ref$` proof pass
  2/2. The backend coverage demonstrates one verified transactional ordinary
  scalar floating chain; the frontend test remains the producer-authority
  regression neighbor.

Accepted in `af00014d7`: the fresh focused 2/2 backend/producer proof showed
one verified transactional ordinary scalar double FAdd-to-FMul chain. The
matching regression guard passed 2/2 before/after, and fresh broader
`^backend_` proof passed 4/4.

### Step 7.6 - Receive the checked ordinary scalar floating compare result

Goal: receive only the authority-matrix Step-7.6 ordinary scalar floating
double OLt comparison and its compatibility-result ZExt use. Do not receive
the ZExt result or widen comparison receipt.

Primary targets:

- typed Raw-BIR floating compare payload, builder/view, and reachable verifier
- LIR-to-Raw-BIR compare dispatch and current-function source-value registry
- focused backend receiver coverage plus `frontend_lir_call_type_ref`

Actions:

- map only the producer-verified ordinary, nonpointer, nonvector scalar
  floating `LirCmpOp` double OLt result, preserving native floating mode,
  predicate, compared type, result ID, and the exact downstream compatibility
  ZExt use without receiving that cast result
- require valid, unique current-function results; resolved operands; coherent
  floating mode/predicate/type; instruction-result linkage; and full-module
  rollback for missing, invalid, duplicate, cross-owner, unresolved-use,
  predicate/mode/type-conflict, or malformed-linkage authority
- prove the positive comparison boundary plus neighboring transactional
  failures. Keep floating literal authority, the normalization cast result,
  integer comparisons, other predicates/types, and all pointer/vector/complex/
  logical-helper/builtin/vaarg/statement/presentation-derived forms fail-closed

Completion check:

- a fresh build and focused
  `^backend_lir_to_bir_interface$|^frontend_lir_call_type_ref$` proof pass
  2/2. The backend coverage demonstrates one verified transactional ordinary
  scalar floating comparison boundary; the frontend test remains the
  producer-authority regression neighbor.

Accepted in `c0534153e`: the fresh focused 2/2 backend/producer proof showed
one verified transactional ordinary scalar double OLt comparison and its exact
non-materialized ZExt use. The matching regression guard passed 2/2
before/after, and fresh broader `^backend_` proof passed 4/4.

### Step 7.7 - Receive the checked explicit scalar FPTrunc result

Goal: receive only the authority-matrix Step-7.7 explicit nonpointer,
nonvector scalar double-to-float `LirCastOp::FPTrunc` and its exact later float
FMul use. Do not generalize floating casts.

Primary targets:

- typed Raw-BIR FPTrunc payload, builder/view, and reachable verifier
- LIR-to-Raw-BIR cast dispatch and current-function source-value registry
- focused backend receiver coverage plus `frontend_lir_call_type_ref`

Actions:

- map only the producer-verified explicit scalar `LirCastOp{result: valid
  current-function LirValueId, operand: exact admitted floating source ID,
  kind: FPTrunc, from_type: double, to_type: float}` to one typed Raw-BIR
  result, preserving its exact later float FMul use
- require valid, unique current-function results; resolved source/result use
  edges; exact FPTrunc kind and double-to-float endpoints; strict narrowing;
  instruction-result linkage; and full-module rollback for missing, invalid,
  duplicate, cross-owner, unresolved-use, wrong-kind/endpoints/direction, or
  malformed-linkage authority
- prove the positive source-to-FPTrunc-to-FMul chain plus neighboring
  transactional failures. Keep FPExt and integer/floating conversions,
  pointer/bitcast/vector/complex/aggregate, implicit, no-op, monostate-source,
  and presentation-derived casts fail-closed

Completion check:

- a fresh build and focused
  `^backend_lir_to_bir_interface$|^frontend_lir_call_type_ref$` proof pass
  2/2. The backend coverage demonstrates one verified transactional explicit
  scalar FPTrunc chain; the frontend test remains the producer-authority
  regression neighbor.

Accepted in `d90f32090`: the fresh focused 2/2 backend/producer proof showed
one verified transactional explicit scalar double-to-float FPTrunc chain. The
matching regression guard passed 2/2 before/after, and fresh broader
`^backend_` proof passed 4/4.

### Step 7.8 - Receive the checked explicit scalar FPExt result

Goal: receive only the authority-matrix Step-7.8 explicit nonpointer,
nonvector scalar float-to-double `LirCastOp::FPExt` and its exact later double
FMul use. Do not generalize floating casts.

Primary targets:

- typed Raw-BIR FPExt payload, builder/view, and reachable verifier
- LIR-to-Raw-BIR cast dispatch and current-function source-value registry
- focused backend receiver coverage plus `frontend_lir_call_type_ref`

Actions:

- map only the producer-verified explicit scalar `LirCastOp{result: valid
  current-function LirValueId, operand: exact admitted floating source ID,
  kind: FPExt, from_type: float, to_type: double}` to one typed Raw-BIR result,
  preserving its exact later double FMul use
- require valid, unique current-function results; resolved source/result use
  edges; exact FPExt kind and float-to-double endpoints; strict widening;
  instruction-result linkage; and full-module rollback for missing, invalid,
  duplicate, cross-owner, unresolved-use, wrong-kind/endpoints/direction, or
  malformed-linkage authority
- prove the positive source-to-FPExt-to-FMul chain plus neighboring
  transactional failures. Keep FPTrunc and integer/floating conversions,
  pointer/bitcast/vector/complex/aggregate, implicit, no-op, monostate-source,
  and presentation-derived casts fail-closed

Completion check:

- a fresh build and focused
  `^backend_lir_to_bir_interface$|^frontend_lir_call_type_ref$` proof pass
  2/2. The backend coverage demonstrates one verified transactional explicit
  scalar FPExt chain; the frontend test remains the producer-authority
  regression neighbor.

Accepted in `dff35977c`: the fresh focused 2/2 backend/producer proof showed
one verified transactional explicit scalar float-to-double FPExt chain. The
matching regression guard passed with its allowed non-decreasing fixed-test
count, and fresh broader `^backend_` proof passed 4/4.

### Step 7.9 - Receive the checked explicit scalar SIToFP result

Goal: receive only the authority-matrix Step-7.9 explicit nonpointer,
nonvector scalar signed i32-to-double `LirCastOp::SIToFP` and its exact later
double `FMul` use. Do not generalize integer-to-floating casts.

Primary targets:

- typed Raw-BIR SIToFP payload, builder/view, and reachable verifier
- LIR-to-Raw-BIR cast dispatch and current-function source-value registry
- focused backend receiver coverage plus `frontend_lir_call_type_ref`

Actions:

- map only the producer-verified explicit scalar `LirCastOp{result: valid
  current-function LirValueId, operand: exact admitted signed i32 Add result
  ID, kind: SIToFP, from_type: i32, to_type: double}` to one typed Raw-BIR
  result, preserving its exact later double FMul use
- require valid, unique current-function results; resolved source/result use
  edges; exact SIToFP kind and signed-i32-to-double endpoints; strict
  integer-to-floating direction; instruction-result linkage; and full-module
  rollback for missing, invalid, duplicate, cross-owner, unresolved-use,
  wrong-kind/endpoints/direction, or malformed-linkage authority
- prove the positive Add-to-SIToFP-to-FMul chain plus neighboring transactional
  failures. Keep UIToFP, FPTrunc/FPExt, floating-to-integer, pointer/bitcast/
  vector/complex/aggregate, implicit, no-op, monostate-source, and
  presentation-derived casts fail-closed

Completion check:

- a fresh build and focused
  `^backend_lir_to_bir_interface$|^frontend_lir_call_type_ref$` proof pass
  2/2. The backend coverage demonstrates one verified transactional explicit
  scalar SIToFP chain; the frontend test remains the producer-authority
  regression neighbor.

Accepted in `44f4119a2`: the fresh focused 2/2 backend/producer proof showed
one verified transactional explicit scalar signed i32-to-double SIToFP chain.
The matching regression guard passed its allowed non-decreasing fixed CTest
count, and fresh broader `^backend_` proof passed 4/4.

### Step 7.10 - Receive the checked explicit scalar UIToFP result

Goal: receive only the authority-matrix Step-7.10 explicit nonpointer,
nonvector scalar unsigned i32-to-double `LirCastOp::UIToFP` and its exact later
double `FMul` use. Do not generalize integer-to-floating casts.

Primary targets:

- typed Raw-BIR UIToFP payload, builder/view, and reachable verifier
- LIR-to-Raw-BIR cast dispatch and current-function source-value registry
- focused backend receiver coverage plus `frontend_lir_call_type_ref`

Actions:

- map only the producer-verified explicit scalar `LirCastOp{result: valid
  current-function LirValueId, operand: exact admitted unsigned i32 Add result
  ID, kind: UIToFP, from_type: i32, to_type: double}` to one typed Raw-BIR
  result, preserving its exact later double FMul use
- require valid, unique current-function results; resolved source/result use
  edges; exact UIToFP kind and unsigned-i32-to-double endpoints; strict
  integer-to-floating direction; instruction-result linkage; and full-module
  rollback for missing, invalid, duplicate, cross-owner, unresolved-use,
  wrong-kind/endpoints/direction, or malformed-linkage authority
- prove the positive Add-to-UIToFP-to-FMul chain plus neighboring transactional
  failures. Require native UIToFP kind authority: signless i32 type refs do
  not independently distinguish an SIToFP/UIToFP kind swap. Keep SIToFP,
  FPTrunc/FPExt, FPToSI/FPToUI, pointer/bitcast/vector/complex/aggregate,
  implicit, no-op, monostate-source, and presentation-derived casts fail-closed

Completion check:

- a fresh build and focused
  `^backend_lir_to_bir_interface$|^frontend_lir_call_type_ref$` proof pass
  2/2. The backend coverage demonstrates one verified transactional explicit
  scalar UIToFP chain; the frontend test remains the producer-authority
  regression neighbor.

Accepted in `4999fbc6f`: the fresh focused 2/2 backend/producer proof showed
one verified transactional explicit scalar unsigned i32-to-double UIToFP
chain. The matching regression guard passed with its allowed non-decreasing
fixed-test count, and fresh broader `^backend_` proof passed 4/4.

### Step 7.11 - Receive the checked explicit scalar FPToSI result

Goal: receive only the authority-matrix Step-7.11 explicit nonpointer,
nonvector scalar double-to-signed-i32 `LirCastOp::FPToSI` and its exact later
i32 Add use. Do not generalize floating-to-integer casts.

Primary targets:

- typed Raw-BIR FPToSI payload, builder/view, and reachable verifier
- LIR-to-Raw-BIR cast dispatch and current-function source-value registry
- focused backend receiver coverage plus `frontend_lir_call_type_ref`

Actions:

- map only the producer-verified explicit scalar `LirCastOp{result: valid
  current-function LirValueId, operand: exact admitted double FAdd result ID,
  kind: FPToSI, from_type: double, to_type: i32}` to one typed Raw-BIR result,
  preserving its exact later i32 Add use
- require valid, unique current-function results; resolved source/result use
  edges; exact FPToSI kind and double-to-signed-i32 endpoints; strict
  floating-to-integer direction; instruction-result linkage; and full-module
  rollback for missing, invalid, duplicate, cross-owner, unresolved-use,
  wrong-kind/endpoints/direction, or malformed-linkage authority
- prove the positive FAdd-to-FPToSI-to-Add chain plus neighboring transactional
  failures. Require native FPToSI kind authority: signless i32 type refs do
  not independently distinguish an FPToSI/FPToUI kind swap. Keep FPToUI,
  SIToFP/UIToFP, FPTrunc/FPExt, pointer/bitcast/vector/complex/aggregate,
  implicit, no-op, monostate-source, and presentation-derived casts fail-closed

Completion check:

- a fresh build and focused
  `^backend_lir_to_bir_interface$|^frontend_lir_call_type_ref$` proof pass
  2/2. The backend coverage demonstrates one verified transactional explicit
  scalar FPToSI chain; the frontend test remains the producer-authority
  regression neighbor.

Accepted in `3e45cef35`: the fresh focused 2/2 backend/producer proof showed
one verified transactional explicit scalar double-to-signed-i32 FPToSI chain.
The matching regression guard passed non-decreasing 2/2, and fresh broader
`^backend_` proof passed 4/4.

### Step 7.12 - Receive the checked explicit scalar FPToUI result

Goal: receive only the authority-matrix Step-7.12 explicit nonpointer,
nonvector scalar double-to-unsigned-i32 `LirCastOp::FPToUI` and its exact later
unsigned i32 Add use. Do not generalize floating-to-integer casts.

Primary targets:

- typed Raw-BIR FPToUI payload, builder/view, and reachable verifier
- LIR-to-Raw-BIR cast dispatch and current-function source-value registry
- focused backend receiver coverage plus `frontend_lir_call_type_ref`

Actions:

- map only the producer-verified explicit scalar `LirCastOp{result: valid
  current-function LirValueId, operand: exact admitted double FAdd result ID,
  kind: FPToUI, from_type: double, to_type: i32}` to one typed Raw-BIR result,
  preserving its exact later unsigned i32 Add use
- require valid, unique current-function results; resolved source/result use
  edges; exact FPToUI kind and double-to-unsigned-i32 endpoints; strict
  floating-to-integer direction; instruction-result linkage; and full-module
  rollback for missing, invalid, duplicate, cross-owner, unresolved-use,
  wrong-kind/endpoints/direction, or malformed-linkage authority
- prove the positive FAdd-to-FPToUI-to-Add chain plus neighboring transactional
  failures. Require native FPToUI kind authority: signless i32 type refs do
  not independently distinguish an FPToUI/FPToSI kind swap. Keep FPToSI,
  SIToFP/UIToFP, FPTrunc/FPExt, pointer/bitcast/vector/complex/aggregate,
  implicit, no-op, monostate-source, and presentation-derived casts fail-closed

Completion check:

- a fresh build and focused
  `^backend_lir_to_bir_interface$|^frontend_lir_call_type_ref$` proof pass
  2/2. The backend coverage demonstrates one verified transactional explicit
  scalar FPToUI chain; the frontend test remains the producer-authority
  regression neighbor.

Accepted in `9f8194fd0`: the fresh focused 2/2 backend/producer proof showed
one verified transactional explicit scalar double-to-unsigned-i32 FPToUI chain.
The matching regression guard passed non-decreasing 2/2, and fresh broader
`^backend_` proof passed 4/4.

### Step 7.13 - Receive the checked wide builtin-ffs select narrowing (complete)

Goal: receive only PI's authority-matrix i64 `__builtin_ffsll`
`LirSelectOp` result and its required explicit i64-to-i32 `LirCastOp::Trunc`
edge to the exact later ordinary i32 Add use. Do not generalize select or cast
receipt.

Primary targets:

- typed Raw-BIR select payload and i64-to-i32 truncation payload, builders,
  views, and reachable verification
- LIR-to-Raw-BIR select/cast dispatch and current-function source-value registry
- focused backend receiver coverage plus `frontend_lir_call_type_ref`

Actions:

- map only the producer-verified PI i64 ffs `LirSelectOp{result: valid
  current-function LirValueId, type_str: i64}` result through one typed
  Raw-BIR i64 select container, then map its exact result ID only through
  `LirCastOp{result: valid current-function LirValueId, operand: that select
  result ID, kind: Trunc, from_type: i64, to_type: i32}` to one typed
  Raw-BIR truncation result, preserving the exact later ordinary i32 Add use
- require valid, unique current-function select/cast results; resolved
  select-to-trunc and trunc-to-Add edges; exact native i64-to-i32 Trunc kind
  and strict narrowing; instruction-result linkage; and full-module rollback
  for missing, invalid, duplicate, cross-owner, unresolved-use,
  wrong-kind/endpoints/direction, or malformed-linkage authority
- prove the positive select-to-Trunc-to-Add chain plus neighboring transactional
  failures. Do not receive the shared ffs add-one false arm, equality-to-zero
  condition, Cttz call, i32 ffs route, other select/cast producers,
  pointer/vector/complex/aggregate/object work, implicit coercions, or
  presentation-derived authority

Completion check:

- a fresh build and focused
  `^backend_lir_to_bir_interface$|^frontend_lir_call_type_ref$` proof pass
  2/2. The backend coverage demonstrates one verified transactional wide ffs
  select narrowing chain; the frontend test remains the producer-authority
  regression neighbor.

Accepted in `f88276157`: the fresh focused 2/2 backend/producer proof showed
one verified transactional operand-free i64 ffs Select-to-Trunc-to-Add chain.
The matching regression guard was non-decreasing, and fresh broader
`^backend_` proof passed 4/4.

### Step 7.14 - Receive the checked builtin-ffs add-one/select false arm (complete)

Goal: receive only PI's shared i32/i64 builtin-ffs integer Add-one result and
its exact `LirSelectOp.false_val` edge. Do not receive the Cttz lhs, equality
comparison, select condition, or other binary/select rows.

Primary targets:

- typed Raw-BIR scalar integer binary payload, builder/view, and reachable
  verifier support for an exact representable immediate
- LIR-to-Raw-BIR binary dispatch and current-function source-value registry
- focused backend receiver coverage plus `frontend_lir_call_type_ref`

Actions:

- map only PI's producer-verified `LirBinOp{result: valid current-function
  LirValueId, opcode: Add, type_str: i32|i64, rhs: LirIntegerImmediate{1}}`
  to one typed Raw-BIR integer Add result and preserve that exact result ID as
  the existing i32/i64 ffs `LirSelectOp.false_val`
- require valid, unique add-one results; native Add/type coherence; a
  representable exact immediate one; and resolved same-function result-to-
  false-arm linkage, with full-module rollback for missing, invalid, duplicate,
  cross-owner, unresolved-use, opcode/type conflict, wrong operand authority,
  unrepresentable immediate, or malformed linkage
- prove i32 and i64 positive add-one-to-false-arm boundaries plus neighboring
  transactional failures. Keep the Cttz lhs, Eq-zero comparison and condition,
  other builtin/binary/select producers, pointer/vector/complex/aggregate/
  object work, implicit coercions, and presentation-derived authority
  fail-closed

Completion check:

- a fresh build and focused
  `^backend_lir_to_bir_interface$|^frontend_lir_call_type_ref$` proof pass
  2/2. The backend coverage demonstrates verified transactional i32/i64
  builtin-ffs add-one to false-arm linkage; the frontend test remains the
  producer-authority regression neighbor.

Accepted in `b0f4c4a93`: the fresh focused 2/2 backend/producer proof showed
verified transactional i32/i64 builtin-ffs Add-one-to-false-arm linkage. The
matching regression guard was non-decreasing, and fresh broader `^backend_`
proof passed 4/4.

### Step 7.15 - Receive the checked builtin-ffs zero-comparison/select condition

Goal: receive only PI's shared i32/i64 builtin-ffs equality-to-zero comparison
result and its exact `LirSelectOp.cond` edge. Do not receive the prepared
argument, Cttz call, add-one false arm, or other compare/select rows.

Primary targets:

- typed Raw-BIR scalar integer comparison payload, builder/view, and reachable
  verifier support for an exact representable immediate
- LIR-to-Raw-BIR comparison dispatch and current-function source-value registry
- focused backend receiver coverage plus `frontend_lir_call_type_ref`

Actions:

- map only PI's producer-verified `LirCmpOp{result: valid current-function
  LirValueId, mode: integer, predicate: Eq, type_str: i32|i64,
  rhs: LirIntegerImmediate{0}}` to one typed Raw-BIR integer equality result
  and preserve that exact result ID as the existing i32/i64 ffs
  `LirSelectOp.cond`
- require valid, unique comparison results; native integer Eq/type coherence;
  a representable exact immediate zero; and resolved same-function
  result-to-condition linkage, with full-module rollback for missing, invalid,
  duplicate, cross-owner, unresolved-use, predicate/mode/type conflict, wrong
  operand authority, unrepresentable immediate, or malformed linkage
- prove i32 and i64 positive equality-to-zero-to-condition boundaries plus
  neighboring transactional failures. Keep the prepared argument, Cttz call,
  Add-one false arm, other builtin/compare/select producers, pointer/vector/
  complex/aggregate/object work, implicit coercions, and presentation-derived
  authority fail-closed

Completion check:

- a fresh build and focused
  `^backend_lir_to_bir_interface$|^frontend_lir_call_type_ref$` proof pass
  2/2. The backend coverage demonstrates verified transactional i32/i64
  builtin-ffs equality-to-zero to select-condition linkage; the frontend test
  remains the producer-authority regression neighbor.

### Source completion gate (not an executor packet)

Do not execute this as a placeholder. This source cannot close until its
coverage matrix has an evidenced typed receiver disposition for every valid
current-LIR fact and the source acceptance criteria, including broader
supervisor-selected proof, are met. Each remaining receiver-ready row requires
its own plan repair and bounded executor packet; any missing producer authority
requires a separately scoped successor/blocker rather than presentation
recovery.
