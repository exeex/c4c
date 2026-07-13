# LIR-To-New-BIR Container And Import Completeness

Status: Open (active)
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
identity blocker recorded below and is now reactivated at the exact typed
receiver rows in the committed handoff. The in-progress phase-C documentation
child remains open but parked. The already-landed phase-A and phase-B
documentation acceptances and phase-C C1-C6 slices are preserved as historical
evidence; they are not current post-implementation acceptance.

After this idea is implemented, accepted and closed, reactivate idea 732 and
rerun documentation convergence against the landed C++ implementation from
phase A, then B, then C and onward. Do not resume directly at the old phase-C
Step 7 checkpoint.

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
