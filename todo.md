# Current Packet

Status: Active
Source Idea Path: ideas/open/171_identity_authority_migration_closure_gate.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Reconcile Remaining Identity Inventory

## Just Finished

Step 3: Reconcile Remaining Identity Inventory checkpoint completed for idea
171.

Evidence reviewed:
- Idea 167 final audit artifact from historical `todo.md` commit `80c86e13d`,
  including the Step 1-5 inventory, classification legend, retained bridge
  table, route-local/generated families, and follow-up recommendations.
- Idea 168 closure notes, which retired or fenced parser, sema/consteval, HIR,
  LIR/HIR-to-LIR, BIR, and backend rendered/raw-symbol compatibility surfaces.
- Idea 169 closure notes, which moved the selected BIR local-slot family to
  typed `SlotNameId` authority and classified the remaining route-local raw
  name families.
- Idea 170 closure notes plus `scripts/string_authority_classifications.json`,
  which provide the current declaration-level guard inventory.

Current guard inventory from `scripts/string_authority_classifications.json`:

| Classification category | Entries |
| --- | ---: |
| `abi-spelling` | 10 |
| `compatibility-bridge` | 120 |
| `diagnostic-debug` | 1 |
| `display-output` | 1 |
| `generated-temporary-name` | 1 |
| `route-local-identity` | 43 |
| `structured-authority` | 10 |
| Total classified entries | 186 |

The guard reports more declaration-level hits than JSON entries because one
classification entry can cover repeated declaration hits for the same exact
path plus symbol. The fresh guard proof reports `235 classified
declaration-level hits`.

Remaining-item ledger:

| Inventory lane | Current classification | Closure decision |
| --- | --- | --- |
| Parser record/type/const-int rendered helpers from idea 167 | Documented bridge or fixed by idea 168 where broad fallback was retired/fenced. | No unclassified semantic entry remains; retained parser bridges are explicit compatibility/no-metadata boundaries. |
| Sema validate, type-utils, consteval, and canonical-symbol rendered maps | Documented bridge or structured authority; display/ABI spelling kept output-only. | No new source idea needed; complete structured misses remain assigned to the guard boundary. |
| HIR module declaration, record owner, template, specialization, and lowerer maps | Mix of structured authority, documented bridge, route-local identity, ABI spelling, and display/output. | No unclassified semantic entry remains; retained rendered paths are compatibility, no-owner/incomplete-metadata, route-local, or output cases. |
| LIR and HIR-to-LIR link-visible identity maps and final-output text scans | `LinkNameId`/`StructNameId` authority where structured; retained raw-name/text paths are documented bridge, ABI spelling, or display/output. | No blocker; production bridge narrowing was handled by idea 168, with output text kept separate from semantic authority. |
| BIR link-visible function/global lookup and validator raw-name helpers | Documented bridge or diagnostic/debug; id/name mismatches fail validation where ids are present. | No unclassified semantic entry remains; invalid-id and raw-import boundaries are documented compatibility. |
| BIR local slots, local values, blocks, memory-address bases, phi labels, and string-pool names | Route-local identity, generated temporary name, or documented bridge. | The selected local-slot family was fixed under idea 169; broader AArch64 direct LIR emitter and prealloc/out-of-SSA raw-name helper sets remain follow-up candidates only if deeper typed-id migration is desired. |
| Prealloc, stack-layout, regalloc, and out-of-SSA raw-name helpers | Route-local identity, structured authority, documented bridge, or ABI spelling. | No closure blocker; raw route-local synchronization is classified, not source/link semantic authority. |
| Diagnostics, debug, printers, dumps, parity labels, final assembly/LLVM text, and ABI spellings | Diagnostic/debug, display/output, or ABI spelling. | Out of scope for semantic string-authority cleanup unless future code feeds the text back into production lookup. |

Blocker status:
- No declaration-level semantic string-authority entry remains unclassified by
  the current guard.
- No Step 3 implementation blocker was found.
- No new follow-up idea is required for closure. The only noted follow-up
  candidates are non-blocking deeper typed-id migration candidates already
  named by idea 169: AArch64 direct LIR emitter string-keyed lowering and the
  larger prealloc/out-of-SSA raw-name helper set.

## Suggested Next

Proceed to Step 4 by writing the compact identity-domain closure ledger for
`TextId`, structured semantic keys, `QualifiedNameKey`, owner-aware template
keys, `LinkNameId`, route-local ids, and display text.

## Watchouts

- Keep Step 4 as a closure-ledger packet; do not start idea 172 or type-identity
  audit work.
- Do not treat classified route-local names, generated names, ABI spellings, or
  display/output strings as source/link identity bugs.
- If the supervisor wants deeper migration of the AArch64 direct LIR emitter or
  prealloc/out-of-SSA raw-name helpers, that should be a separate follow-up
  initiative rather than an expansion of this closure gate.

## Proof

Supervisor-selected guard proof:
- `python3 scripts/string_authority_guard.py > test_after.log 2>&1`
  - Result: passed.
  - Proof log path: `test_after.log`
  - Log summary: `string authority guard passed: 235 classified declaration-level hits`
- `git diff --check -- todo.md`
  - Result: passed.
