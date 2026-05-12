# Current Packet

Status: Active
Source Idea Path: ideas/open/182_type_identity_migration_closure_gate.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Build the Type-Domain Closure Ledger

## Just Finished

Step 2 built the type-domain closure ledger for
`ideas/open/182_type_identity_migration_closure_gate.md`.

Ledger by domain:

- Syntax payload: parser/AST `TypeSpec`, rendered LIR type text, rendered
  signature suffixes, final output spelling, dumps, and diagnostics remain
  allowed as syntax/display payloads. They are not accepted as semantic
  authority for the migrated metadata-rich paths.
- Resolved type identity: structured facts are authoritative where closed
  ideas selected a path: HIR aggregate direct assignment uses structured owner
  identity instead of rendered `TypeSpec`; `LirTypeRef::operator==` uses
  structured `StructNameId` when both sides carry ids; template record owner
  identity uses structured `SpecializationKey` facts for equality, hashing,
  and completeness; indirect function-pointer calls carry structured
  `LirCallOp::callee_signature` facts through LIR/BIR.
- Layout identity: selected local aggregate GEP, generated global aggregate
  initializer/layout, and incoming byval materialization routes use structured
  record/layout/type-ref facts. Metadata-rich stale, mismatched, missing,
  opaque, invalid, zero-sized, or zero-aligned layout metadata fails closed
  rather than being repaired by rendered struct/type spelling.
- ABI facts: selected fixed aggregate byval call-argument classification and
  selected AArch64 direct-LIR aggregate return/byval signature routes prefer
  structured `LirTypeRef` / `StructNameId` / layout facts. Rendered LLVM ABI
  spelling remains output/ABI spelling, not the preferred classification
  authority for metadata-rich aggregate inputs.
- Display spelling: rendered canonical template text, `%struct...` names,
  LIR/BIR printer text, diagnostics, final emitted syntax, and call signature
  suffix text are retained as display, diagnostic, ABI spelling, or route-local
  rendering surfaces.
- Compatibility bridges: retained bridges are explicitly legacy/no-metadata or
  mirrorless/no-id boundaries: no-metadata local/global layout fallback,
  mirrorless byval ABI compatibility, no-id `LirTypeRef` compatibility,
  legacy byval copy inputs, and no-signature/no-id AArch64/direct-LIR
  compatibility. Closed notes classify these as fenced compatibility, not
  structured metadata mismatch recovery.

Unresolved or ambiguous authority:

- No high-risk rendered-string authority was found unclassified in the closed
  idea evidence for the selected migration paths.
- Out-of-scope surfaces remain follow-up candidates only if they become active
  migration targets: broader sema canonical type equality replacement,
  parser-only `TypeSpec::is_fn_ptr` comparisons, direct-call signature
  metadata, final formatting/output syntax, post-BIR backend ABI decisions,
  and complete retirement of all legacy no-metadata bridges.

## Suggested Next

Proceed to Step 3: validate broad closure state by citing the accepted
`test_baseline.log` full-suite baseline or running the supervisor-selected
fresh broad validation command.

## Watchouts

- Do not edit implementation files or tests as part of this closure gate.
- Do not touch `ideas/closed/` except to read closure evidence.
- Do not touch `test_before.log` or `test_baseline.log` during activation.
- Do not accept rendered type spelling as semantic authority unless it is
  classified as output, diagnostics, ABI spelling, or an explicit
  compatibility bridge.
- The ledger is closure evidence only. If plan-owner review wants any
  out-of-scope surface promoted from follow-up candidate to blocker, route it
  through a new narrow idea instead of expanding this closure gate.
- Retained rendered surfaces are acceptable only in the classified roles above;
  a metadata-rich path silently falling back to text remains a blocker pattern.

## Proof

No build required for this closure ledger packet. Read-only checks used:

- `sed -n '1,180p' todo.md`
- `sed -n '1,180p' plan.md`
- `rg -n 'syntax payload|resolved type identity|layout identity|ABI class|ABI facts|display spelling|compatibility bridge|rendered|structured|legacy|fallback|authority|Closed after|Completion|Closure' ideas/closed/17{2,3,4,5,6,7,8,9}_*.md ideas/closed/180_*.md ideas/closed/181_*.md`

Result: ledger recorded. No unclassified high-risk rendered-string authority
was identified from closed idea evidence for the selected migration paths.
