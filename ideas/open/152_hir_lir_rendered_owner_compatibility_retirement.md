# HIR LIR Rendered Owner Compatibility Retirement

Status: Open
Created: 2026-05-08

Parent Ideas:
- `ideas/closed/151_parser_out_of_class_owner_probe_token_sequence.md`

## Goal

Retire the remaining HIR and LIR rendered-owner compatibility routes that were
left after idea 151 completed the parser-owned out-of-class owner-probe route.

The target is not parser declaration-shape classification. Parser owner probing
should already prefer token `TextId` segment sequences and structured
qualified-owner metadata. This idea tracks the later compatibility bridges that
still accept rendered method, aggregate, typedef, or owner spellings after a
structured lookup misses.

## Why This Idea Exists

Idea 151 removed parser-owned rendered `qualified_owner` authority from
out-of-class owner probing and template-owner relabeling. During Step 6, the
compatibility inventory also identified broader HIR/LIR routes that were not
safe to retire inside the parser packet.

Those routes are different in kind: they recover method attachment, aggregate
layout, TypeSpec ownership, member typedefs, signatures, or LIR lowering names
when older producers lack complete owner keys or module-canonical text ids.
They should be retired under their own proof plan instead of being hidden in a
closed parser idea.

## In Scope

- HIR rendered method-name fallback after incomplete structured metadata.
  Owner: HIR lowerer. Limitation: allowed only when structured out-of-class
  method metadata is absent or incomplete. Removal condition: all parser and
  HIR function producers carry complete `qualifier_text_ids`,
  `unqualified_text_id`, namespace/global metadata, and owner-indexed method
  registration.
- HIR AST-node owner-key spelling canonicalization across parser and module
  text tables. Owner: HIR owner-key bridge. Limitation: spelling is a
  cross-table carrier, not semantic authority. Removal condition: parser owner
  carriers are stored in the same semantic text table or passed as
  already-canonical HIR owner keys.
- HIR and LIR aggregate `TypeSpec` owner/layout fallback. Owner: HIR/LIR
  aggregate layout bridge. Limitation: needed for legacy `TypeSpec` producers,
  parser-owned `tag_text_id`s, and missing `struct_def_order` or layout
  metadata. Removal condition: all aggregate `TypeSpec`s carry complete
  module-canonical owner keys or record definitions and all layout declarations
  are owner-indexed.
- HIR member typedef and signature type recovery fallback. Owner: HIR member
  typedef resolver. Limitation: only a fallback for incomplete `TypeSpec`
  owner metadata. Removal condition: deferred member typedef producers always
  carry `deferred_member_type_owner_key`, qualifier `TextId`s, and member
  `TextId`s.
- LIR aggregate helper compatibility tags and owner recovery. Owner: LIR
  lowering. Limitation: layout and name recovery after structured lookup
  misses. Removal condition: HIR module export gives LIR complete structured
  aggregate names and owner keys for every aggregate use.

## Out Of Scope

- Reopening parser out-of-class owner-probe classification completed by idea
  151.
- Removing parser diagnostic, dump, or legacy display names when structured
  owner metadata already exists beside them.
- Treating one rendered-owner `TextId` as a semantic compound-owner key.
- Broad backend rewrites unrelated to aggregate owner/layout recovery.
- Weakening, deleting, or marking unsupported tests to make compatibility
  retirement appear complete.

## Acceptance Criteria

- HIR out-of-class method lowering no longer needs to split rendered
  `Node::name` for semantic owner recovery after complete structured metadata
  is available.
- HIR owner-key construction no longer needs parser/module text-table spelling
  repair when semantic owner keys can be carried directly.
- Aggregate `TypeSpec`, layout, member typedef, signature type, and LIR lowering
  paths use structured owner keys or record definitions before name recovery,
  and any remaining compatibility fallback is explicitly documented with an
  owner, limitation, and removal condition.
- Focused tests or probes cover stale rendered-owner spellings, same-suffix
  owner ambiguity, and aggregate/member-type recovery cases that would fail if
  rendered compatibility were still semantic authority.
- Validation includes the HIR/LIR/frontend subsets touched by the slice, with
  broader regression proof when aggregate layout or lowering behavior changes.

## Reviewer Reject Signals

- A slice claims HIR/LIR rendered-owner compatibility has been retired while a
  new helper still splits rendered owner names or compatibility tags as the
  primary semantic lookup route.
- A change treats a single rendered `TextId` containing `A::B` as a complete
  compound-owner identity.
- Tests are weakened, marked unsupported, or rewritten only to match changed
  rendered names instead of proving structured owner behavior.
- Parser owner-probe code is reopened for broad HIR/LIR cleanup without a
  direct parser-owned regression.
- Aggregate layout, member typedef, or LIR recovery keeps the exact old
  failure mode behind a new abstraction name.
- Broad lowering rewrites are mixed into this idea without focused proof for
  the specific compatibility route being retired.
