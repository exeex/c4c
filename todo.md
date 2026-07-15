# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 7.25
Current Step Title: Receive typed PHI incoming authority

## Just Finished

- Step 7.25: received one typed `LirPhiOp` result and ordered typed incoming
  value/predecessor rows into Raw-BIR `PhiNode` transactionally. The receiver
  reserves PHI results for loop backedges, preserves native SpecialToken
  authority without inspecting its display mirror, and binds a row only to its
  uniquely selected semantic CFG edge.
  Raw BIR retains duplicate conditional/switch successor occurrences, while a
  predecessor-only PHI row facing parallel occurrences rejects rather than
  inventing an occurrence from input order.

## Suggested Next

- Supervisor: assess the Step 7.25 exact-parallel boundary. The current
  `LirPhiIncoming` schema carries a predecessor but no occurrence identity, so
  semantic receipt of a particular parallel edge requires a separately scoped
  authority extension; do not assign it from PHI input order.

## Watchouts

- Do not recover values, predecessors, edges, or semantics from labels,
  spellings, printer output, LLVM text, or instruction order.
- The verifier requires every Raw-BIR PHI edge occurrence exactly once. The
  importer accepts only a unique source edge and rejects ambiguous parallel
  predecessor-only rows before publication.
- PHI SpecialTokens are accepted or rejected exclusively by their native
  carrier domain and type; display text is non-semantic.
- PHI, local/object, memory/va, aggregate/vector, body-parameter, and all
  other later families remain fail-closed outside this packet.

## Proof

- Executor ran `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^backend_'`; passed 5/5. `test_after.log` contains
  the full proof output. Supervisor owns broader acceptance and any separately
  scoped edge-occurrence authority work.
