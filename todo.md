# Current Packet

Status: Active
Source Idea Path: ideas/open/717_prepared_mir_join_source_identity_completion.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize the prepared-to-MIR join identity divergence

## Just Finished

- Step 1 localized the first incorrect join-source identity fact to
  `mir::prepared::PreparedMirFunctionView::current_block_direct_edge_publication_sources`
  in `src/backend/mir/prepared_view.cpp`.
- Preparation is still correct in
  `prepare::prepare_current_block_join_parallel_copy_source_facts`: the named
  row is `Available` with selected direct-edge publication/move freshness and
  exact predecessor, successor, destination, source, and homes; the immediate
  row is deliberately `Available` with `NoCandidate` freshness because an
  immediate has no named-value freshness authority; the stack row is
  `Available` with its exact stack source/home and selected freshness; and the
  fourth row is explicitly `UnsupportedMove`.
- The earliest bad fact is the prepared-MIR adapter's unconditional
  `Available`-row freshness check. It requires a
  `source_freshness_authority` even when `fact.immediate_source` is true, so
  the correctly prepared immediate row is reclassified from `Available` to
  `InvalidSourceFreshness`. The adapter then takes its early negative-status
  branch before copying the row's predecessor/successor, destination/source,
  move-derived storage, and immediate identity fields.
- The same copy-after-status-gate ordering also leaves the typed
  `UnsupportedMove` row with default/invalid identity keys. Downstream,
  `mir::find_bir_current_block_join_source_identity` correctly keeps the
  aggregate unavailable when any row is negative, but currently collapses
  every prepared-MIR negative row to `MissingPublication`; its per-row view
  therefore cannot preserve the prepared `UnsupportedMove` category or useful
  structural keys.
- General repair rule: adapt identity independently from availability. Copy
  every fact's exact structural publication/move keys before status gating;
  require selected freshness only for named sources (never synthesize it for
  immediates); preserve the prepared typed negative status for unsupported or
  incomplete rows; and let the BIR aggregate become `Available` only when all
  required rows are available. Do not select, recover, or correlate authority
  by row position, names alone, diagnostic text, or target behavior.

## Suggested Next

- Execute Step 2 in the common prepared-MIR/BIR adapters: repair
  `current_block_direct_edge_publication_sources` at the first bad fact, then
  preserve typed negative status in
  `find_bir_current_block_join_source_identity` without weakening aggregate
  fail-closed semantics.

## Watchouts

- Preserve idea 716's completed prepared-call assertions and keep x86
  joined-control work with idea 708.
- The immediate row's `NoCandidate` freshness is valid, not missing evidence.
  Named register and named stack sources must continue to require exact
  selected publication/move freshness.
- Copying structural keys for `UnsupportedMove` must not make that row
  available; supported sibling rows must remain intact while the BIR aggregate
  remains unavailable.

## Proof

- No build or tests run, as delegated for this read-only localization packet.
  Evidence came from AST-backed definition/callee queries plus the focused
  four-shape fixture and the three owning helpers. No `test_after.log` was
  required by the delegated proof contract.
