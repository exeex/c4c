# LIR-To-New-BIR Container And Import Completeness Runbook

Status: Active
Source Idea: ideas/open/734_lir_to_new_bir_container_completeness.md
Resumed from: accepted idea-744 ordinary value-identity handoff (`69d91e613`)

## Purpose

Resume bounded target-independent Raw-BIR receipt from the producer-authority
handoff, preserving prior module/signature and direct-void-call work. Take one
checked receiver row at a time and leave every other form fail-closed.

## Goal

Import each structured-authority LIR fact into one verified Raw-BIR module
without loss or partial publication. Never recover a fact from presentation.

## Core Rule

Every admitted row maps existing typed LIR authority directly to a typed
Raw-BIR container, importer path, verifier rule, and transactional proof.

## Read First

- `ideas/open/734_lir_to_new_bir_container_completeness.md`
- `docs/lir_remaining_ordinary_value_identity/handoff_to_734.md`
- `docs/lir_structured_identity/handoff_to_734.md`
- Raw-BIR function/value/node builders, views, verifier, and LIR importer

## Landed Progress

- Steps 1-3: coverage foundation plus typed module/type/value, global, string,
  extern, symbol, initializer, specialization, intrinsic, and direct scalar
  return-signature receipt.
- Steps 4.1-4.4: selected-global integer Store, Load, array-decay GEP, and
  scalar integer Return receipt from closed idea 741.
- Steps 4.5 and its bounded follow-ons: authorized parameter signatures,
  linkage/elision metadata, and direct zero-argument void Call receipt.
- The previous Step 5.1 direct-void Call is complete (`49ed1b386`). Do not
  repeat it; the first new ordinary receiver work begins at Step 5.2.
- Step 5.3 accepted the deferred intrinsic `i64 -> i32` truncation result
  (`031bab915`). The following resolved fixed-void native-floating direct-call
  importer contract stopped at the shared Raw-BIR `CallSpec` result gate:
  `FunctionBuilder::append(BlockId, CallSpec)` currently admits result-bearing
  calls only for integer returns. Repair that typed container/builder/verifier
  seam before retrying the same importer row; do not add FAdd receipt.

## Non-Goals

- no LIR redesign or presentation-text recovery
- no target interpretation, ABI placement, canonicalization, allocation, MIR,
  emission, assembler work, or legacy-BIR revival
- no receipt of producerless, raw, aggregate/object, CFG/local/body-parameter,
  indirect, variadic, ABI-expanded, or otherwise fail-closed handoff rows

## Execution Rules

1. Implement exactly one handoff row or explicitly shared typed seam per packet.
2. Add container, importer, reachable Raw-BIR verification, and transactional
   positive/negative proof together.
3. Keep source IDs, types, callee identity, and source order native; names and
   rendering are diagnostics only after structured authority exists.
4. Preserve full-module rollback for every malformed or unsupported form.
5. Record a separate producer initiative for any required authority gap.

## Ordered Steps

### Step 5.2 - Receive resolved direct integer-result calls

Goal: receive the first newly authorized idea-744 handoff row without
generalizing to unproven call forms.

Primary target:

- typed Raw-BIR direct-call payload/result registry, importer dispatch, and
  reachable verification

Actions:

- define a typed direct-call receipt for only a resolved direct `LinkNameId`
  callee, owning `LirValueId` result, matching structured return/parameter
  types, and fixed-void immediate or SSA argument subrows
- resolve callee identity and source value IDs directly from the established
  module/function registries; materialize native integer immediates directly
- verify result uniqueness/ownership, callee/signature agreement, argument
  order/type/extension, and atomic rejection of malformed authority
- prove positive direct integer-result receipt and neighboring missing,
  cross-owner, signature/type/count, alternative, and rollback failures
- keep indirect, variadic, ABI-expanded, aggregate, unresolved, nonmatching
  coercion, and floating forms unsupported

Completion check:

- a fresh build and focused receipt proof show structured result/callee/
  signature/argument edges imported transactionally with no text recovery.

### Step 5.3 - Take subsequent checked ordinary rows one at a time

Goal: receive only the next source row explicitly selected from the idea-744
handoff after Step 5.2 is accepted.

#### Step 5.3.1 - Admit a native-floating `CallSpec` result

Goal: repair the shared Raw-BIR direct-call result contract which currently
rejects the selected floating importer row before importer acceptance.

Primary targets:

- `src/backend/bir/core/builder.hpp` and `src/backend/bir/core/builder.cpp`
- the reachable Raw-BIR verifier and core builder/verifier tests

Actions:

- change only the source-backed `CallSpec::source_result_id` contract so a
  resolved direct, fixed-void native-floating callee (`F32` or `F64`) may
  define one ordinary current-function result of exactly the callee signature
  return type; retain the existing void/no-result and integer-result rules
- preserve source-result uniqueness, owner/epoch checks, instruction-result
  linkage, result-registry insertion/order, and rollback on every failure; do
  not create a new floating-call payload or alternate result carrier
- extend the Raw-BIR verifier so a `Call` instruction with a result accepts
  exactly native integer or native floating signature return types and requires
  result type, result definition, source ID, and callee signature coherence
- add focused core builder/verifier positive coverage for source-backed `F32`
  and `F64` direct-call results, plus negative coverage for void-with-result,
  result/type or signature mismatch, duplicate/cross-owner source identity,
  malformed result linkage, and transactional rollback; retain unsupported
  variadic, argument-bearing, indirect, aggregate/object, coercion, and other
  unselected call forms

Completion check:

- a fresh build and the focused core builder/verifier test selection prove that
  the Raw-BIR seam admits native-floating results without weakening existing
  integer/void contracts or publication rollback.

#### Step 5.3.2 - Retry the interrupted floating direct-call importer row

Goal: after Step 5.3.1 is accepted, execute the exact importer packet selected
in commit `aeecf048c`; this is a return point, not a new call-form selection.

Actions:

- receive only the resolved direct zero-argument fixed-void native-floating
  `LirCallOp` with a current-function owning `LirValueId` result, direct
  `LinkNameId` callee, and matching native floating return/signature
- preserve the result solely for the already checked downstream double `FAdd`
  source-use chain; do not import `FAdd` or any other scalar binary operation
- retain the exact importer/verifier and focused proof contract recorded in
  `todo.md` by `aeecf048c`, including `backend_lir_to_bir_interface` plus
  `frontend_lir_call_type_ref`

Completion check:

- the same importer row that previously failed at `CallSpec` now imports and
  rejects neighboring malformed authority transactionally, with no FAdd or
  broader floating-call receipt.

#### Step 5.3.3 - Receive the checked downstream double FAdd source chain

Goal: take the next ordered idea-744 receiver row by receiving only the
already checked `double` `LirBinOp FAdd` whose lhs is the Step 5.3.2 direct
call result; do not generalize scalar binary receipt.

Primary targets:

- a typed Raw-BIR binary payload, `BinarySpec` builder entry, and result
  registry path
- reachable Raw-BIR verification and the LIR-to-Raw-BIR importer dispatch

Actions:

- receive only `LirBinOp{result: current-function LirValueId, opcode: FAdd,
  type_str: double, lhs: the resolved Step 5.3.2 call-result LirValueId, rhs:
  current-function double LirValueId}` into a typed Raw-BIR binary instruction
  with one source-backed `F64` result and two ordered SSA operands
- map opcode, type, result, and operands directly from typed LIR authority;
  preserve the call-result use edge and result registry without presentation
  recovery
- verify native `F64`/`FAdd` coherence, exactly two current-function `F64`
  operands, result source-ID ownership/uniqueness, instruction-result linkage,
  and atomic rollback for malformed authority
- prove Raw and Canonical positive receipt plus missing, duplicate,
  cross-owner, wrong-type, non-`FAdd`, non-SSA, and malformed-result rejection
  through `backend_lir_to_bir_interface` and `frontend_lir_call_type_ref`
- keep all integer and other floating opcodes, unary, literal,
  presentation-derived, compound, complex, vector, pointer/object,
  logical-helper, builtin, cast, compare, select, return, and other unselected
  binary/use rows unsupported

Completion check:

- a fresh build and the focused two-test selection show one verified,
  transactional typed Raw-BIR `double FAdd` receipt directly linked to the
  accepted floating direct-call result, with every neighboring form fail-closed.

### Step 6 - Complete terminators and structured inline-assembly transport

Goal: receive only a source-authorized terminator or inline-assembly row at a
time. Do not treat raw CFG labels or compatibility rendering as authority.

#### Step 6.1 - Receive the checked i32 output-only inline-assembly binding

Goal: repair the existing Raw-BIR inline-assembly receipt seam so the first
currently producer-verified semantic output binding is registered by its native
source identity, rather than by its compatibility spelling.

Primary targets:

- `src/backend/bir/lir_to_bir.cpp` source-value registration and the existing
  `InlineAsmSpec` receipt path
- the existing typed Raw-BIR `InlineAsmNode` result carrier/builder/view and
  reachable Raw-BIR verifier
- `tests/backend/bir/backend_lir_to_bir_interface_test.cpp` and the focused
  frontend LIR identity proof

Typed source authority and destination:

- admit only `LirInlineAsmOp::ordinary_results[0]` whose `value` is a valid,
  current-function `LirValueId`, whose `type` is exactly `LirTypeRef::integer(32)`,
  whose role is `Output`, and whose constraint index is zero; its one later
  `LirStoreOp` use must carry the same source ID and exact i32 type
- receive that binding as result index zero of the existing typed
  `InlineAsmNode`, register the resulting Raw-BIR `ValueId` under that exact
  source ID in the current-function source-value registry, and preserve the
  existing opaque asm bytes, opaque constraint bytes, ordered clobbers, and
  side-effect flag without using any of them as value authority

Actions:

- replace spelling-keyed lookup/registration for this admitted semantic result
  edge with exact `LirValueId` lookup and registration; do not derive an ID
  from `result`, `args_str`, rendered operands, asm text, constraint text, or
  clobbers
- retain the existing `InlineAsmNode`/ordinary result carrier; verify the
  source-backed result has current-function ownership, unique source identity,
  exact i32 type, and coherent instruction-result index/definition before its
  typed Store use is accepted
- prove Raw and Canonical positive receipt for the one output-only i32 binding
  and its Store edge, plus missing/invalid/duplicate/cross-function binding
  IDs, wrong role/index/count/type, unknown or cross-function Store use,
  binding-to-Store type mismatch, malformed result linkage, and full-module
  rollback
- keep newly admitted source-ID receipt fail-closed for i64, inputs,
  read/write/tied, memory/address/immediate, clobber/explicit-register,
  vector/aggregate/multi-output, `insn_r`, and all opaque-text-derived facts;
  do not reopen target interpretation, constraint parsing/binding, allocation,
  or assembler work. Preserve the closed idea-731 carrier contract rather than
  claiming that its broader historical transport validates this new source-ID
  registry row.

Completion check:

- a fresh build and focused interface/frontend proof show that one i32
  output-only semantic binding reaches a verified Raw and Canonical
  `InlineAsmNode` result and its Store through native IDs only, while malformed
  neighbors publish neither module.

#### Step 6.2 - Select the next authority-backed terminator or inline-assembly row

Goal: after Step 6.1, select exactly one subsequent row only when its source
authority and typed Raw-BIR destination are evidenced. CFG labels remain
unavailable as typed successor authority and must not be recovered from text.

Completion check:

- the selected row names its exact typed source carriers, destination,
  verifier ownership, transactional proof, and fail-closed neighbors.

### Step 7 - Integrate the dispatcher, verifier and build boundary

Goal: prove landed families form one production importer with no partial state.

Completion check:

- broader proof passes and every accepted row has typed authority and receipt.

### Step 8 - Prove lossless completeness and transactional publication

Goal: close only after exhaustive matrix coverage and accepted full proof.

Completion check:

- every source acceptance criterion is callable and evidenced without
  unsupported rows being claimed as complete.
