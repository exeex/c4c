Status: Active
Source Idea Path: ideas/open/195_frontend_to_bir_legacy_string_lookup_closure_gate.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Reconstruct Closure Ledger

# Current Packet

## Just Finished

Step 1 reconstructed the frontend-to-BIR closure ledger from the closed
dependency ideas and resolved HIR blockers:

| Surface | Classification | Evidence |
| --- | --- | --- |
| Parser token spelling and deferred syntax text | Syntax payload, diagnostic/display text, compatibility mirror | Closed ideas 139, 154, 157, 141, and 145 classify parser strings as token/source payloads, diagnostics, display mirrors, syntax reconstruction compatibility, or provisional carriers paired with structured metadata. Semantic parser/Sema lookup must use direct AST links, declaration/owner objects, namespace context ids, `TextId` in text-identity domains, `QualifiedNameKey`, `NamePathKey`, `TypeSpec::record_def`, or Sema structured owner/tag tables. |
| Sema consteval and type utility boundaries | Structured authority or explicit no-metadata compatibility | Closed idea 139 removed covered parser/Sema rendered-string semantic lookup routes and parked cross-module HIR metadata blockers. Closed ideas 141 and 145 removed `TypeSpec::tag` authority and parser rendered-tag completion. Closed idea 182 classifies type syntax/display/ABI strings separately from resolved type identity, layout identity, and ABI facts. |
| HIR compile-time state and consteval registries | Structured authority for metadata-rich routes; explicit no-metadata compatibility for retained rendered registry APIs | Closed ideas 192 and 196 cover direct and pending consteval routes: complete structured consteval metadata is authoritative and complete structured misses fail closed instead of recovering through rendered consteval registry names. Retained rendered names are display, diagnostics, or no-metadata compatibility. |
| HIR object/member owner lookup | Structured authority for metadata-rich constructor/member owner lookup; rendered fallback only no-owner/no-metadata compatibility | Closed idea 193 selected the direct struct constructor route and requires structured owner metadata there. Complete structured owner misses fail closed; rendered tag fallback is classified as explicit no-owner/no-metadata compatibility or display/follow-up cleanup. |
| HIR template registry/generated template paths | Structured authority for generated call, replay, deduction, collection, seed, and retry paths; no-metadata compatibility where structured declaration metadata is absent | Closed idea 201 cleared the gate blocker by fencing generated template registry authority. Metadata-rich generated template paths no longer recover through string-only `find_template_def(name)`/`has_template_def(name)` after complete structured misses. |
| HIR generated-member payload paths | Structured authority for metadata-rich generated owner/member lookup; explicit no-metadata compatibility for incomplete generated-member callers | Closed idea 202 cleared the generated-member blocker. Complete structured owner/member misses cannot recover semantic authority through rendered `find_struct_static_member_decl` or `find_struct_static_member_const_value`; retained rendered lookup is limited to no-metadata or non-semantic text/display paths with a removal condition. |
| HIR-to-LIR call/type metadata | Structured authority for metadata-rich call signatures, call args, byval markers, aggregate/type refs; final spelling mirrors remain output/ABI/display | Closed ideas 182, 184, 190, and 191 classify generated call/type metadata as structured. Direct-call signatures and LIR call arguments carry structured facts for return/parameter types, variadic/byval/sret, operands, and byval type refs. `signature_text`, `callee_type_suffix`, and `args_str` remain final spelling or raw/no-metadata compatibility only. |
| LIR call payload | Structured authority when present; raw/no-metadata compatibility when absent; final spelling output remains | Closed idea 190 records `LirCallOp` structured argument payloads from generated `OwnedLirTypedCallArg` values. LIR-to-BIR lowering uses non-empty structured payloads as authority for operand, type text, and byval facts; empty payloads are the explicit legacy text-parsing compatibility path. |
| BIR lowering and global memory provenance | Structured authority via signature metadata and `LinkNameId`; route-local handles, ABI/final spelling, and no-id compatibility are explicit | Closed ideas 188, 191, and 194 classify BIR retained strings as presentation/output, diagnostics, route-local handles, ABI/final spelling, runtime/intrinsic placeholders, or raw/no-id compatibility. Dynamic global scalar-array materialization carries `LinkNameId` provenance into BIR; stale or missing ids fail closed on the covered path, while `kInvalidLinkName` is the explicit no-id compatibility boundary. |

No unproven high-risk semantic rendered-string authority remains in the closed
evidence ledger. Step 2 still needs a fresh residual audit of the metadata-rich
frontend-to-BIR paths before the gate can move to milestone validation.

## Suggested Next

Execute Step 2: audit residual high-risk metadata-rich paths for any rendered
string recovery after complete structured misses. Focus on the HIR registry,
constructor/member, generated template, generated member, HIR-to-LIR call/type
metadata, LIR call payload, and BIR lowering boundaries named by the plan.

## Watchouts

The ledger is evidence reconstruction, not a fresh code audit. If Step 2 finds
a metadata-rich path that still recovers semantic identity through rendered
strings, create a narrow blocker idea instead of folding implementation work
into this closure gate. Do not start backend restart inside this plan.

## Proof

Ledger-only `todo.md` update per supervisor packet. No build or tests were run,
and no root-level logs were created or modified.
