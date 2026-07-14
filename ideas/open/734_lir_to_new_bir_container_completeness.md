# LIR-To-New-BIR Container And Import Completeness

Status: Open (paused after accepted Step 7.22; switch-selector value-authority
successor 756 is active before the next receiver repair)
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

Classification: `separate-blocker`. Open idea
`ideas/open/756_lir_switch_selector_value_identity_publication.md` owns only
publication, verification, focused malformed-authority coverage, and a typed
handoff for the active switch selector. This source is paused after completed
Steps 1 through 7.22. After 756 closes with the exact structured handoff,
reactivate 734 and repair its runbook for one bounded `LirSwitch` Raw-BIR
receiver packet: consume only the typed selector with already typed default and
ordered case successors, preserve case order and transactional rejection, and
do not inspect selector or label text for semantic recovery. Do not repeat
Step 7.22 or absorb computed-goto, PHI, local/object, memory/va,
aggregate/vector, body-parameter, or other remaining families.

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
