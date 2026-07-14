# LIR-To-New-BIR Container And Import Completeness

Status: Open (active at repaired Step 7.14 builtin-ffs add-one/select-arm packet)
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
