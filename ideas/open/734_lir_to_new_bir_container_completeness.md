# LIR-To-New-BIR Container And Import Completeness

Status: Open (resumed after closed 825's accepted DirectScalar switch-selector
parameter-authority handoff; Step 7.38 receiver packet active)
Type: target-independent new-BIR schema and LIR import completeness
Historical Documentation Input:
the pre-implementation phase-A acceptance recorded by
`ideas/open/732_bir_stage_document_convergence_umbrella.md`; it is evidence for
this implementation route, not a current docs-first activation barrier
Historical Bounded Proof:
`ideas/closed/731_inline_asm_transport_and_regalloc_contract.md`
Parked Downstream Proposal:
`ideas/draft/733_accepted_bir_a1_f3_architecture_implementation.md`

## Goal

Give every semantic fact in the existing complete typed LIR surface a
lossless, target-independent typed container in new Raw BIR and a faithful,
module-transactional LIR-to-new-BIR importer path.

This is a C++ implementation initiative. It owns the new Raw-BIR typed
structs/containers, builders, immutable views, verifier, importer, build wiring
and tests required to make that goal callable. It is not documentation-only.

## Lifecycle Sequencing

The user's current sequencing supersedes the former umbrella-wide docs-first
prohibition. Idea 734 paused while idea 741 resolved the structured LIR
identity blocker recorded below, then received the four exact store/load/GEP/
return rows from that committed handoff. At Step 4.5 it reached a second
producer-authority boundary: declarations publish structured logical
parameters through `LirFunction.params`, but definitions omitted that
publication while retaining their structured ABI signature tracks. Closed
idea 742 repaired and classified that producer seam. Idea 734 then received
the bounded target-stable parameter signatures, function linkage/elision
metadata, and direct zero-argument void calls. At the next Step 5 row it reached
the repeated remaining ordinary result/use identity family now owned by active
idea 744. Idea 734 remains open and must resume only from idea 744's accepted
handoff. The in-progress phase-C documentation child remains open but parked.
The already-landed phase-A and phase-B documentation acceptances and phase-C
C1-C6 slices are preserved as historical evidence; they are not current
post-implementation acceptance.

After this idea is implemented, accepted and closed, reactivate idea 732 and
rerun documentation convergence against the landed C++ implementation from
phase A, then B, then C and onward. Do not resume directly at the old phase-C
Step 7 checkpoint.

## Resumption Record: direct-branch successor authority blocker

Paused after accepted Step 5.3.5 normalized i32 `Mul` receipt
(`ea4b63135`, focused 2/2 proof) and the already landed Step 6.1 i32 and Step
6.2 i64 output-only inline-assembly native-ID binding/selected-global Store
receipts (`ad82d1456`, `37014f013`). Steps 5.2 through 6.2 are completed
historical work and must not be repeated.

The interrupted point is `Step 6.3 - Select the next authority-backed
terminator or inline-assembly row`. At pause, the first candidate direct
`LirBr` row had only a raw target label; this idea cannot recover CFG identity from text and
does not own the required LIR producer/schema change. The now-closed producer
blocker `ideas/closed/747_lir_direct_branch_successor_identity_publication.md`
owns the accepted publication and verification of the same-function structural
direct-successor `LirBlockId` plus its receiver handoff.

Blocker acceptance is recorded in `cebc0a3bf` with focused producer proof
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_lir_call_type_ref$'`.
Resume at Step 6.3 with one bounded direct-`LirBr` receiver packet: map the
handoff's carried successor ID to a typed Raw-BIR direct-jump destination,
verify same-function edge ownership and rollback, and prove the receiver's
positive/negative transactional boundary. All conditional/switch/indirect
terminators and nonselected inline-assembly forms remain separate and
fail-closed.

## Resumed Consumer Status

Execution resumes at the function-body receipt boundary after module/global and
direct scalar signature receipt. Closed idea 741 added four bounded producer
authority contracts for direct selected-global scalar integer store, direct
selected-global scalar load, selected-global array-decay GEP, and scalar integer
return. `docs/lir_structured_identity/handoff_to_734.md` is the current exact
evidence for their typed `LirTypeRef`, `LirValueId`, `LinkNameId`,
`LirIntegerImmediate`, and ordered typed-index facts.

The first receiver packet owns only the direct selected-global scalar integer
store subrow and the generic value/global/immediate mapping it requires. The
other handed-off rows remain ordered later packets. All other raw or monostate
compatibility rows stay fail-closed; the handoff does not prove whole-modern-LIR
readiness and does not authorize parsing display text or further LIR redesign.

## Current Parameter Authority Handoff

Closed idea 742 established declaration/definition logical publication and an
exact structured parity contract among `LirFunction.params`,
`signature_params`, and `signature_param_type_refs` for default-shape
nonvariadic plain fixed scalars. It also preserved the distinct zero-parameter
and explicit-void shapes.

The first resumed Step 4.5 packet owns only those zero/void shapes and
default-shape nonvariadic fixed `int`, `uint`, `long long`, `unsigned long
long`, `float`, and `double` declaration/definition signatures. It must not
bind body operands to parameters.

`long` and `unsigned long` are newly blocked by a target-policy contradiction:
production LIR and its verifier require an unconditional 64-bit mirror while
new BIR preserves 32-bit I686 long semantics. Open inactive idea 743 owns that
cross-surface convergence. Pointer, narrow, aggregate/byval,
HFA/vector/other expansion, variadic, function-pointer, and `va_list` rows
remain blocked exactly as the handoff matrix states. Names, signature
rendering, body text, and ABI position remain non-authoritative.

## Current Ordinary Value Identity Blocker

Plan Step 5.1 direct zero-argument void `Call` receipt is accepted in commit
`49ed1b386`, with fresh focused proof and a 3033/3033 full monotonic regression
checkpoint. That receiver row stores only a structured target `FunctionId` and
needs no result or argument value identity.

The next call rows are not receiver-ready. Production scalar-result calls use
`fresh_tmp(ctx)`, leaving `LirCallOp.result` with display spelling but no
`LirValueId`. `lir_call_structured_args` constructs argument operands from
formatted strings, so scalar SSA call uses likewise lack stable current-
function identity. Current authoritative `fresh_value(ctx)` production is
limited to the bounded selected-global load/GEP seams delivered by closed idea
741. The same missing result/use family recurs across most remaining ordinary
variants.

Idea 734 must not reconstruct these identities from result names, formatted
operands, printer output, or testcase identity. Closed idea 744 has now
published the checked producer handoff in
`docs/lir_remaining_ordinary_value_identity/handoff_to_734.md` (commit
`69d91e613`), after the final resolved external-double verifier packet
`c2f0f13e9` and accepted 3033/3033 regression proof. Resume only at the first
newly authorized receiver row: resolved direct integer-result `LirCallOp` with
an owning result `LirValueId`, direct `LinkNameId` callee, and matching
structured fixed-void signature. The already received 741 regression-neighbor
rows and accepted zero-argument void-call row are historical progress and must
not be repeated.

The handoff's other rows remain later bounded packets. CFG/terminator targets,
stack/local objects, body parameters, producerless legacy alternatives, and
every handoff fail-closed form remain separate and unsupported until their own
typed source/receiver contracts are selected.

## Runbook Repair Decision: Step 7.17

Close rejected. Accepted Step 7.17 (`701476a5a`) has fresh focused 2/2 proof,
a non-decreasing matching regression guard, and fresh broader `^backend_` 4/4
proof, but it does not satisfy this source's coverage-matrix completion gate.
The next receiver-ready typed row is PI's builtin-clz i32/i64 Ctlz call, i64
narrowing, and final-use chain under authority-matrix Step 7.18; builtin-
popcount and later bounded rows remain after it. Valid current-LIR families
with missing producer authority, including pointer/object, stack/local,
body-parameter, aggregate/vector, va-list/memory, CFG, and opaque inline-asm
forms, remain separately scoped.

Classification: `repair-current-route` for Step 7.18 because its required
authority and typed Raw-BIR receiving seams already exist; execute one bounded
receiver packet without presentation recovery. The listed missing-authority
families are not absorbed by that packet: each requires its own separately
scoped successor/blocker before any receiver work. Return point after Step
7.18 acceptance is the next matrix receiver-ready row, with the same source
completion gate reapplied.

## Runbook Repair Decision: Step 7.19

Close rejected. Accepted Step 7.18 builtin-clz receipt (`93a075d34`) has a
fresh build, focused 2/2 proof, non-decreasing matching regression guard, and
fresh broader `^backend_` 4/4 proof, but it does not satisfy this source's
coverage-matrix completion gate. Step 7.17 builtin-ctz remains accepted
historical progress. The next receiver-ready typed row is PI's builtin-popcount
i32/i64 `Ctpop` call, i64 narrowing, and final-use chain under
authority-matrix Step 7.19. Later bounded rows remain after it. Valid
current-LIR families with missing producer authority, including pointer/object,
stack/local, body-parameter, aggregate/vector, va-list/memory, CFG, and opaque
inline-asm forms, remain separately scoped.

Classification: `repair-current-route` for Step 7.19 because its required
authority and typed Raw-BIR receiving seams already exist; execute exactly one
bounded receiver packet without presentation recovery. Receive only a native
`Ctpop` i32/i64 result with direct module `LinkNameId`, fixed nonvariadic
one-integer signature, no `zero_count_behavior`, and its exact direct i32 Add
or i64-to-i32 Trunc-to-Add final-use chain. Do not absorb parity,
prepared-argument receipt, or any other intrinsic/call/cast/binary family. The
listed missing-authority families are not absorbed by this packet: each
requires its own separately scoped successor/blocker before any receiver work.
Return point after Step 7.19 acceptance is the next matrix receiver-ready row,
with the same source completion gate reapplied.

## Runbook Repair Decision: post-Step 7.19 producer blocker

Close rejected. Accepted Step 7.19 builtin-popcount receipt (`565be6932`) has
the supervisor-provided fresh build, focused 2/2 proof, non-decreasing matching
regression guard, and fresh broader `^backend_` 4/4 proof. It nevertheless does
not satisfy the source completion gate: every valid current-LIR fact still
needs an evidenced typed receiver disposition, and no additional
receiver-ready row remains after the accepted bounded rows.

The earliest remaining valid matrix row is `LirMemcpyOp`. Its destination,
source, and size are text-only/monostate, and its pointer/object and
object-lifetime relationship lacks a current-function structured authority
contract. Idea 734 must not invent that identity from display operands, so this
is a `separate-blocker`, not a repair-current-route packet.

Required switch: the supervisor must create and activate one narrowly scoped
producer successor that publishes, verifies, and hands off a selected
`LirMemcpyOp` pointer/object authority row, including its current-function
operand IDs and object/lifetime ownership. Preserve this idea's accepted work
through Step 7.19 and its cited proof. Return to idea 734 only after that
handoff, at repaired Step 7.20: receive exactly the handed-off `LirMemcpyOp`
row in typed Raw-BIR, importer dispatch, reachable verification, and
transactional positive/negative coverage. All other memory/object, stack,
va-list, CFG, aggregate/vector, parameter, opaque-inline-asm, and
presentation-derived rows remain separately scoped and fail-closed.

## Runbook Exhaustion Decision: post-Step 7.20

Close rejected. Accepted Step 7.20 (`1a3adbc58`) received exactly the closed-
748 selected non-volatile fixed-aggregate byval `LirMemcpyOp` authority into a
typed Raw-BIR container, importer, reachable verifier, and transactional
positive/negative coverage. The supervisor acceptance evidence is fresh focused
backend proof (5/5), a matching non-decreasing before/after regression guard
(5/5), and the broader full checkpoint (3034/3034). That proves one bounded
memory/object receiver row; it does not satisfy the source completion gate.

Unmet source criteria are: the checked matrix still has valid current-LIR rows
without evidenced typed Raw-BIR receiver dispositions, including conditional,
switch, and indirect CFG successors; PHI incoming value/predecessor identity;
local/stack/object and general pointer authority; remaining memory/va-list
rows; aggregate/vector identity; body-parameter identity; and other unreceived
instruction, terminator, module, type, global, metadata, inline-asm, and call
families. Consequently the source cannot yet claim every current-LIR semantic
fact has a lossless typed destination, complete neighboring coverage, or a
complete explicit dispatcher.

Classification: `separate-blocker`. `ideas/open/750_lir_cfg_terminator_block_identity_completion.md`
is the first executable successor and owns publication and verification of
typed active CFG-successor authority. This source is paused after completed
Steps 1 through 7.20, including the accepted selected memcpy receipt. After
750 closes with an exact structured handoff, reactivate 734 and repair its
runbook for the first handoff-authorized bounded Raw-BIR CFG receiver row;
derive that row from the accepted handoff rather than label text. Do not repeat
Step 7.20 or absorb PHI, local/object, memory/va, aggregate/vector,
body-parameter, or other remaining families into successor 750.

## Resumption Record: CFG successor-authority completion

Closed idea 750 completed the active successor-authority prerequisite at
lifecycle pointer `62be1f1`, with accepted commits `0f214dc` (conditional and
switch) and `40673da` (computed-goto). The typed current-function successor
fields are `LirBr.successor`, `LirCondBr.true_successor` and
`false_successor`, `LirSwitch.default_successor` and ordered
`case_successors`, and ordered `LirIndirectBrOp.successors`. Labels are checked
display mirrors only. Missing, invalid, duplicate or ambiguous, and
foreign-function authority fails closed before printing or downstream use; no
receiver may recover it from text.

The acceptance proof is the focused 1/1 pre/post `frontend_lir_call_type_ref`
guard and a fresh full `ctest --test-dir build -j --output-on-failure` result
of 3034/3034 passing.

Exact return action: do not activate this idea as part of this record. When
the supervisor next activates 734, plan-owner must repair the runbook for one
bounded legacy `LirIndirectBr` receiver packet only: map its existing typed
`addr` `LirValueId` and ordered current-function target IDs to Raw-BIR's typed
indirect-jump destination, verify ownership and target order, and prove
transactional rejection/no partial publication. `LirCondBr`, `LirSwitch`, and
`LirIndirectBrOp` are not receiver-ready in that packet because their condition,
selector, or address identity remains outside the 750 handoff and must stay
fail-closed without presentation recovery.

## Runbook Exhaustion Decision: post-Step 7.21

Close rejected. Accepted Step 7.21 (`b528dc1`) received exactly legacy
`LirIndirectBr.addr` and its ordered current-function targets into verified
Raw-BIR `IndirectJumpTerm` authority, with transactional malformed
address/target rollback coverage. The matching backend guard passed 5/5 and
the prior full checkpoint remains 3034/3034 passing. This proves one bounded
CFG receiver row, not this source's complete matrix.

The next candidate `LirCondBr` receiver row is not source-authorized:
`true_successor` and `false_successor` are typed after closed idea 750, but
`cond_name` remains display text without a current-function `LirValueId`.
`LirSwitch` likewise lacks typed selector authority, and `LirIndirectBrOp`
lacks typed address authority. This source must not recover any of those facts
from labels, names, printer output, or rendered operands.

Classification: `separate-blocker`. Open idea 755 owns only conditional-branch
condition value identity publication, verification, and a bounded handoff.
After its accepted handoff, reactivate 734 and repair its runbook for one
`LirCondBr` Raw-BIR receiver packet that consumes the typed condition and the
already accepted typed successor IDs. Do not repeat Step 7.21 or absorb switch,
computed-goto, PHI, local/object, memory/va, aggregate/vector, body-parameter,
or other remaining families.

## Runbook Exhaustion Decision: post-Step 7.22

Close rejected. Accepted Step 7.22 (`220a3b5ad`) received exactly typed
`LirCondBr.condition`, `true_successor`, and `false_successor` into the
verified Raw-BIR conditional-jump destination. It validates a local I1
condition and distinct same-function targets before atomic Raw-BIR
publication, with focused positive and malformed-authority rollback coverage.
The supervisor acceptance evidence is a fresh build plus matching backend
guard 5/5 before and after (non-decreasing); the focused cases are internal to
an existing executable. A later full postcheck found 2961/3034 passing and 73
failures, but no matching clean pre-change full baseline exists because the
external c-testsuite submodule contents are absent. This is not full-suite
green evidence.

This bounded conditional receiver does not satisfy the source completion gate.
The earliest next valid CFG row, `LirSwitch`, has typed
`default_successor` and ordered `case_successors` from closed idea 750, but
its semantic selector remains `selector_name`/`selector_type` presentation
text without a current-function `LirValueId`. Idea 734 must not recover the
selector from text. `LirIndirectBrOp` address authority, PHI, local/object,
memory/va, aggregate/vector, body-parameter, and all other unreceived families
remain separately scoped and fail closed.

Classification: `separate-blocker`. Closed idea
`ideas/closed/756_lir_switch_selector_value_identity_publication.md` owned
publication, verification, focused malformed-authority coverage, and a typed
handoff for the active switch selector. Its accepted handoff resumes 734 for
one bounded `LirSwitch` Raw-BIR receiver packet: consume only the typed
selector with already typed default and ordered case successors, preserve case
order and transactional rejection, and do not inspect selector or label text
for semantic recovery. Do not repeat Step 7.22 or absorb computed-goto, PHI,
local/object, memory/va, aggregate/vector, body-parameter, or other remaining
families.

## Resumption Record: switch-selector authority completion

Closed idea 756 completed the switch-selector prerequisite with accepted
implementation `cafc757ec`. Active `LirSwitch.selector` is a verifier-checked
current-function `LirValueId`; missing, foreign, non-integer, and
display-mismatched authority fails closed before printing or downstream use.
The pre-existing typed `default_successor` and ordered `case_successors` from
closed idea 750 remain the sole CFG target authority. The handoff is accepted
with a fresh build, focused `^frontend_lir_` proof 4/4 and matching
non-decreasing guard, and broader `^(frontend_cxx_|positive_sema_)` proof
35/35. No Raw-BIR work landed in 756.

Exact return action: repair and execute one bounded Step 7.23 `LirSwitch`
Raw-BIR receiver. Consume only `selector`, `default_successor`, and ordered
`case_successors`; preserve integer selector facts and case order; verify the
typed authority and reject malformed input transactionally before publication.
Do not derive semantics from selector/label display text, repeat Step 7.22, or
absorb `LirIndirectBrOp`, PHI, local/object, memory/va, aggregate/vector,
body-parameter, or any other family.

## Runbook Exhaustion Decision: post-Step 7.23

Close rejected. Accepted Step 7.23 (`0995a3deb`) received exactly typed
`LirSwitch.selector`, `default_successor`, and ordered `case_successors` into
verified Raw-BIR `SwitchTerm` authority, with transactional malformed-selector
and target rollback coverage. The supervisor acceptance evidence is a fresh
build plus matching backend guard 5/5 before and after (non-decreasing). A
full baseline candidate was rejected: the current full result has 73 failures
against the stored 3034/3034 green checkpoint, so this is not full-suite green
evidence.

This one bounded switch receiver does not satisfy the source completion gate.
The earliest remaining computed-goto row, `LirIndirectBrOp`, has ordered typed
current-function `successors` from closed idea 750, but its pointer `addr`
remains only a `LirOperand` without a current-function `LirValueId`. Idea 734
must not recover pointer identity from its operand spelling, labels, printer
output, or rendered text. PHI, local/object, memory/va, aggregate/vector,
body-parameter, and all other unreceived families remain separately scoped and
fail closed.

Classification: `separate-blocker`. Closed idea
`ideas/closed/757_lir_computed_goto_address_value_identity_publication.md`
owned only publication, verification, focused malformed-authority coverage,
and a typed handoff for the active `LirIndirectBrOp` address. Its accepted
handoff resumes this source for one bounded `LirIndirectBrOp` Raw-BIR receiver
packet: consume only the typed address and already typed ordered successors,
preserve target order and transactional rejection, and do not inspect operand
or label text for semantic recovery. Do not repeat Step 7.23 or absorb PHI,
local/object, memory/va, aggregate/vector, body-parameter, or other remaining
families.

## Resumption Record: computed-goto address authority completion

Closed idea 757 completed the active computed-goto address prerequisite with
accepted implementation `8527c6dbc`. Each active `LirIndirectBrOp` now carries
optional typed `addr_value` authority populated directly by `emit_rval_operand`;
`addr` remains a checked display mirror only. The LIR verifier rejects missing,
invalid, foreign, non-pointer, and display-mismatched authority before printing
or downstream consumption. Closed idea 750's ordered current-function
`successors` remain the sole target authority.

The accepted proof is a fresh build plus focused
`^frontend_lir_call_type_ref$` proof 1/1, with matching `^backend_` regression
guard passing 5/5 before and after under allow-non-decreasing. No Raw-BIR work
landed in 757.

Exact return action: repair and execute one bounded Step 7.24
`LirIndirectBrOp` Raw-BIR receiver. Consume only `addr_value` and ordered
`successors`; verify address presence, validity, current-function ownership,
pointer suitability, target presence/ownership/order, and transactional
rejection before publication. Do not derive semantics from `addr`, labels,
printer output, or rendered text; do not repeat Step 7.23 or absorb PHI,
local/object, memory/va, aggregate/vector, body-parameter, or any other
family.

## Why This Exists

The landed importer proves only a bounded structured inline-assembly path. It
does not establish that new BIR can receive the rest of the existing LIR
module, function, instruction, terminator, value, type, object, and metadata
surface. Unsupported diagnostics are useful fail-closed behavior, but they do
not substitute for missing new-BIR containers or importer wiring.

Existing LIR is authoritative and complete for this initiative. README
`source gap` labels are observations about current documentation/importer
assumptions to audit and correct; they are not evidence that LIR itself may be
changed. Idea 734 isolates the new-BIR receiving boundary and does not authorize
canonicalization or any later backend stage.

The sole possible LIR-schema exception is the existing inline-assembly
constraint carrier. A minimal carrier edit is allowed only if concrete evidence
shows that current ordinary input/output positions and roles cannot record an
already reviewed constraint requirement losslessly. This exception cannot
create a separate inline-asm value model or authorize downstream interpretation.

Quarantined legacy-BIR structures and their disposition ledger are coverage
evidence only. They may reveal semantic families that must remain representable,
but they are not the new schema and must not be revived, copied wholesale or
compiled to claim completeness.

## Original Activation Evidence

The following records the pre-implementation boundary that motivated this
idea. Landed Steps 1-3 and the structured-identity handoff supersede these
observations where the active runbook now records implemented capability.

- `src/backend/bir/lir_to_bir.cpp::validate_module_surface` rejects globals,
  string-pool state, extern declarations and indexes, type/struct/layout
  declarations, intrinsic-requirement flags, and specialization metadata.
- `validate_function` accepts only zero-parameter, non-variadic, void-return
  functions; rejects stack objects and hoisted allocas; and requires the LIR
  entry block to be first.
- Ordinary instruction import admits only `LirInlineAsmOp` from the current
  `LirInst` variant.
- Terminator import admits only void `LirRet`, unconditional `LirBr`, and
  `LirUnreachable`; conditional branch, switch, and indirect branch reject.
- `src/backend/CMakeLists.txt` excludes
  `src/backend/bir/lir_to_bir/*.cpp`, documenting those translation units as
  unmigrated semantic families.
- `src/backend/bir/lir_to_bir/README.md` inventories many rows as `source gap`
  and states that full compiler-backend coverage is unproven while such rows
  remain. Idea 734 must audit those labels against actual existing LIR and
  assign missing responsibility to new BIR containers or importer wiring.

## In Scope

- inventory every existing `LirInst` and `LirTerminator` alternative
- inventory all relevant existing `LirModule`, `LirFunction`, block,
  parameter, value, type, global, string, extern, specialization, stack-object,
  alloca, intrinsic-requirement, symbol/link identity, initializer, and other
  metadata facts
- compare each existing LIR fact with the legacy-BIR disposition ledger and
  new Raw-BIR schema; record one authoritative typed new-BIR receiving
  container and importer disposition for every row
- add target-independent typed new-BIR containers, builders, views, and Raw
  verification required for lossless receipt of the unchanged LIR surface
- implement a complete explicit LIR variant/metadata dispatcher with
  module-transactional success and stable fail-closed diagnostics
- preserve ordinary identity, type, CFG, object, symbol, initializer, effect,
  and source ordering facts without interpreting target meaning
- preserve inline-asm inputs and outputs as ordinary LIR/BIR operands and
  results using the same value/SSA model as every other node
- record reviewed constraint spellings and requirements such as `r`, `=r`,
  `f`, `VR`, and any other explicitly evidenced form/group against the
  corresponding ordinary operand/result position and role; do not speculate
  beyond evidenced forms
- permit only the smallest evidence-required LIR inline-asm constraint-carrier
  field needed to preserve that position/role/requirement mapping, if the
  existing carrier is proven insufficient
- preserve asm text opaquely and byte-exact for the later assembler; no stage
  in this idea parses mnemonics, placeholders, directives, or encodings
- correct documentation that falsely calls an existing LIR fact a source gap
  and distinguish missing new-BIR container from missing importer wiring
- build an explicit checked coverage matrix mapping every existing variant and
  metadata family to its LIR authority, new-BIR container, importer rule,
  verifier rule, tests, and completion state
- focused and broader proof that all rows import losslessly and malformed input
  rejects without partial publication

## Container Contract

- Every successful row names an existing typed LIR field and a typed Raw-BIR
  destination. Display strings, LLVM text, names, and printer fragments may
  verify parity only when structured authority already exists.
- No semantic identity, opcode, type, CFG edge, initializer topology, effect,
  address-space fact, or relocation may be reconstructed from rendering text.
- If new BIR lacks a receiving container, add it to new BIR. If importer wiring
  is missing, add it to the importer. LIR files, schemas, and producers remain
  unchanged except for the sole evidence-gated minimal inline-asm constraint-
  carrier exception above.
- `InlineAsm` uses ordinary input operands and result values. Constraint
  requirements attach to their existing positions and roles; no
  `InlineAsmOperand`, parallel value subsystem, name-binding table, or special
  allocator is permitted.
- A later shared generic regalloc service may consume ordinary operands/results
  plus recorded requirements. Idea 734 records and transports those facts but
  does not interpret them into allocation, target preparation, or projection
  machinery.
- Unsupported or malformed input rejects the whole module. No partial graph,
  capability, fixup table, or mixed old/new state may escape.
- Raw BIR remains target-independent and unallocated. A container may preserve
  opaque target-authored payload but may not interpret it.

## Out Of Scope

- any edit to LIR files, schemas, variants, metadata, or producers except the
  sole minimal, evidence-proven inline-asm constraint-carrier field described
  above
- treating a README `source gap` label as authorization to repair or redesign
  LIR
- canonical passes or Canonical-BIR behavior
- target selection or interpretation, preparation, constraint parsing/binding,
  or ABI placement
- special inline-asm allocation, generic register-allocation implementation,
  requirement projection machinery, or speculative tie/group architecture
- call lowering beyond preserving existing typed call semantics in Raw BIR
- out-of-SSA, liveness, register allocation, spill/reload, frame layout, or
  MIR-ready publication
- MIR, instruction selection, concrete registers/opcodes, emission, object or
  linker integration, operand substitution, or late assembly
- implementing any general A1-F3 behavior from draft idea 733
- reviving legacy BIR structures or compiling quarantined files merely to
  claim coverage
- accepting unsupported diagnostics, allowlists, expectation rewrites, or
  unsupported downgrades as completion of a missing container/import row

## Acceptance Criteria

- One checked coverage matrix enumerates every existing `LirInst` alternative,
  every `LirTerminator` alternative, and every relevant module/function/type/
  global/value/metadata family with no catch-all or omitted row.
- Every row identifies its existing structured LIR authority, exact typed
  new-BIR container, importer disposition, verifier ownership, and proof.
- Every existing LIR semantic fact imports losslessly into verified Raw BIR;
  completion is rejected while any valid current-LIR row remains merely
  unsupported or lacks a typed new-BIR destination.
- Functions with parameters/non-void results, complete terminators, globals,
  strings, externs, types, stack objects/allocas, initializers, symbol/link
  identities, intrinsic requirements, and every existing ordinary instruction
  family have typed receiving coverage and neighboring positive/negative
  tests.
- Success publishes one verified target-independent Raw BIR module; every
  failure path publishes nothing and reports stable source-located diagnostics.
- No LIR file changes. Build integration contains only migrated new-BIR/importer
  owners and no duplicate legacy authority, except a minimal inline-asm
  constraint-carrier edit when the final evidence proves it necessary.
  Documentation and implementation status match the final matrix and code.
- Inline asm retains byte-exact opaque asm text; ordinary input/output
  identities remain ordinary SSA values; reviewed `r`, `=r`, `f`, `VR`, and
  other explicitly evidenced requirements are recorded only against their
  ordinary positions/roles and survive import without interpretation.
- Supervisor-selected focused and broader regression proof is green.

## Reviewer Reject Signals

- Reject any LIR edit outside the sole evidence-proven minimal inline-asm
  constraint-carrier exception, and reject that exception if current fields
  already preserve the required mapping.
- Reject a dispatcher that handles only named tests or leaves valid current
  variants behind a generic unsupported fallback while claiming completeness.
- Reject using LLVM/printer text, names, integer spellings, or `args_str` to
  invent missing opcode, identity, type, CFG, initializer, relocation, or
  effect semantics.
- Reject unsupported diagnostics, stale `source gap` labels, expectation
  downgrades, allowlist filtering, or classification-only matrix edits claimed
  as container completeness.
- Reject a new container that merely renames a legacy side table, duplicates
  semantic authority, or retains the same missing typed fact.
- Reject `InlineAsmOperand`, a parallel inline-asm value system, a name-binding
  table, a feature allocator, or any collapse of ordinary input/result SSA
  identities.
- Reject target preparation, constraint interpretation/binding, ABI placement,
  canonicalization, requirement projection, allocation, MIR, emission, or
  assembler parsing.
- Reject guessed constraint forms, speculative tie/group architecture, or
  parsing asm text before the later assembler.
- Reject partial publication, string-based fixups, testcase-shaped matching,
  helper-only refactors, or proof that omits neighboring variants/metadata
  families.

## Resumption Record: post-Step 7.24 production computed-goto blocker

Paused after accepted Step 7.24, `Receive typed computed-goto authority`.
The uncommitted coherent receiver slice is limited to
`src/backend/bir/lir_to_bir.cpp`,
`tests/backend/bir/backend_lir_to_bir_interface_test.cpp`, and its `todo.md`
record: it imports the final `LirIndirectBrOp` instruction carrier into Raw-BIR
`IndirectJumpTerm` using only typed `addr_value` and ordered `successors`, with
transactional positive and malformed-authority coverage. Do not re-execute
this receiver packet when this source resumes.

Accepted executor proof: fresh `cmake --build --preset default` passed; the
matching `^backend_` before/after regression guard passed 5/5 under
allow-non-decreasing in `test_before.log` and `test_after.log`. Supervisor
broader validation built freshly and ran
`ctest --test-dir build -j --output-on-failure`: 3029/3034 passed and 5 failed.
Only the reproduced `^llvm_gcc_c_torture_src_comp_goto_1_c$` failure is routed
here: before Raw-BIR import it reports
`LirIndirectBrOp.addr_value: must carry current-function pointer LirValueId`.
The remaining failures (`20040302-1.c`, `20041214-1.c`, `920501-4.c`, and
`920501-5.c`) have no ownership claim in this record.

Classification: `separate-blocker`. The active production computed-goto path
does not populate the same authority that closed 757's bounded handoff proved.
Open `ideas/open/764_lir_production_computed_goto_addr_value_publication.md`
owns only tracing and repairing that first production authority-loss owner,
verifying the carried current-function pointer `LirValueId`, and handing it
back. It must not change Raw-BIR/importer receiver code or recover identity
from display text.

Interrupted point: Step 7.24 is complete pending broader/lifecycle
disposition, not incomplete implementation. Exact return action after 764 has
accepted a producer handoff proving `comp-goto-1.c` publishes valid
`addr_value`: reactivate 734 after Step 7.24 and send the completed receiver
slice to plan-owner for a close/repair decision. Preserve the accepted receiver
work and proof; do not repeat Step 7.24.

## Resumption Update: accepted 764/772 baseline disposition

The Step 7.24 receiver remains accepted and must not be re-executed. Closed
764 delivered its production computed-goto carrier repair, and closed 772
cleared the remaining `pr70460` GEP-pointer baseline blocker; the supervisor
accepted the resulting fresh full-suite candidate at 3037/3037.

This source is not capability-complete: its matrix still lacks structured
PHI incoming value/predecessor authority, followed by the separately scoped
local/object, memory/va, aggregate/vector, body-parameter, and other remaining
families already enumerated above. The earliest eligible dependency is the
already-open `ideas/open/751_lir_phi_incoming_value_and_predecessor_identity.md`:
it owns only the PHI carrier authority that this importer cannot obtain from
text, and its sole predecessor, closed 750, has accepted current-function CFG
block identities.

Return point: activate 751 now. After its accepted typed handoff, reactivate
734 for plan-owner repair of one bounded Raw-BIR PHI receiver derived from that
handoff; preserve accepted Steps 7.20 through 7.24 and do not absorb the later
authority families into that receiver.

## Resumption Update: accepted 751 PHI authority handoff

Closed 751 is capability-complete in `6ece9fe8f`. Its typed
`LirPhiIncoming` carrier publishes each incoming `LirOperand` value and
current-function `LirBlockId` predecessor for ternary, logical, AArch64-vaarg,
and AMD64-vaarg PHI producers; its verifier rejects missing, unknown,
cross-function, and predecessor/edge-mismatched authority. The accepted
focused proof passed 1/1, and the supervisor full after proof and matching
full regression guard both passed 3037/3037.

Repair and execute exactly one next receiver packet, Step 7.25: receive the
typed `LirPhiOp` result/type and ordered closed-751 incoming value/predecessor
pairs into a Raw-BIR PHI container. Bind every incoming to an exact existing
predecessor-to-destination CFG edge occurrence, preserve multiplicity/order,
verify ownership/type/coherence and transactional rollback, and never recover
identity from value text, labels, printer output, LLVM text, or instruction
order. Do not repeat Steps 7.20 through 7.24 or absorb local/object, memory/va,
aggregate/vector, body-parameter, or any other remaining family. Return to
the source completion gate after this one bounded receipt.

## Resumption Record: Step 7.25 special-token authority blocker

Last accepted progress remains Steps 1 through 7.24, including closed 751's
typed PHI incoming value/predecessor handoff (`6ece9fe8f`). No Step 7.25
implementation, focused PHI-receiver proof, or implementation commit is
accepted. The focused frontend/interface baseline and attempted exact build plus
`^backend_lir_to_bir_interface$` proof did pass, but that test did not exercise
the new PHI path and is not accepted Step 7.25 proof.

Interrupted step: Step 7.25, `Receive typed PHI incoming authority`. The first
blocking fact is outside this source's Raw-BIR receiver scope:
`LirOperandKind::SpecialToken` has only `std::monostate` authority in
`src/codegen/lir/operands.hpp`. `LirOperandAuthority` can presently carry only
`LirValueId`, `LinkNameId`, or `LirIntegerImmediate` in addition to
`monostate`; SpecialToken's distinguishing content is therefore display
spelling/classification, not native semantic authority. DirectConstant and
Immediate have their respective typed authority, but SpecialToken does not.
Step 7.25 cannot receive and preserve a typed special-token PHI input without
forbidden presentation recovery.

Classification: `separate-blocker`. Open
`ideas/open/786_lir_phi_special_token_semantic_authority_publication.md` owns
only publication and verification of native semantic authority for the relevant
PHI SpecialToken operands. It must not change the Raw-BIR receiver, backend
lowering, or recover semantics from token text. Exact return point: after 786
accepts its producer/verifier handoff, reactivate 734 at the unchanged Step
7.25 and receive exactly the newly authorized typed SpecialToken incoming rows
alongside the already accepted 751 value/predecessor authority, with existing
edge-occurrence and transactional requirements. The remaining action is the
single bounded Step 7.25 receiver packet; do not repeat Steps 1 through 7.24
or treat the baseline/interface test as PHI receipt proof.

## Resumption Record: Step 7.25 parallel-CFG-edge verifier blocker

Last accepted progress remains Steps 1 through 7.24, including the typed
computed-goto receiver and the accepted producer handoffs from closed 751
(`6ece9fe8f`) for PHI incoming value/predecessor authority and closed 786
(`91b5bde43`) for native PHI SpecialToken semantic authority. Step 7.25,
`Receive typed PHI incoming authority`, is interrupted and remains unaccepted.

The current Raw-BIR PHI receiver worktree changes and its focused
`test_after.log` result (1/1) are unaccepted WIP, not an implementation commit
or acceptance evidence. `test_before.log` passed 1/1; the focused after result
does not establish the required parallel-edge route and must not be treated as
acceptance. Preserve this WIP without committing it, and do not repeat or
retroactively accept any of it when this source resumes.

The blocker investigation was outside this source's scope. It confirmed that
the typed-LIR verifier already validates duplicate `LirCondBr` successors and
duplicate ordered `LirSwitch.case_successors` per occurrence; closed 787
(`a889ce33f`) supplies the focused positive coverage proving they remain
distinct ordered parallel CFG-edge occurrences. Idea 734 still must not make
LIR file/schema/producer edits.

Classification: resolved separate blocker. Closed
`ideas/closed/787_lir_parallel_cfg_edge_verifier_admission.md` owns the typed
LIR verification/admission conclusion and handoff of duplicate conditional-
branch and switch-case successor occurrences as distinct ordered CFG edges.
It accepted no Raw-BIR receiver/container/importer work, broader producer
semantics, target lowering, or source/presentation recovery.

Exact return point: reactivate 734 at the unchanged Step 7.25 and reattempt
only complete PHI receiver coverage, including exact parallel edge occurrences.
The remaining action is that one bounded PHI receiver packet; preserve
accepted Steps 1 through 7.24 and do not treat the current WIP as accepted.

## Resumption Record: post-partial-Step-7.25 PHI incoming occurrence-identity blocker

Last accepted progress is historical Steps 1 through 7.24; closed 751's typed
PHI incoming value/predecessor handoff (`6ece9fe8f`); closed 786's native PHI
SpecialToken handoff (`91b5bde43`); closed 787's typed parallel conditional and
switch successor-occurrence handoff (`a889ce33f`); and the safe partial
Raw-BIR Step 7.25 receiver in `006d79aaf`. The latter receives unambiguous PHI
edges, reserves loop-backedge results, preserves native SpecialToken authority,
retains Raw-BIR successor-occurrence topology, and fails closed before
publication when a predecessor-only PHI row is ambiguous. Its supervisor
acceptance is a fresh build, matching `^backend_` 5/5 before-and-after
non-decreasing guard (`--allow-non-decreasing-passed`), and fresh full `ctest`
3037/3037 passing. It is accepted safe partial receiver progress, not full Step
7.25 completion.

Interrupted step: Step 7.25, `Receive typed PHI incoming authority`. The
remaining exact-parallel portion cannot be implemented in this source: each
`LirPhiIncoming` carries typed incoming value and predecessor `LirBlockId`, but
no typed successor-occurrence ID selecting one duplicate conditional or switch
edge from that predecessor. Raw-BIR must not infer this identity from PHI
incoming order, labels, spelling, printer output, LLVM text, or instruction
order.

Classification: `separate-blocker`. Open
`ideas/open/788_lir_phi_incoming_successor_occurrence_identity.md` owns only
the selected LIR PHI producer/schema/verifier publication and handoff of a
native per-incoming CFG successor-occurrence identifier. It must not edit
Raw-BIR code, backend importer/container/verifier, target lowering, or later
LIR families.

Exact return point: after 788 has an accepted typed handoff, reactivate 734 at
the unchanged Step 7.25 and repair/complete only the parallel-edge portion of
the Raw-BIR PHI receiver, consuming its occurrence authority alongside closed
751 value/predecessor and closed 786 SpecialToken authority. Preserve
`006d79aaf`; do not repeat Steps 7.20 through 7.24 or the already accepted
unambiguous/loop-backedge partial receipt.

## Resumption Update: accepted 788 successor-occurrence handoff

Closed 788 is capability-complete. Its accepted commits `f52ced6ae` and
`10d63ff7a` publish and verify native
`LirSuccessorOccurrenceId` in
`LirPhiIncoming.successor_occurrence` for the selected ternary, logical,
AArch64-vaarg, and AMD64-vaarg direct PHI producers. The ID selects the exact
typed predecessor terminator occurrence; verifier admission requires its
presence, validity, current-function predecessor ownership,
predecessor/destination coherence, and unique exact destination-edge coverage
with native multiplicity/order. Conditional/switch true/false/default/case
parallel occurrences remain distinct. Missing, invalid, foreign,
predecessor-mismatched, destination-mismatched, duplicate, and incomplete
coverage fails closed. Accepted proof is focused `frontend_lir_call_type_ref`
1/1, matching `^backend_` before/after 5/5 non-decreasing, and fresh full
CTest 3037/3037.

Resume only unchanged Step 7.25's parallel-edge Raw-BIR PHI receiver portion:
consume `successor_occurrence` with closed 751 value/predecessor and closed
786 SpecialToken authority. Preserve accepted `006d79aaf`; do not repeat
Steps 7.20–7.24 or the accepted unambiguous/loop-backedge receiver work.

## Runbook Exhaustion Decision: post-Step 7.25 complete PHI receipt

Close rejected. Commit `7dc03f23a` completes the remaining parallel-edge
portion of Step 7.25 and, together with accepted partial `006d79aaf`, now
receives the selected structured PHI rows without presentation recovery. The
accepted proof is focused `backend_lir_to_bir_interface` 1/1, the matching
`^backend_` before/after guard 5/5 non-decreasing, and fresh full CTest
3037/3037. The prerequisite handoffs remain closed 751 (`6ece9fe8f`, incoming
value/predecessor), 786 (`91b5bde43`, SpecialToken), and 788 (`f52ced6ae`,
`10d63ff7a`, successor occurrence).

This does not meet the source completion gate. Valid current-LIR families
still lack an evidenced typed Raw-BIR receiver disposition, including
local/stack/object and general pointer authority; remaining memory/va-list;
aggregate/vector; body-parameter; module/type/global/metadata; and other
unreceived instruction, terminator, and inline-assembly rows. Therefore this
source cannot yet claim a complete checked matrix, a complete explicit
dispatcher, or lossless receipt of every current-LIR semantic fact.

Classification: `separate-blocker`. The already-open
`ideas/open/752_lir_local_object_pointer_authority_convergence.md` is the
earliest executable successor. It owns only the missing current-function
local-object, pointer-definition, and lifetime authority for representative
alloca/local load/store/GEP and VLA stack-lifetime routes; it must not edit
Raw-BIR/importer code or recover identity from local spellings. The active
runbook is switched to 752 as part of this decision.

Resumption record: completed receiver progress is Steps 1 through 7.25,
including accepted `006d79aaf` and `7dc03f23a`; do not repeat any of it. The
interrupted return point is the first bounded Raw-BIR receiver row authorized
by 752's accepted handoff. After 752 closes with its typed authority and proof,
reactivate 734, repair its runbook from that handoff, and receive exactly that
row with importer dispatch, reachable verification, and transactional
positive/negative coverage. Leave memory/va-list, aggregate/vector,
body-parameter, module/type/global/metadata, other instruction/terminator,
and inline-assembly families separately scoped and fail-closed.

## Runbook Exhaustion Decision: post-Step 7.26 selected alloca receipt

Close rejected. Commit `2cce9da69` receives exactly the one closed-752
selected-hoisted `LirAllocaOp` row from its typed result and
`local_object_authority` into the Raw-BIR container, builder, view, importer,
and reachable verifier, with positive and malformed transactional interface
coverage. Supervisor acceptance is a fresh prescribed build plus
`^backend_lir_to_bir_interface$` 1/1, a matching non-decreasing
`test_before.log`/`test_after.log` guard, and fresh broader `^backend_` 5/5.

This does not meet the source completion gate. Unmet source criteria are:

- the no-omission checked coverage matrix and its per-row authority,
  destination, importer, verifier, and proof disposition;
- lossless verified Raw-BIR receipt of every valid current-LIR semantic fact,
  plus the complete explicit dispatcher and neighboring coverage required for
  that receipt;
- typed receiving coverage for the still-unreceived local load/store/GEP and
  VLA lifetime, named/local-temporary, memory/va-list, aggregate/vector,
  body-parameter, module/type/global/metadata, other instruction/terminator,
  and inline-assembly families;
- final whole-module transactional success/failure evidence, final
  documentation-to-matrix/code convergence, and the source-wide focused and
  broader regression acceptance required after all rows land.

Classification: `separate-blocker`. Closed 752 deliberately authorizes only
the alloca row as its sole first Raw-BIR return packet; it orders local
load/store/GEP and VLA lifetime later but supplies no exact second
receiver-ready row. New open idea
`ideas/open/789_lir_local_operation_receiver_handoff_completion.md` owns one
bounded next local-operation authority handoff only; it must not edit Raw-BIR
or recover identity from local spelling.

Resumption record: Steps 1 through 7.25 remain historical complete work, and
Step 7.26 is complete. The interrupted/return point is `Step 7.27 - Receive
the exact first post-alloca local-operation authority row`, which must not be
invented from closed 752's representative scope. After 789 accepts a handoff
naming that one row, reactivate 734 and repair its runbook at Step 7.27 to
receive only the handed-off structured fields with importer dispatch,
reachable verification, and transactional positive/negative coverage.
Preserve commits `006d79aaf`, `7dc03f23a`, and `2cce9da69` and the proof
references above; do not repeat their receiver work. Later local/VLA variants
and all remaining families stay separately scoped and fail-closed.

## Runbook Exhaustion Decision: post-Step 7.27 local-scalar load receipt

Close rejected. Commit `eabf7a3b8` receives exactly closed 789's selected
direct non-array/non-VLA local-scalar `LirLoadOp` authority into typed Raw-BIR,
with native result admission, result/pointer/object/owner/type/liveness
validation, reachable verifier coverage, and transactional positive/negative
cases. The supervisor acceptance evidence is a fresh build plus
`ctest --test-dir build -j --output-on-failure -R '^backend_'` passing 5/5 and
a matching non-decreasing canonical guard.

This one receiver row does not meet the source completion gate. The no-omission
matrix, lossless verified receipt of every valid current-LIR fact, complete
dispatcher and neighboring coverage, final whole-module transactional proof,
and the remaining local store/GEP/VLA, named/local-temporary, memory/va,
aggregate/vector, body-parameter, module/type/global/metadata, other
instruction/terminator, and inline-assembly rows remain unmet.

Classification: `separate-blocker`. The earliest remaining local-operation
family is store/GEP/VLA, but neither closed 752 nor closed 789 authorizes a
second exact receiver row. New open idea
`ideas/closed/790_lir_next_local_operation_receiver_handoff.md` owns choosing,
publishing, verifying, and handing off exactly one next valid local-operation
row. It must not edit Raw-BIR/importer code or derive authority from local
spellings.

Resumption record: Steps 1 through 7.26 remain accepted historical work,
including `006d79aaf`, `7dc03f23a`, and `2cce9da69`; Step 7.27 is accepted in
`eabf7a3b8`. Closed 790 accepted the exact next direct non-array/non-VLA
integer local scalar declaration `LirStoreOp` authority in `727949c9c`, with
focused fresh `^frontend_lir_call_type_ref$` proof passing 1/1 and a matching
non-decreasing canonical guard. Resume now at `Step 7.28 - Receive the
selected direct local-scalar LirStoreOp authority`, consuming only the native
immediate, type, pointer definition, and checked local-object
owner/type/liveness facts documented in
`docs/lir_local_operation_authority/handoff_to_734.md`. Add only its typed
Raw-BIR destination, importer dispatch, reachable verification, and
transactional coverage. Do not repeat accepted alloca/load receipt or absorb
assignment/SSA/pointer/aggregate/vector/array/VLA stores, GEP, or later
families.

## Runbook Exhaustion Decision: post-Step 7.28 local-scalar store receipt

Close rejected. Commit `f5cda70ee` receives exactly closed 790's selected
direct non-array/non-VLA integer local scalar declaration `LirStoreOp` into a
typed Raw-BIR local-store destination. It preserves the native integer
immediate and checked pointer-definition, object, owner, type, and liveness
authority; malformed selected-store input rejects transactionally. The
supervisor-provided fresh acceptance proof is
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`,
passing 5/5 and recorded in `test_after.log`.

This bounded receipt does not satisfy the source completion gate. The checked
no-omission matrix, lossless verified receipt of every valid current-LIR fact,
complete explicit dispatcher, whole-module transactional proof, and final
documentation-to-code convergence remain unmet. In particular, no exact next
receiver-ready local row is authorized for the remaining local GEP/VLA,
named/local-temporary, or nonselected store families; all remain fail closed.
The separately scoped memory/va-list, aggregate/vector, body-parameter,
module/type/global/metadata, other instruction/terminator, and inline-assembly
families also remain unreceived.

Classification: `separate-blocker`. New open idea
`ideas/open/791_lir_next_local_operation_receiver_handoff.md` owns selecting,
publishing, verifying, and handing off exactly one next valid local-operation
authority row after the accepted store. It must not edit Raw-BIR/importer code
or derive authority from local spelling.

Resumption record: Steps 1 through 7.28 are accepted, including receiver
commits `006d79aaf`, `7dc03f23a`, `2cce9da69`, `eabf7a3b8`, and `f5cda70ee`,
plus closed 790's prerequisite authority `727949c9c`. The exact return point
is `Step 7.29 - Receive the one 791-authorized local-operation authority row`.
After 791 closes with its typed handoff and focused producer proof, reactivate
734, repair its runbook for that one receiver row, and preserve all accepted
work. Do not repeat Step 7.28 or absorb other local/VLA, memory/va,
aggregate/vector, body-parameter, module/type/global/metadata, instruction,
terminator, or inline-assembly families.

## Resumption Record: accepted 791 local-array GEP authority handoff

Closed 791 is capability-complete in `ea579c648`. It authorizes exactly the
direct static-local-array `LirGepOp` with a valid current-function result,
native SSA base pointer, matching element type, one native `i64` immediate
index, and checked live local-object authority. Missing admission/local
authority, raw or SSA index, invalid result/base, foreign or incoherent
object/owner, type mismatch, and dead authority fail closed before downstream
use. Display spelling, `%t`, rendered operands, printer output, and LLVM text
remain nonsemantic.

The supervisor accepted a fresh build, focused
`^frontend_lir_call_type_ref$` 1/1, matching non-decreasing canonical
before/after 1/1 guard, broader `^frontend_cxx_` 1/1 and `^backend_` 5/5;
direct review found no material issue. Resume only `Step 7.29 - Receive the
selected direct static-local-array LirGepOp authority`: consume the documented
result, element type, base pointer, immediate index, and local-object fields
into one typed Raw-BIR destination with importer dispatch, reachable
verification, and transactional positive/negative coverage. Do not repeat
Steps 7.26 through 7.28 or absorb SSA-indexed/local-temporary/
aggregate-member/VLA GEPs, stores, later loads, stack lifetime, or any other
family.

## Runbook Exhaustion Decision: post-Step 7.29 local-array GEP receipt

Close rejected. Commit `4ab2deb7e` receives exactly closed 791's selected
direct static-local-array `LirGepOp` authority into a typed Raw-BIR
destination, importer dispatch, reachable verifier, and transactional
positive/malformed interface coverage. Supervisor acceptance is a fresh build,
focused `^backend_lir_to_bir_interface$` proof passing 1/1, and a matching
non-decreasing `^backend_` before/after regression guard passing 5/5 to 5/5;
the direct scope/diff review is accepted. Closed 791's producer handoff remains
`ea579c648`.

This one bounded receiver row does not satisfy the source completion gate.
Unmet criteria are the no-omission checked coverage matrix and per-row typed
authority/destination/importer/verifier/proof disposition; lossless verified
Raw-BIR receipt of every valid current-LIR semantic fact; a complete explicit
dispatcher with neighboring coverage; and final whole-module transactional,
documentation-convergence, focused, and broader acceptance proof. In
particular, valid remaining local/VLA, named/local-temporary, nonselected
store/GEP, memory/va-list, aggregate/vector, body-parameter,
module/type/global/metadata, other instruction/terminator, and inline-assembly
families are still unreceived and fail closed.

Classification: `separate-blocker`. New open idea
`ideas/open/792_lir_next_local_operation_receiver_handoff.md` owns selecting,
publishing, verifying, and handing off exactly one next valid local-operation
row after the accepted local-array GEP. It must not edit Raw-BIR/importer code
or derive authority from local spelling, formatted operands, printer output,
LLVM text, or testcase identity.

Resumption record: Steps 1 through 7.29 are accepted, including receiver
commits `006d79aaf`, `7dc03f23a`, `2cce9da69`, `eabf7a3b8`, `f5cda70ee`, and
`4ab2deb7e`; prerequisite handoffs include closed 791's `ea579c648`. Do not
repeat those packets. The exact return point is `Step 7.30 - Receive the one
792-authorized local-operation authority row`. After 792 closes with an exact
typed handoff and accepted producer proof, reactivate 734 and repair its
runbook for only that receiver row: add the minimum typed Raw-BIR destination,
importer dispatch, reachable verification, and transactional positive/negative
coverage. Do not absorb any other local/VLA or later family without a separate
producer-authorized handoff.

## Runbook Exhaustion Decision: post-Step 7.30 VLA stack-save receipt

Close rejected. Commit `2323afb91` receives exactly closed 792's selected VLA
`LirStackSaveOp` saved-stack-pointer authority into a typed Raw-BIR node,
transactional importer dispatch, reachable verifier, and nearby backend
coverage. Supervisor acceptance is a fresh build, `^backend_` proof 5/5, and
a matching non-decreasing canonical regression guard 5/5 to 5/5. Closed 792's
producer handoff is `900a42bfd` and `89f4d7f85`; its closure is `ecccb195f`.

This bounded row does not satisfy the source completion gate. The no-omission
checked coverage matrix and per-row authority/destination/importer/verifier/
proof dispositions remain incomplete; lossless verified receipt of every
valid current-LIR semantic fact, a complete explicit dispatcher, and final
whole-module transactional, documentation-convergence, focused, and broader
proof remain unproven. Remaining local/VLA (stack restore, dynamic VLA
allocation, VLA GEP, nonselected operations, and local temporaries), memory/
va-list, aggregate/vector, body-parameter, module/type/global/metadata,
instruction/terminator, and inline-assembly families remain fail closed.

Classification: `separate-blocker`. The already-open umbrella
`ideas/open/793_lir_to_new_bir_remaining_coverage_umbrella.md` is the next
executable route. It owns post-Step 7.30 evidence classification and an
ordered, first-owner successor queue; it must not implement Raw-BIR or LIR
changes. This source is paused after accepted Steps 1 through 7.30, including
receiver commits `006d79aaf`, `7dc03f23a`, `2cce9da69`, `eabf7a3b8`,
`f5cda70ee`, `4ab2deb7e`, and `2323afb91`.

Exact return point: 793 concluded with its evidence summary, ownership
classification, and ordered bounded successor queue in
`docs/lir_to_new_bir_remaining_coverage/successor_queue.md`. Its first
successor, now closed 794, accepted one exact typed local/VLA handoff. The
active runbook now repairs 734 for only that matching receiver row, preserving
Steps 1 through 7.30. The queue did not preselect stack restore, dynamic VLA
allocation, VLA GEP, or any other row from unresolved or text/monostate
classifications; 794's completed selected-only handoff is the sole authority
for this resumed packet.

## Resumption Record: selected stack-restore authority completion

Closed idea 794 completed the first successor handoff with selected-only
native `LirStackRestoreOp` authority. Its closed record and the exact receiver
contract are `ideas/closed/794_lir_next_local_vla_authority_handoff.md` and
`docs/lir_local_operation_authority/handoff_to_734.md`. Accepted producer
contract/implementation/handoff commits are `8bd881842`, `cdeacb2cd`, and
`f5cfa52b7`; 794's final focused verification record is `b2e2e5594`, with
`^frontend_lir_call_type_ref$` passing 1/1 after a fresh build.

Exact return action: resume only at **Step 7.31 - Receive the selected VLA
`LirStackRestoreOp` authority**. Receive only the selected admission,
`saved_ptr`, checked current-function local object/owner/pointer-type/
pointee-type/live checkpoint binding, and
`RestoreSavedVlaStackCheckpoint` transition into one typed Raw-BIR receiver
path with transactional positive/negative coverage. Do not repeat Steps 1
through 7.30 or absorb dynamic-VLA count/allocation, VLA GEP, other local or
lifetime rows, Raw-BIR work beyond this one receiver packet, target lowering,
MIR, or presentation-derived recovery. This resumption record authorizes the
packet; it does not claim that the receiver work has occurred.

## Runbook Exhaustion Decision: post-Step 7.31 VLA stack-restore receipt

Close rejected. Commit `d411ff989` receives exactly closed 794's selected VLA
`LirStackRestoreOp` authority into a typed Raw-BIR stack-restore destination,
transactional importer dispatch, reachable verification, and nearby positive/
negative coverage. Supervisor route review found the selected typed restore
receipt scope-correct and transactionally validated. Its fresh build and exact
`^backend_` proof passed, and the documented allow-non-decreasing backend
before/after guard is non-regressive at 5/5 to 5/5 with no new failures.

This bounded row does not satisfy this source's completion gate. The checked
coverage matrix still has valid current-LIR families without an evidenced
typed Raw-BIR receiver disposition, including remaining memory/va,
aggregate/vector, body-parameter, module/type/global/metadata,
instruction/terminator, and inline-assembly families; no-omission matrix
completion, explicit dispatcher completeness, whole-module transactional
proof, and documentation convergence remain unproven. Do not infer their
authority from text, names, `monostate`, or unclassified operands.

Classification: `separate-blocker`. The next dependency-ordered first owner
is existing open idea
`ideas/open/753_lir_memory_va_pointer_authority_convergence.md`. This source
is parked after accepted Steps 1 through 7.31, including `d411ff989`. After
753 closes with an exact selected one-row producer/verifier handoff and
accepted proof, reactivate 734 and repair its runbook at **Step 7.32** for only
that matching typed Raw-BIR receiver row. Do not repeat Step 7.31 or absorb
other memory/va, local/VLA, aggregate/vector, parameter, module/type/global,
instruction/terminator, inline-assembly, or later families.

## Runbook Exhaustion Decision: post-Step 7.32 AMD64 aggregate VA-arg overflow receipt

Close rejected. Commit `cf8c05985` receives exactly closed 753's selected
non-volatile direct-local AMD64 SysV aggregate-overflow `va_arg` `LirMemcpyOp`
carrier into a typed Raw-BIR receipt. The accepted evidence is a fresh build,
focused `^backend_lir_to_bir_interface$` proof, and matching non-decreasing
`^backend_` guard, each passing 5/5. This proves only the selected overflow
carrier and does not authorize other memory/VA or source-wide rows.

The unmet source criteria remain the no-omission checked coverage matrix and
its per-row typed authority/destination/importer/verifier/proof disposition;
lossless verified Raw-BIR receipt of every valid current-LIR semantic fact;
complete explicit dispatcher and neighboring coverage; and final
whole-module transactional, documentation-convergence, focused, and broader
acceptance proof. Valid unreceived families include the remaining memory/VA,
aggregate/vector, body-parameter, module/type/global/metadata, residual
instruction/terminator, and inline-assembly families. They remain fail-closed;
no authority may be inferred from text, names, rendered operands, `monostate`,
or unclassified values.

Classification: `separate-blocker`. The dependency-ordered next first owner
is existing open idea
`ideas/open/763_lir_composite_type_ref_model.md`, as recorded in
`docs/lir_to_new_bir_remaining_coverage/successor_queue.md`. It owns the
structured composite `LirTypeRef` prerequisite; it is not a 734 receiver row.

Resumption record: Steps 1 through 7.32 are accepted, including receiver
commit `cf8c05985` and the prior accepted history recorded above. The exact
return point is not preselected: after 763 has accepted its structured
composite-type model, follow the dependency-ordered queue and obtain an exact
producer/verifier handoff for one later receiver row before reactivating 734.
Do not repeat Step 7.32, receive a residual row merely because 763 closes, or
absorb remaining memory/VA, aggregate/vector, parameter, module/type/global,
instruction/terminator, or inline-assembly work without its separately scoped
first owner and checked handoff.

## Resumption Record: closed 816 inline-assembly output-only authority

The prior composite-type prerequisite and later dependency queue do not
authorize a receiver by themselves. Closed idea 816 now supplies the required
checked one-row producer/schema/verifier handoff. Its closure commit
`1b04a886e` records that `StmtEmitter::emit_inline_asm` publishes only a
scalar-integer output-only `LirInlineAsmOp.ordinary_results[0]` with a fresh
native value ID, typed `Output` binding at index 0, and its `LirTypeRef`.
The exact verifier tuple requires a valid ID owned by the current function;
missing, invalid, duplicate, role-mismatched, index-mismatched,
type-mismatched, and foreign forms reject. Compatibility `result` remains
presentation-only.

Steps 1 through 7.32 remain accepted historical work and must not be repeated.
Resume at **Step 7.33 - Receive the selected inline-assembly output-only
authority**. Receive only that handoff into the minimum typed Raw-BIR
destination, importer dispatch, and reachable verifier path, with transactional
positive and malformed-authority rejection coverage. Do not derive value or
type from `result`, operands, templates, constraints, or other presentation
text; do not receive input, read/write, non-integer, multi-result, or any other
inline-assembly form. Reapply the source completion gate after this bounded
receipt; all remaining source families stay separately owned and fail closed.

## Runbook Exhaustion Decision: post-Step 7.33 inline-assembly output-only receipt

Close rejected. Commit `3b8870e2b` receives exactly closed 816's selected scalar-integer output-only `LirInlineAsmOp.ordinary_results[0]` into typed Raw-BIR using only the structured SSA ID, typed Output/index-0 binding, and `LirTypeRef`. Supervisor route review found no presentation-derived admission. The accepted evidence is a fresh build, focused `^backend_lir_to_bir_interface$` proof, and broader `^backend_` proof; malformed authority rejects transactionally and all nonselected inline-assembly forms remain fail closed.

This one receiver row does not satisfy the source completion gate: the no-omission coverage matrix, per-row typed authority/destination/importer/verifier/proof disposition, complete explicit dispatcher, whole-module transactional proof, and documentation convergence remain unproven. Valid unreceived families include function-body parameter use identity, remaining memory/VA, aggregate/vector, module/type/global/metadata, residual instruction/terminator, and other inline-assembly forms. No receiver may infer authority from compatibility fields, text, names, rendered operands, `monostate`, or unclassified values.

Classification: `separate-blocker`. The queue's listed body-parameter owner `ideas/open/795_lir_body_parameter_authority_handoff.md` is already complete for an unrelated baseline return and supplies no 734 handoff. New open idea `ideas/open/817_lir_body_parameter_receiver_authority_handoff.md` owns one fresh producer/schema/verifier selection and handoff for exactly one function-body parameter use row. This source is paused after accepted Steps 1 through 7.33, including `3b8870e2b`. After 817 closes with an exact selected one-row structured handoff and accepted focused proof, reactivate 734 and repair its runbook only for that matching Raw-BIR receiver row. Do not repeat Step 7.33 or absorb other parameter forms, memory/VA, aggregate/vector, module/type/global, instruction/terminator, or inline-assembly work.

## Resumption Record: closed 817 direct-pointer body-parameter authority

Closed idea 817 is capability-complete for its bounded producer/schema/verifier
handoff in commit `613f947b5`. It authorizes exactly one direct, non-expanded
pointer parameter: the current-function `LirGepOp.ptr` typed-GEP base for
`p[0]`. The complete permitted receiver row is exactly `LirValueId`, parameter
index, pointer `LirTypeRef`, current `LinkNameId` owner, and explicit
`LirNativeBodyParameterAbi::DirectPointer`. Scalar, byval/aggregate,
HFA/vector, array, variadic, and every other parameter form remain rejected
and fail closed. Spelling, signature text, raw operands, and diagnostics are
not authority.

The supervisor accepted a fresh `cmake --build --preset default`, focused
`ctest --test-dir build -j --output-on-failure -R '^backend_lir_selected_pointer_authority$'`,
and matching before/after regression guard with non-decreasing 1/1 pass count.

Exact return action: reactivate 734 only for one later single Raw-BIR receipt
of this structured row. Repair the runbook for that one receiver packet; do
not repeat Step 7.33 and do not receive any other parameter form or perform
producer/receiver work beyond this row.

## Runbook Exhaustion Decision: post-Step 7.34 direct-pointer body-parameter receipt

Close rejected. Commit `8418036b1` receives exactly closed 817's selected
direct non-expanded pointer `LirGepOp.ptr` body-parameter authority into a
typed Raw-BIR GEP-base destination, importer dispatch, reachable verifier, and
transactional positive/malformed-authority coverage. The supervisor accepted a
fresh `cmake --build --preset default`, focused
`^backend_lir_to_bir_interface$` proof passing 1/1, and matching broader
`^backend_` guard passing 6/6 without a regression. This proves one bounded
body-parameter receiver row only.

The source completion gate remains unmet: its checked no-omission matrix and
per-row typed authority/destination/importer/verifier/proof dispositions;
lossless verified receipt of every valid current-LIR semantic fact; complete
explicit dispatcher and neighboring coverage; and final whole-module
transactional, documentation-convergence, focused, and broader acceptance
proof. Valid unreceived families include the remaining body-parameter forms,
memory/VA, aggregate/vector, module/type/global/metadata, residual
instruction/terminator, and inline-assembly forms. They remain fail-closed;
no receiver may reconstruct authority from text, names, rendered operands,
`monostate`, or unclassified values.

Classification: `separate-blocker`. New open idea
`ideas/open/818_lir_next_body_parameter_authority_handoff.md` owns tracing,
publishing, verifying, and handing off exactly one next valid function-body
parameter-use row. It must not edit Raw-BIR/importer code, choose a row from
presentation fields, reopen the accepted direct-pointer row, or absorb any
other family.

Resumption record: Steps 1 through 7.34 are accepted historical work, most
recently receiver commit `8418036b1`; Step 7.34's accepted proof is the fresh
build, focused 1/1 `^backend_lir_to_bir_interface$` result, and broader 6/6
`^backend_` guard stated above. After 818 closes with an exact structured
one-row handoff and accepted focused producer proof, reactivate 734 and repair
its runbook at **Step 7.35 - Receive the one 818-authorized body-parameter
authority row**. Add only that row's typed Raw-BIR destination, importer
dispatch, reachable verification, and transactional positive/negative
coverage. Do not repeat Step 7.34 or absorb other parameter, memory/VA,
aggregate/vector, module/type/global/metadata, instruction/terminator, or
inline-assembly work.

## Resumption Record: closed 818 scalar binary-LHS body-parameter authority

Closed idea 818 is capability-complete. Its Step 1 selection trace
(`0864c8aed`) accepted exactly closed 819's plain fixed scalar `LirBinOp.lhs`
row as receiver-ready; 819's accepted producer/schema/verifier implementation
and proof are `b16935c69`, with `^backend_` 6/6 and matching before/after
guard showing no new failures. The sole receiver-consumable authority is the
matching native `LirValueId`, parameter index, `LirTypeRef`, current-function
`LinkNameId` owner, `LirNativeBodyParameterAbi::DirectScalar`, explicit `Lhs`
role, and matching `LirBinOp.lhs` SSA value/type. Missing, invalid, duplicate,
foreign-owner, out-of-range, non-scalar, type-or-ABI-incoherent, wrong-role,
and lhs value/type-mismatch forms reject transactionally; presentation fields
are forbidden.

Exact return action: resume only at **Step 7.35 - Receive the one
818-authorized body-parameter authority row**. Add only its typed Raw-BIR
destination, importer dispatch, reachable verification, and transactional
positive/negative coverage. No Raw-BIR receipt occurred in 818; do not repeat
Steps 1 through 7.34 or receive another parameter form.

## Resumption Record: DirectScalar producer/verifier publication blocker

Paused after accepted Steps 1 through 7.34, most recently the direct-pointer
body-parameter receipt in `8418036b1`. Step 7.35, **Receive the one
818-authorized body-parameter authority row**, has uncommitted receiver work
only and is not accepted progress or acceptance evidence. Its importer-side
repair preserves the selected i32 `LirBinOp.lhs` receipt and makes unselected
producer-emitted DirectScalar rows avoid generic SSA-definition receipt; the
matching `^backend_` before/after proof is 6/6 non-regressive.

Fresh full CTest nevertheless fails the external
`llvm_gcc_c_torture_src_20041011_1_c` case before BIR import in
`src/codegen/lir/verify.cpp`: `LirFunction.native_body_parameter_definitions`
requires a native direct-pointer or direct-scalar current-function parameter
identity and type. This missing `ull` DirectScalar producer/verifier
publication is outside 734's Raw-BIR receiver scope. Open blocker
`ideas/closed/820_lir_directscalar_parameter_producer_verifier_publication.md`
owns only that existing typed authority publication and fail-closed LIR
validation; it does not authorize generic BIR scalar parameters or a broader
DirectScalar family.

Exact return action after blocker acceptance: reactivate 734 at **Step 7.35**,
retain only the selected DirectScalar `LirBinOp.lhs` receiver, and rerun its
fixed `^backend_` proof plus the full checkpoint. Do not treat the current
uncommitted Step 7.35 work as accepted, repeat Step 7.34, receive any other
parameter form, or classify presentation text as authority.

## Resumption Record: closed 820 DirectScalar producer/verifier publication

Closed idea 820 is capability-complete. Its accepted implementation
`4bcc7c8ff` publishes the existing native `ull` DirectScalar current-function
identity and matching typed mirror, while retaining fail-closed rejection for
missing, foreign, and type-incoherent authority. That accepted slice included
the exact external `^llvm_gcc_c_torture_src_20041011_1_c$` guard and matching
before/after regression evidence. A fresh current build reported no work and
the independently attributable focused
`^frontend_lir_function_signature_type_ref$` checkpoint passed 1/1 with the
DirectScalar alias-boundary coverage from `4bcc7c8ff`.

The current composite external pass remains diagnosis only: it includes
unaccepted Ideas 821/822 selector work and unaccepted 734-shaped receiver
work. It grants no acceptance to those changes. The unavailable `^backend_`
executables likewise provide no proof.

Exact return action: reactivate 734 only at **Step 7.35 - Receive the one
818-authorized body-parameter authority row**. Retain only the selected
DirectScalar `LirBinOp.lhs` receiver authorization, preserve all uncommitted
receiver and selector work as unaccepted, and rerun Step 7.35's fixed
`^backend_` proof plus its full checkpoint when its implementation packet is
ready. No generic scalar authority is granted.

## Runbook Exhaustion Decision: post-Step 7.35 DirectScalar binary-LHS receipt

Close rejected. Commit `18443fc0f` receives exactly closed 818's selected
DirectScalar `LirBinOp.lhs` body-parameter authority into the typed Raw-BIR
binary destination, preserving current-function owner, parameter index,
`LirTypeRef`, ABI, role, and matching lhs SSA value/type. The supervisor
accepted a fresh focused
`^backend_lir_selected_pointer_authority$` proof (1/1) and the matching full
CTest `test_before.log`/`test_after.log` guard (3003/3038 passed, 35 failures
in each, non-regressive). Malformed authority rejects transactionally.

This bounded receiver row does not satisfy the source completion gate: the
checked no-omission matrix and its per-row typed
authority/destination/importer/verifier/proof dispositions remain incomplete,
as do lossless receipt of every valid current-LIR semantic fact, explicit
dispatcher completeness, whole-module transactional proof, and documentation
convergence. Valid unreceived families include the remaining body-parameter
forms, memory/VA, aggregate/vector, module/type/global/metadata, residual
instruction/terminator, and inline-assembly forms. Presentation text, names,
rendered operands, `monostate`, and unclassified values remain non-authority.

Classification: `separate-blocker`. New open idea
`ideas/open/823_lir_next_body_parameter_authority_handoff.md` owns only
tracing, publishing, verifying, and handing off one next valid function-body
parameter-use row. It must not edit Raw-BIR/importer code, reopen the accepted
pointer or DirectScalar rows, choose from presentation fields, or absorb any
other family.

Resumption record: Steps 1 through 7.35 are accepted historical work, most
recently receiver commit `18443fc0f` with the focused 1/1 and matching full
CTest guard above. After 823 closes with an exact structured one-row handoff
and accepted focused producer proof, reactivate 734 and repair its runbook for
only that matching typed Raw-BIR receiver row. Do not repeat Step 7.35 or
receive another parameter, memory/VA, aggregate/vector, module/type/global,
instruction/terminator, or inline-assembly form without its separately scoped
first-owner handoff.

## Resumption Record: closed 823 DirectScalar binary-RHS authority

Closed idea 823 is capability-complete for one distinct producer/schema/verifier handoff in commit `83d7959f2`. It publishes only `LirBinOp.scalar_rhs_parameter_authority`: the RHS `LirValueId`, parameter index, `LirTypeRef`, current-function `LinkNameId` owner, `LirNativeBodyParameterAbi::DirectScalar`, explicit `Rhs` role, matching RHS SSA operand/value, and matching operation type. The native producer binds only when its current-function parameter definition agrees on value, type, and ABI; the verifier rejects absent carrier, invalid value, duplicate definition, foreign owner, wrong LHS role, type mismatch, ABI mismatch, and RHS operand/value mismatch. No Raw-BIR/importer/builder/receiver change was accepted in 823.

Accepted proof is the fresh `cmake --build --preset default --target backend_lir_selected_pointer_authority_test` build and focused `ctest --test-dir build --output-on-failure -R '^backend_lir_selected_pointer_authority$'` pass, with matching `test_before.log`/`test_after.log` 1/1 passing and an accepted non-decreasing guard using `--allow-non-decreasing-passed`.

Exact return action: resume at **Step 7.36 - Receive the one 823-authorized DirectScalar binary-RHS body-parameter authority row**. Add only that tuple's typed Raw-BIR destination, importer dispatch, reachable verifier path, and transactional positive/malformed-authority coverage. Do not repeat accepted Steps 1 through 7.35 (including `18443fc0f`), receive direct-pointer or binary-LHS rows again, or absorb any other parameter, memory/VA, aggregate/vector, module/type/global/metadata, instruction/terminator, or inline-assembly family. Reapply this source's completion gate after the one receipt.

## Runbook Exhaustion Decision: post-Step 7.36 DirectScalar binary-RHS receipt

Close rejected. Commit `c87976453` receives exactly closed 823's structured
`LirBinOp.rhs` DirectScalar body-parameter authority into a distinct typed
Raw-BIR RHS destination, importer dispatch, and reachable verifier path.
Nearby positive inspection and malformed-authority cases prove transactional
rejection. The supervisor accepted the fresh focused
`backend_lir_selected_pointer_authority_test` build and selected CTest 1/1,
the matching non-decreasing before/after 1/1 regression guard, and the fresh
broader `^backend_` checkpoint 6/6.

This bounded receipt does not satisfy the source completion gate. Unmet source
criteria are the checked no-omission coverage matrix and its per-row typed
authority/destination/importer/verifier/proof disposition; lossless verified
Raw-BIR receipt of every valid current-LIR semantic fact; complete explicit
dispatcher and neighboring coverage; and final whole-module transactional,
documentation-convergence, focused, and broader acceptance proof. Valid
unreceived families include remaining function-body parameter uses, memory/VA,
aggregate/vector, module/type/global/metadata, residual
instruction/terminator, and inline-assembly forms. They remain fail closed;
no receiver may recover authority from text, names, rendered operands,
`monostate`, or unclassified values.

Classification: `separate-blocker`. New open idea
`ideas/open/824_lir_next_body_parameter_authority_handoff.md` is the next
first owner. It must trace, publish, verify, and hand off exactly one next
valid function-body parameter-use row; it must not edit Raw-BIR/importer code,
reopen the accepted direct-pointer or DirectScalar LHS/RHS rows, select a row
from presentation fields, or absorb another family.

Resumption record: Steps 1 through 7.36 are accepted historical work, most
recently `c87976453`; retain the focused 1/1, matching guard, and broader 6/6
acceptance evidence above. After 824 closes with one exact structured handoff
and accepted focused producer proof, reactivate 734 and repair its runbook for
only that matching typed Raw-BIR receiver row. Do not repeat Step 7.36 or
receive another parameter, memory/VA, aggregate/vector, module/type/global,
instruction/terminator, or inline-assembly form without its separately scoped
first-owner handoff.

## Resumption Record: closed 824 DirectScalar return-value authority

Closed idea 824 is capability-complete for one distinct producer/schema/
verifier handoff in `fac485148`. It publishes only optional
`LirRet.return_value_parameter_authority` for an unchanged current-function
DirectScalar parameter returned through the exact SSA return operand. The
authoritative tuple is value identity, parameter index, `LirTypeRef`, owning
`LinkNameId`, `LirNativeBodyParameterAbi::DirectScalar`, and `ReturnValue`
role. It must agree with exactly one native definition, the return operand,
and signature return. Missing carrier for the selected form and malformed,
foreign, duplicate, owner/index/type/ABI/role, return-operand, or
signature-return mismatch reject transactionally. Nonselected return forms
synthesize no carrier and remain outside this row. No Raw-BIR/importer/
receiver work landed in 824.

Accepted proof is a fresh
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`
pass (6/6), with a matching before/after non-regression guard passing 6/6 on
both sides.

Exact return action: resume only at **Step 7.37 - Receive the one
824-authorized DirectScalar return-value parameter authority row**. Add only
that tuple's typed Raw-BIR return destination, importer dispatch, reachable
verifier path, and transactional positive/malformed-authority coverage. Do not
repeat accepted Steps 1 through 7.36 (including `c87976453`), receive another
parameter form, or recover authority from text, signatures, names, rendered
operands, diagnostics, or `monostate`. Reapply this source's completion gate
after the one receipt.

## Runbook Exhaustion Decision: post-Step 7.37 DirectScalar return-value receipt

Close rejected. Commit `88e930ee1` receives exactly closed 824's structured
`LirRet.return_value_parameter_authority` DirectScalar ReturnValue tuple into
the typed Raw-BIR parameter `ValueId` and `ReturnTerm`. Its importer and
reachable verifier reject missing, foreign, duplicate, owner/index/type/ABI/
role/operand/signature-incoherent authority before publication; nearby
positive and malformed-authority coverage retains the exact parameter
identity. Supervisor acceptance is a fresh
`cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^backend_'` pass (6/6), with a matching 6/6 before/
after non-regression guard.

This bounded receiver row does not satisfy the source completion gate. The
checked no-omission coverage matrix and its per-row typed
authority/destination/importer/verifier/proof dispositions remain incomplete;
lossless verified Raw-BIR receipt of every valid current-LIR semantic fact,
complete explicit dispatcher and neighboring coverage, whole-module
transactional proof, and documentation convergence are still unmet. Valid
unreceived families include remaining function-body parameter uses, memory/VA,
aggregate/vector, module/type/global/metadata, residual
instruction/terminator, and inline-assembly forms. They remain fail closed;
no receiver may recover authority from text, names, rendered operands,
signatures, `monostate`, or unclassified values.

Classification: `separate-blocker`. New open idea
`ideas/open/825_lir_next_body_parameter_authority_handoff.md` owns only
tracing, publishing, verifying, and handing off one next valid function-body
parameter-use row. It must not edit Raw-BIR/importer code, reopen accepted
direct-pointer or DirectScalar LHS/RHS/ReturnValue rows, select a row from
presentation fields, or absorb another family.

Resumption record: Steps 1 through 7.37 are accepted historical work, most
recently receiver commit `88e930ee1` with the fresh 6/6 focused broader proof
and matching 6/6 guard above. Interrupted point: source completion
reassessment after Step 7.37; no further 734 receiver row is authorized.
After 825 closes with one exact structured handoff and accepted focused
producer proof, reactivate 734 and repair its runbook for only that matching
typed Raw-BIR receiver row. Do not repeat Step 7.37 or receive another
parameter, memory/VA, aggregate/vector, module/type/global, instruction/
terminator, or inline-assembly form without its separately scoped first-owner
handoff.

## Resumption Record: closed 825 DirectScalar switch-selector authority

Closed idea 825 is capability-complete in `37674ad76`. It publishes exactly
one new producer/schema/verifier row: an unchanged current-function integer
parameter with `LirNativeBodyParameterAbi::DirectScalar` used directly as
`LirSwitch.selector`. The native authority tuple is the parameter-definition
`LirValueId`, current `LirFunction.link_name_id` owner, parameter index,
`LirTypeRef`, DirectScalar ABI, and
`LirSwitchSelectorParameterRole::SwitchSelector`. It is carried only by
`LirSwitch.selector_parameter_authority`, and its checked consumer relation
requires `LirSwitch.selector == authority.value` and
`LirSwitch.selector_type_ref == authority.type`. Missing, invalid, duplicate,
foreign, owner/index/type/ABI/role-mismatched, and consumer-incoherent
authority rejects; no fact is recovered from display text.

The accepted producer proof is a fresh `cmake --build --preset default`, the
exact focused `ctest --test-dir build -j --output-on-failure -R
'^frontend_lir_call_type_ref$'` pass, and its matching guard from the prior
0/1 expected abort to 1/1 pass. No Raw-BIR/importer/receiver code landed in
825.

Exact return action: resume at **Step 7.38 - Receive the one
825-authorized DirectScalar switch-selector parameter authority row**. Add
only the typed Raw-BIR switch-selector parameter destination, importer
dispatch, reachable verifier path, and transactional positive/malformed
authority coverage for this tuple and its two equality checks. Do not repeat
Steps 1 through 7.37 (including `88e930ee1`), reuse binary-LHS authority,
materialize an `add`, admit another parameter row, or absorb memory/VA,
aggregate/vector, module/type/global, instruction/terminator, or
inline-assembly work. Reapply this source completion gate after the one
receipt.

## Runbook Exhaustion Decision: post-Step 7.38 DirectScalar switch-selector receipt

Close rejected. Commit `ab6e08fa8` receives exactly closed 825's
`LirSwitch.selector_parameter_authority` DirectScalar switch-selector tuple
into the typed Raw-BIR switch receiver. Its importer and reachable verifier
retain native value/owner/index/type/ABI/role authority plus exact selector and
selector-type equality, rejecting missing, invalid, duplicate, foreign,
owner/index/type/ABI/role-mismatched, and consumer-incoherent authority before
publication. Supervisor acceptance is a fresh `cmake --build --preset default`
and exact `ctest --test-dir build -j --output-on-failure -R '^backend_'` pass
(6/6), with a matching 6/6 before/after non-regression guard accepted with
`--allow-non-decreasing-passed` because executable count is unchanged.

This bounded receiver row does not satisfy the source completion gate. The
checked no-omission coverage matrix and its per-row typed
authority/destination/importer/verifier/proof dispositions remain incomplete;
lossless verified Raw-BIR receipt of every valid current-LIR semantic fact,
complete explicit dispatcher and neighboring coverage, whole-module
transactional proof, and documentation convergence are still unmet. Valid
unreceived families include additional function-body parameter uses, memory/VA,
aggregate/vector, module/type/global/metadata, residual
instruction/terminator, and inline-assembly forms. They remain fail closed; no
receiver may recover authority from text, names, rendered operands, signatures,
`monostate`, or unclassified values.

Classification: `separate-blocker`. New open idea
`ideas/open/826_lir_next_body_parameter_authority_handoff.md` owns only
tracing, publishing, verifying, and handing off exactly one next valid
function-body parameter-use row. It must not edit Raw-BIR/importer code,
reopen accepted DirectPointer or DirectScalar GEP/binary-LHS/binary-RHS/
ReturnValue/switch-selector receipts, select a row from presentation fields,
or absorb another family.

Resumption record: Steps 1 through 7.38 are accepted historical work, most
recently receiver commit `ab6e08fa8` with the fresh 6/6 focused broader proof
and matching 6/6 guard above. Interrupted point: source completion
reassessment after Step 7.38; no further 734 receiver row is authorized.
After 826 closes with one exact structured handoff and accepted focused
producer proof, reactivate 734 and repair its runbook for only that matching
typed Raw-BIR receiver row. Do not repeat Step 7.38 or receive another
parameter, memory/VA, aggregate/vector, module/type/global, instruction/
terminator, or inline-assembly form without its separately scoped first-owner
handoff.

## Resumption Record: closed 826 DirectScalar truthiness-comparison LHS authority

Closed idea 826 is capability-complete in `02ef01e94`. It publishes exactly
one producer/schema/verifier row: an unchanged current-function DirectScalar
integer parameter used directly as `LirCmpOp.lhs` by
`StmtEmitter::to_bool_operand`. The native tuple is
`LirCurrentFunctionBodyParameterDefinition.value`, current
`LirFunction.link_name_id` owner, definition index, definition integer
`LirTypeRef`, `LirNativeBodyParameterAbi::DirectScalar`, and the bounded
truthiness-comparison LHS role. Its checked consumer relation requires
`LirCmpOp.lhs == authority.value`, `LirCmpOp.type_str == authority.type`,
integer predicate `ne`, and an authoritative integer-zero RHS.

The verifier rejects missing, invalid, duplicate-definition, foreign,
owner/index/type/ABI/role-mismatched, and consumer-incoherent authority
transactionally. `LirCondBr.condition` is not selected because it is the
comparison result rather than the direct parameter use; all other comparison
and parameter forms remain fail closed. The accepted proof is fresh
`cmake --build --preset default`, exact focused `ctest --test-dir build
--output-on-failure -R '^frontend_lir_call_type_ref$'` (1/1), and a matching
1/1 non-regression guard. No Raw-BIR/importer/receiver code landed in 826.

Exact return action: resume at **Step 7.39 - Receive the one 826-authorized
DirectScalar truthiness-comparison-LHS parameter authority row**. Add only
this tuple's typed Raw-BIR destination, importer dispatch, reachable verifier
path, and transactional positive/malformed-authority coverage for the stated
LHS/type/predicate/zero relations. Do not repeat Steps 1 through 7.38,
receive another parameter row, use `LirCondBr.condition` as direct parameter
authority, or recover identity from presentation fields. Reapply this source
completion gate after the one receipt.

## Runbook Exhaustion Decision: post-Step 7.39 DirectScalar truthiness-comparison-LHS receipt

Close rejected. Commit `f7c33237e` receives exactly closed 826's
`LirCmpOp` truthiness-comparison-LHS DirectScalar parameter tuple into typed
Raw BIR, retaining the value, owner, index, type, ABI, role, and checked
integer-`ne`/authoritative-zero-RHS relation. The supervisor accepted a fresh
`cmake --build --preset default`, exact
`ctest --test-dir build -j --output-on-failure -R '^backend_'` proof passing
6/6, and the matching accepted regression guard recorded in `test_after.log`.
Malformed missing, invalid, duplicate, foreign, owner/index/type/ABI/role,
and consumer-incoherent authority rejects transactionally.

This bounded receiver row does not satisfy the source completion gate. The
checked no-omission coverage matrix and its per-row typed
authority/destination/importer/verifier/proof dispositions remain incomplete;
lossless verified receipt of every valid current-LIR semantic fact, complete
explicit dispatcher and neighboring coverage, whole-module transactional
proof, documentation convergence, and source-wide focused/broader proof are
still unmet. Valid unreceived families include additional function-body
parameter uses, memory/VA, aggregate/vector, module/type/global/metadata,
residual instruction/terminator, and inline-assembly forms. They remain fail
closed; no receiver may recover authority from text, names, rendered operands,
signatures, `monostate`, or unclassified values.

Classification: `separate-blocker`. New open idea
`ideas/open/827_lir_next_body_parameter_authority_handoff.md` owns only the
trace, publication, verification, and exact one-row handoff for the next valid
function-body parameter use. It must not edit Raw-BIR/importer code, reopen
the accepted DirectPointer or DirectScalar GEP/binary-LHS/binary-RHS/
ReturnValue/switch-selector/truthiness-comparison-LHS receipts, select from
presentation fields, or absorb another family.

Resumption record: Steps 1 through 7.39 are accepted historical work, most
recently receiver commit `f7c33237e` with the fresh build, exact backend 6/6,
and matching accepted guard above. Interrupted point: source completion
reassessment after Step 7.39; no further 734 receiver row is authorized.
After 827 closes with one exact structured handoff and accepted focused
producer proof, reactivate 734 and repair its runbook only for that matching
typed Raw-BIR receiver row. Do not repeat Step 7.39 or receive another
parameter, memory/VA, aggregate/vector, module/type/global, instruction/
terminator, or inline-assembly form without its separately scoped first-owner
handoff.

## Resumption Record: closed 827 fixed-direct-call argument-0 body-parameter authority

Closed idea 827 is capability-complete for exactly one producer/schema/
verifier handoff in `96a6bb20f`. It authorizes only
`LirCallOp.structured_args[0]` carrying
`LirFixedDirectCallArgumentParameterAuthority`: the matching current-function
parameter definition/value, current-function `LinkNameId` owner,
`parameter_index`, `LirTypeRef`, `LirNativeBodyParameterAbi::DirectScalar`,
and `LirFixedDirectCallArgumentParameterRole::FixedDirectCallArgument0`.
The selected consumer is an unchanged native DirectScalar current-function
parameter used as structured argument 0 of a direct, non-variadic, specified
call; its SSA value/type must agree with the authority and with fixed callee
parameter 0. Missing, invalid, duplicate matching native definition, foreign,
owner/index/type/ABI/role-mismatched, and consumer-incoherent forms reject.

Accepted producer proof is fresh `cmake --build --preset default` plus
`ctest --test-dir build --output-on-failure -R
'^frontend_lir_call_type_ref$'`, passing 1/1 before and after, with fresh
`ctest --test-dir build --output-on-failure -R '^backend_'` passing 6/6.

Exact return action: resume only at **Step 7.40 - Receive the one
827-authorized fixed-direct-call argument-0 DirectScalar body-parameter
authority row**. Add only its typed Raw-BIR call-argument destination,
importer dispatch, reachable verifier path, and transactional positive/
malformed-authority coverage. Do not repeat Steps 1 through 7.39, receive any
other parameter form, or reconstruct authority from text, names, signatures,
rendered operands, diagnostics, or `monostate`. This handoff makes no Raw-BIR
receipt claim and does not complete this source.

## Runbook Exhaustion Decision: post-Step 7.40 fixed-direct-call argument-0 receipt

Close rejected. Commit `6609d92d4` receives exactly closed 827's
`LirCallOp.structured_args[0]` DirectScalar fixed-direct-call argument-0
authority into the typed Raw-BIR call-argument destination. The importer and
reachable verifier preserve the native parameter/owner/index/type/ABI/role
tuple and its direct non-variadic fixed-callee parameter-0 coherence,
rejecting missing, invalid, duplicate, foreign, owner/index/type/ABI/role, and
consumer-incoherent authority transactionally before publication. Supervisor
acceptance is a fresh `cmake --build --preset default`, exact
`ctest --test-dir build -j --output-on-failure -R '^backend_'` proof passing
6/6, and a matching accepted 6/6 before/after non-decreasing regression
guard.

This one bounded receiver row does not satisfy the source completion gate. The
checked no-omission coverage matrix and its per-row typed
authority/destination/importer/verifier/proof dispositions remain incomplete;
lossless verified receipt of every valid current-LIR semantic fact, complete
explicit dispatcher and neighboring coverage, whole-module transactional
proof, documentation convergence, and source-wide focused/broader proof are
still unmet. Valid unreceived families include further function-body parameter
uses, memory/VA, aggregate/vector, module/type/global/metadata, residual
instruction/terminator, and inline-assembly forms. They remain fail closed;
no receiver may recover authority from text, names, rendered operands,
signatures, `monostate`, or unclassified values.

Classification: `separate-blocker`. New open idea
`ideas/open/829_lir_next_body_parameter_authority_handoff.md` owns only tracing,
publishing, verifying, and handing off exactly one next valid function-body
parameter-use row. It must not edit Raw-BIR/importer code, reopen the accepted
DirectPointer or DirectScalar GEP/binary-LHS/binary-RHS/ReturnValue/
switch-selector/truthiness-comparison-LHS/fixed-direct-call-argument-0
receipts, select from presentation fields, or absorb another family.

Resumption record: Steps 1 through 7.40 are accepted historical work, most
recently receiver commit `6609d92d4` with the fresh build, exact backend 6/6,
and matching accepted guard above. Interrupted point: source completion
reassessment after Step 7.40; no further 734 receiver row is authorized.
After 829 closes with one exact structured handoff and accepted focused
producer proof, reactivate 734 and repair its runbook only for that matching
typed Raw-BIR receiver row. Do not repeat Step 7.40 or receive another
parameter, memory/VA, aggregate/vector, module/type/global,
instruction/terminator, or inline-assembly form without its separately scoped
first-owner handoff.

## Runbook Exhaustion Decision: post-Step 7.41 fixed-direct-call argument-1 receipt

Close rejected. Commit `380ee782f` receives exactly closed 829's
`LirCallOp.structured_args[1]` DirectScalar fixed-direct-call argument-1
authority into the typed Raw-BIR call-argument destination. The importer and
reachable verifier preserve the native parameter value, owner, parameter
index, argument index, scalar type, ABI, explicit
`FixedDirectCallArgument1` role, and direct non-variadic fixed-callee
argument/signature coherence through the call node. The receiver preserves the
accepted argument-0 row and rejects missing, invalid, duplicate, foreign,
owner/index/type/ABI/role, and consumer-incoherent argument-1 authority
transactionally before publication. Supervisor acceptance is a fresh
`cmake --build --preset default`, exact
`ctest --test-dir build -j --output-on-failure -R
'^backend_lir_to_bir_interface$'` proof passing 1/1, and a matching accepted
1/1 before/after non-decreasing regression guard.

This one bounded receiver row does not satisfy the source completion gate. The
checked no-omission coverage matrix and its per-row typed
authority/destination/importer/verifier/proof dispositions remain incomplete;
lossless verified receipt of every valid current-LIR semantic fact, complete
explicit dispatcher and neighboring coverage, whole-module transactional
proof, documentation convergence, and source-wide focused/broader proof are
still unmet. Valid unreceived families include further function-body parameter
uses, memory/VA, aggregate/vector, module/type/global/metadata, residual
instruction/terminator, and inline-assembly forms. They remain fail closed;
no receiver may recover authority from text, names, rendered operands,
signatures, `monostate`, compatibility mirrors, or unclassified values.

Classification: `separate-blocker`. New open idea
`ideas/open/853_lir_next_body_parameter_authority_handoff.md` owns only
tracing, publishing, verifying, and handing off exactly one next valid
function-body parameter-use row after the accepted fixed-direct-call
argument-1 receipt. It must not edit Raw-BIR/importer code, reopen the
accepted DirectPointer or DirectScalar GEP/binary-LHS/binary-RHS/ReturnValue/
switch-selector/truthiness-comparison-LHS/fixed-direct-call-argument-0/
fixed-direct-call-argument-1 receipts, select from presentation fields, or
absorb another family.

Resumption record: Steps 1 through 7.41 are accepted historical work, most
recently receiver commit `380ee782f` with the fresh focused proof and matching
accepted guard above. Interrupted point: source completion reassessment after
Step 7.41; no further 734 receiver row is authorized. After 853 closes with
one exact structured handoff and accepted focused producer proof, reactivate
734 and repair its runbook only for that matching typed Raw-BIR receiver row.
Do not repeat Step 7.41 or receive another parameter, memory/VA,
aggregate/vector, module/type/global, instruction/terminator, or
inline-assembly form without its separately scoped first-owner handoff.
