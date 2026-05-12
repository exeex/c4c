# Current Packet

Status: Active
Source Idea Path: ideas/open/171_identity_authority_migration_closure_gate.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Write Closure Ledger

## Just Finished

Step 4: Write Closure Ledger checkpoint completed for idea 171.

Identity-domain closure ledger:

| Identity domain | Owner and purpose | Retained bridges, limits, and removal condition |
| --- | --- | --- |
| `TextId` | Parser and source-facing syntax spelling owner. It carries source text identity and spelling where the compiler needs stable source-origin text, not semantic equality by rendered string. | Retained parser/rendered helpers are compatibility or no-metadata boundaries from the pre-migration surface. They remain valid only when no richer parsed or semantic key exists. Removal condition: the caller receives typed syntax/semantic ids directly and no longer needs rendered fallback text. |
| Structured semantic keys | Sema, HIR, LIR, BIR, and backend semantic owners for domains that already have typed ids or structured key material. They own lookup equality for declarations, constants, slots, records, globals, and route state where identity has more shape than a display string. | Compatibility bridges are allowed where legacy APIs, incomplete metadata, invalid ids, or raw imported symbols still cross an older boundary. They must stay classified by the guard with owner, domain, category, and reason. Removal condition: the producer exports the structured id/key at that boundary and the consumer stops reconstructing identity from raw spelling. |
| `QualifiedNameKey` | Qualified semantic-name owner where identity depends on namespace/module path plus leaf name rather than plain text. It is the canonical lookup key for qualified declaration identity. | Any raw qualified-string lookup is retained only as a documented bridge for compatibility input or incomplete metadata. It is not a new semantic authority. Removal condition: callers construct and pass `QualifiedNameKey` or equivalent structured key material before lookup. |
| Owner-aware template keys | Template binding and canonical-symbol owners for identity that depends on both rendered template shape and owning declaration/context. They prevent plain rendered strings from deciding template equivalence across owners. | Retained rendered-template maps are compatibility bridges only when owner/context is unavailable or when preserving old serialized/debug-facing behavior. Removal condition: owner-aware template key material reaches the lookup site on every production path. |
| `LinkNameId` | Link-visible symbol owner for functions/globals and backend-facing symbol identity. It separates final link identity from source spelling and display text. | Raw symbol-name bridges remain for ABI spelling, imported/raw object surfaces, invalid-id diagnostics, or compatibility routes that have not yet been supplied a valid link id. Removal condition: every production consumer has valid `LinkNameId`/structured link key input and raw spelling is used only for final emitted text or diagnostics. |
| Route-local ids | Backend and lowering-route owners for local slots, temporaries, blocks, memory bases, phi labels, string-pool names, prealloc/regalloc state, and out-of-SSA route synchronization. These ids are local authority inside one lowering route and are not source or link semantic names. | Raw local names are retained when they synchronize a route-local table, name generated temporaries, or preserve direct emitter/prealloc/out-of-SSA helper behavior. Limitation: they must not be promoted into source/link semantic lookup. Removal condition: a future deeper typed-id migration gives those route-local helpers stable typed ids without changing their route-local scope. |
| Display/output text | Diagnostics, debug dumps, printers, parity labels, final assembly text, LLVM text, and ABI spellings own user-visible or emitted spelling only. They are outputs, not semantic lookup keys. | Retained text scans and formatting paths are acceptable while they remain output-only. Removal condition is required only if a future production lookup feeds these strings back into semantic identity; that would need a new structured owner or follow-up idea. |

Remaining semantic string lookup status:
- No unclassified semantic string-authority lookup remains in the current
  Step 3 inventory or the idea 170 guard classification set.
- Retained bridges have explicit owners and limits: parser/source spelling,
  structured semantic boundary gaps, qualified-name compatibility,
  owner-aware template compatibility, link-name/raw import compatibility,
  route-local synchronization, or output-only text.
- The only deeper migration candidates remain non-blocking and outside this
  closure gate: AArch64 direct LIR emitter string-keyed lowering and the larger
  prealloc/out-of-SSA raw-name helper set, if a future typed route-local id
  migration is desired.

Closure blocker status:
- No Step 4 closure blocker remains.
- No new follow-up idea is required to close the string identity-authority
  migration.

## Suggested Next

Proceed to Step 5: Decide Closure Or Follow-Up. The next packet should let the
plan owner or supervisor decide whether to close idea 171, move durable compact
closure notes into the appropriate lifecycle artifact, and retire active
`plan.md`/`todo.md` if closure is accepted.

## Watchouts

- Step 5 is lifecycle/closure work, not implementation work.
- Do not start idea 172 or type-identity audit work from this gate.
- Do not treat classified route-local names, generated names, ABI spellings,
  diagnostics/debug text, or display/output strings as source or link identity
  bugs unless a future production lookup consumes them as semantic identity.
- Retained bridges should stay guard-classified until their removal condition
  is met; closure should not depend on silently accepting unowned string lookup.

## Proof

Supervisor-selected lifecycle proof:
- `git diff --check -- todo.md`
  - Result: passed.
  - Proof log path: none; the delegated proof is a diff whitespace check and
    does not create `test_after.log`.
