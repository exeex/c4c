Status: Active
Source Idea Path: ideas/open/260_phase_f3_prepared_module_structural_one_reader_candidates.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Add Nearby Fail-Closed Rows

# Current Packet

## Just Finished

Step 3 fixed the recovered-source compatibility regression while preserving the
prepared/BIR disagreement boundary.

- Clarified that absent Route 3 BIR same-block identity is treated as no BIR
  evidence, so complete prepared frame-slot recovered-source facts remain
  usable for prepared-only fixtures.
- Kept present complete BIR identity authoritative as a cross-check: if Route 3
  returns load/store memory identity that disagrees with complete prepared
  frame-slot/source facts, the recovered-source helper still returns
  `std::nullopt`.
- Updated the nearby helper rows so missing BIR load/store access and fully
  missing BIR identity prove prepared-only compatibility, while the explicit
  prepared/BIR byte-offset mismatch row still proves fail-closed behavior.

## Suggested Next

Execute Step 4 broader backend validation for the recovered-source identity
candidate.

## Watchouts

- Keep this packet limited to the recovered-source identity candidate.
- Missing Route 3 BIR memory identity is compatibility fallback, not
  disagreement. Only a complete present BIR identity can reject prepared
  recovered-source facts for prepared/BIR mismatch.
- Do not absorb the byval pointer-source, direct-global select-chain, or
  source-value/source-producer metadata candidates.
- The Route 3 stored-value identity only confirms same local-slot range
  evidence; the prepared recovered-source helper remains the policy owner for
  the narrow-store/wide-load scan and width checks.
- Missing source-producer and wrong source-producer-kind rows belong at the
  caller/source-producer selection surface, not the direct recovered-source
  helper signature covered by this packet.
- Preserve prepared publication planner policy, source-producer lookup
  authority at the caller, public prepared aggregate compatibility, and current
  fail-closed behavior.
- Do not rewrite output expectations, diagnostics, helper statuses, or target
  output to claim progress.
- Surfaces explicitly out of scope for this runbook packet: byval formal pointer-source
  classification, direct-global select-chain dependency lookup, source-value
  and source-producer metadata packets, target output/diagnostics/baselines,
  store publication status semantics, frame-slot policy, pointer-base homes,
  pending publication policy, and duplicate-publication handling.

## Proof

Ran:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prepared_lookup_helper|backend_store_source_publication_plan)$'`

Result: passed. `backend_prepared_lookup_helper` and
`backend_store_source_publication_plan` passed 2/2. `git diff --check` also
passed.

Proof log: `test_after.log`.
