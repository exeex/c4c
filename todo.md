# Current Packet

Status: Active
Source Idea Path: ideas/open/719_bir_cfg_edge_publication_source_identity_completion.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize request loss and independent BIR authority

## Just Finished

- Completed Plan Step 1 localization without implementation changes.
- The first discarded `BirCfgEdgePublicationSourceRequest` fact is at
  `prepared_and_bir_cfg_edge_publication_source_identity_match`: the helper
  accepts `request` by value, never reads it, prepares facts from its separate
  label/value-id arguments, and calls the legacy
  `find_bir_cfg_edge_publication_source_identity(names, prepared)` overload.
  That overload has no request or BIR blocks and derives every returned field
  from `PreparedEdgeCopySourceFacts`, so prepared facts currently act as both
  sides of the claimed comparison. The owning common repair seam is the public
  query boundary in `src/backend/mir/query.hpp/.cpp`; the test helper should
  pass the request to that boundary rather than select BIR identity itself.
- Independent BIR CFG authority map:
  - predecessor and successor: `request.predecessor_block` and
    `request.successor_block`, with each `Block::label_id` as semantic authority
    and label text only as compatibility authority; request id/text must agree
    when supplied
  - destination: the leading `PhiInst` in the successor whose `result` matches
    request destination name and type; its owning `Inst*`, `PhiInst*`, index,
    `Value*`, value identity, name, and type are authoritative; request pointer,
    id/name/text/type are constraints, not facts copied into the result
  - source: the matching `PhiIncoming` selected by predecessor label id (text
    fallback only when ids are unavailable); `PhiIncoming::value` owns source
    pointer, kind, name, type, and immediate identity
  - producer instruction: Route 1's predecessor-block producer index and
    `route1_find_same_block_scalar_producer` own the exact `Inst*`, produced
    `Value*`, instruction index, block label id, and generalized kind for
    `LoadLocalInst`, `LoadGlobalInst`, `CastInst`, `BinaryInst`, and
    `SelectInst`; named sources without an exact producer fail closed, while
    immediate sources require no instruction
  - producer memory: Route 3's predecessor-block memory-access index at that
    exact producer instruction owns instruction pointer/index, node/base kind,
    local slot or global symbol id/name, result value, address space,
    volatility, offset, size, alignment, and provenance. Load-local and
    load-global are the memory-producing shapes; cast/binary/select must not
    synthesize memory identity.
- Existing Route 5 CFG authority (`route5_cfg_edge_publication_record`, or the
  indexed `route5_find_cfg_edge_publication`) already performs the edge/phi,
  incoming-source, and Route 1 producer resolution. Its typed negative owner is
  `Route5PublicationStatus`: missing predecessor/successor/destination,
  missing publication, explicit `NoSource`, missing source producer, missing
  or incomplete memory access, and destination `NoMatch`. The index owns
  stale/mismatched edge rejection and must detect duplicate exact matches
  instead of choosing by row order; these states must remain unavailable when
  translated to MIR status. A request with absent or internally inconsistent
  block/destination keys must fail before lookup. An unavailable named source
  retains destination/source evidence but remains `MissingSourceProducer`.

## Suggested Next

- Implement Step 2 at the public MIR query boundary: add a request-consuming
  overload that validates request keys, resolves one exact Route 5 BIR CFG
  record, translates it into `BirCfgEdgePublicationSourceIdentity`, and only
  then lets the test compare that independent result with prepared facts.

## Watchouts

- Bounded semantic repair rule: resolve by exact block identity plus phi
  destination identity, exact predecessor incoming, exact Route 1 producer,
  and (for load-local/load-global only) exact Route 3 memory record. Require a
  unique match and preserve the most specific typed negative state. Never use
  prepared facts, fixture row order, names alone when valid ids exist, or a
  load-local testcase branch to choose the BIR result.
- Route 5 currently attaches Route 3 memory identity only for load-local;
  load-global is classified as a producer but returned as non-memory. Step 2
  must either generalize the common Route 5 memory attachment to both load
  kinds or report that API gap rather than filling global-memory fields from
  prepared facts.
- `BirCfgEdgePublicationSourceStatus` is less expressive than Route 5 for
  `NoSource`, `NoMatch`, missing/incomplete memory, stale/mismatch, and
  duplicate/ambiguous records. Translation must not collapse any such state
  to `Available`; extend the typed MIR status if preserving distinctions is
  required by the focused negative contract.

## Proof

- Per the supervisor packet, no build or test was run for this read-only
  localization; the existing focused failure baseline and canonical logs were
  preserved.
