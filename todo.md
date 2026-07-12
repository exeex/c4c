# Current Packet

Status: Active
Source Idea Path: ideas/open/720_block_entry_publication_proof_evidence_multiplicity.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Define destination, claim, collection, and result contracts

## Just Finished

- Completed Plan Step 1 by tracing the present Route4/prepared/MIR ownership,
  recording the blocked cardinality baseline and truth table, and selecting the
  smallest generic model boundary. No implementation or test files changed.

### Current authority map

- Prepared identity authority starts with `PreparedValueId` and resolves one
  `PreparedRegallocValue`, whose `ValueNameId` and type become the destination
  metadata. The selected publication bundle owns the expected instruction
  coordinate (`src/backend/prealloc/prepared_lookups.cpp:2289-2410`,
  `src/backend/prealloc/value_locations.hpp:594-622`).
- Prepared proof attribution copies the supplied successor into a compatibility
  function, asks the BIR publication view to scan it, then accepts only one
  name/type match whose coordinate equals the publication bundle. It publishes
  only a boolean, one proof status, and one instruction index; it does not
  publish an independently identified claim collection
  (`src/backend/prealloc/prepared_lookups.cpp:2338-2402`).
- Route4 owns the structural PHI scan. Its block-entry record stops at the first
  same-name PHI and identifies it by successor plus display name/type; its index
  retains one record per PHI, but view/reference lookup again keys by successor
  plus name/type and reports a second match as ambiguous/duplicate
  (`src/backend/bir/bir_route4_publication.cpp:96-154,299-344,396-420,611-724`).
- Destination pointer and `ValueNameId` are carried in
  `Route4BlockEntryPublicationRecord`, but the pointer is not part of lookup
  equality and index-built records use an invalid name ID. Therefore neither is
  currently authoritative for distinguishing same-name/same-type destinations
  (`src/backend/bir/bir.hpp:1892-1910`,
  `src/backend/bir/bir_route4_publication.cpp:96-154,329-341`).
- MIR currently owns a competing final validation: it calls the single-record
  Route4 helper (rescanning PHIs), separately counts same-name PHIs, and compares
  the recovered pointer/name/ID/type and coordinate against prepared fields.
  This makes MIR, rather than one Route4 classification, the final ambiguity and
  stale-coordinate authority (`src/backend/mir/query.cpp:762-859`).

### Blocked cardinality baseline

- **Zero claims:** represented only through broad missing/unavailable statuses
  (`MissingProof`, `ProofUnavailable`, `MissingPublication`, etc.); there is no
  explicit empty collection tied to an exact destination.
- **One claim:** collapsed into `block_entry_publication_proof_attributed=true`
  plus one instruction index after the name/type/coordinate checks pass.
- **Inconsistent claim:** name, type, owner, or coordinate disagreement becomes
  `ProofMismatch` (or Route4 `NoMatch`/wrong key); which claim disagreed and on
  which axis is lost.
- **Duplicate claims:** structural same-name/type PHIs can survive as multiple
  Route4 index records and become `Ambiguous`/`DuplicateReference`, but prepared
  attribution has only one boolean/index slot. Independently attributed claims
  cannot both be represented, so agreement versus true duplicate attribution
  is collapsed before MIR receives it.

### Modeling truth table

| Row | Required exact facts | Current representation/collapse | Required authoritative result |
| --- | --- | --- | --- |
| Same name/type, distinct destination identity | Two destination semantic identities despite equal display name/type | Route4 lookup and MIR's name-only count merge them and report ambiguous | Select/classify only claims for the requested exact destination; the other identity is not a duplicate claim |
| True independently attributed duplicates | Two distinct claim identities assert the same exact destination, each with its own prepared attribution and coordinate | Route4 may see two structural records, but prepared state stores only one attributed bit/index and loses independent claim identity | Preserve both claims and return typed ambiguous/duplicate even when their payloads agree |
| Stale coordinate | Exact destination and claim attribution agree, but the claim's instruction coordinate no longer names its owned instruction | Prepared and MIR both flatten this into `ProofMismatch`; MIR discovers it by rescanning | Return typed stale evidence from pointer/index ownership validation performed once by Route4 |
| Missing attribution | A structural publication may exist, but there is no attributed prepared claim for the destination | `MissingProof`, `ProofUnavailable`, false attributed bit, or (in the MIR overload) `ProofMismatch`; zero claims and unattributed structure are not distinct | Return typed unattributed/missing-evidence, not available and not generic mismatch |

### Smallest shared model boundary

- Introduce one Route4-owned **block-entry publication evidence query/result**
  seam, shared by prepared attribution and MIR. The query contains an exact
  destination semantic identity (successor owner identity plus destination
  value identity/ID, with name/type only descriptive) and an ordered collection
  of proof claims. Each claim has an independent attribution identity, its
  prepared instruction coordinate, and the claimed instruction/value owner.
  The result preserves cardinality and returns one typed classification:
  available (with the selected claim), missing, unattributed, stale,
  inconsistent, or ambiguous/duplicate.
- Route4 alone validates claim pointer/index ownership and classifies the whole
  collection. Prepared state carries that authoritative result (or its stable
  value form) forward; MIR validates/consumes it and does not scan successor
  PHIs. This is the smallest boundary because destination identity, claim
  identity, collection cardinality, coordinate ownership, and typed outcome are
  all necessary to express at least one truth-table row; target materialization,
  storage, ABI, and emission policy are not.

## Suggested Next

- Execute Plan Step 2: define the exact destination, claim, collection, and
  typed-result contracts at the shared Route4/prepared/MIR seam, preserving all
  truth-table rows without changing unrelated query families.

## Watchouts

- Do not use display name/type as destination equality; they are diagnostics.
- Do not deduplicate agreeing claims before classification: independently
  attributed duplicates must remain observable and ambiguous.
- Do not let MIR reconstruct cardinality or coordinate ownership by rescanning
  PHIs; carry the Route4 result across the prepared boundary.
- Keep structural absence distinct from an existing structural publication
  with no prepared attribution, and keep stale coordinate distinct from general
  payload disagreement.

## Proof

- No build/test was delegated for this modeling-only packet, so no
  `test_after.log` was produced or modified.
- Source tracing used `c4c-clang-tool-ccdb function-signatures` first for
  `bir_route4_publication.cpp` and `mir/query.cpp`, followed by bounded source
  inspection at the references above.
- `git diff --check` passed.
