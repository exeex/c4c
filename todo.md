# Current Packet

Status: Active
Source Idea Path: ideas/open/182_type_identity_migration_closure_gate.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Convert Remaining Blockers

## Just Finished

Step 4 reviewed the closure ledger blockers for
`ideas/open/182_type_identity_migration_closure_gate.md`.

Remaining candidate surfaces from the Step 2 ledger:

- Parser/AST `TypeSpec`, rendered LIR type text, rendered signature suffixes,
  final output spelling, dumps, and diagnostics: no blocker. Classified as
  syntax payload, display text, diagnostic text, ABI spelling, or route-local
  rendering, not semantic authority for migrated metadata-rich paths.
- Legacy/no-metadata local and global layout bridges: no blocker. Classified
  as explicit compatibility for hand-authored/raw/no-metadata inputs; closed
  notes reject metadata-rich layout misses falling back to rendered text.
- Mirrorless byval ABI compatibility and older aggregate ABI fallbacks: no
  blocker for this closure. Classified as explicit compatibility, not the
  preferred authority for metadata-rich aggregate ABI classification. Complete
  retirement could be a future idea only if selected as a new migration target.
- No-id `LirTypeRef` compatibility and hand-authored no-id direct-LIR inputs:
  no blocker. Classified as intentional compatibility; structured
  `StructNameId` facts own equality where both sides carry ids.
- Legacy byval copy inputs and no-signature/no-id AArch64/direct-LIR
  compatibility: no blocker. Closed notes require metadata-rich AArch64/byval
  routes to fail closed for stale, missing, opaque, or no-id structured facts.
- Broader sema canonical type equality replacement: out of scope for the
  completed wave. Follow-up candidate only if a future idea selects that
  semantic comparison surface.
- Parser-only `TypeSpec::is_fn_ptr` comparisons: out of scope for the
  completed function-pointer idea. Follow-up candidate only if parser/sema
  function-pointer identity becomes the selected path.
- Direct-call signature metadata: out of scope for the completed indirect-call
  LIR/BIR function-pointer path. Follow-up candidate only if direct-call
  signature identity is selected later.
- Final emitted syntax formatting and printer spelling: no blocker. Classified
  as output/display; changing it for cosmetics was explicitly out of scope.
- Post-BIR backend ABI decisions beyond the selected AArch64/direct-LIR and
  aggregate byval paths: out of scope. Follow-up candidate only if future
  backend ABI work finds a metadata-rich path silently falling back to text.

Decision: no true closure blocker was found. No new `ideas/open/` follow-up is
required by this packet. The only follow-up shapes are conditional future
targets if those out-of-scope surfaces are selected by supervisor/plan-owner
later; this packet did not create new idea files.

## Suggested Next

Proceed to Step 5: summarize the accepted ledger, validation result, retained
compatibility bridges, and closure decision for plan-owner review.

## Watchouts

- Do not edit implementation files or tests as part of this closure gate.
- Do not touch `ideas/closed/` except to read closure evidence.
- Do not touch `test_before.log` or `test_baseline.log` during activation.
- Do not accept rendered type spelling as semantic authority unless it is
  classified as output, diagnostics, ABI spelling, or an explicit
  compatibility bridge.
- If review finds an actual metadata-rich route that still silently falls back
  to rendered type/signature text as semantic authority, that is a blocker and
  should become a narrow follow-up idea. The closed idea evidence reviewed here
  did not identify one.
- Do not convert cosmetic output/printer spelling or explicitly no-metadata
  compatibility into a closure blocker without a selected semantic authority
  path.

## Proof

No build required. Read-only checks used:

- `sed -n '1,170p' todo.md`
- `sed -n '100,190p' plan.md`
- `rg -n 'Out Of Scope|Remaining|out of scope|follow-up|legacy|compatibility|Reviewer Reject Signals|metadata-rich|rendered|TypeSpec::is_fn_ptr|direct-call|post-BIR|final emitted|no-metadata|no-id|mirrorless' ideas/closed/17{2,3,4,5,6,7,8,9}_*.md ideas/closed/180_*.md ideas/closed/181_*.md`
- `git status --short`

Result: blocker review recorded. No high-risk spelling-based authority remains
unclassified in the selected migration paths, and no new follow-up idea is
required by this packet.
