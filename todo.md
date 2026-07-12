# Current Packet

Status: Active
Source Idea Path: ideas/open/709_aarch64_named_handoff_materializer_cleanup.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Migrate sibling AArch64 materializers

## Just Finished

- Plan Step 2 migrated the AArch64 call-boundary same-block publication source
  lookup in `calls.cpp` from a locally rebuilt Route 4 publication-availability
  index to the common named-producer record, retaining the prepared
  current-block publication query as the cross-block fallback. It also removed
  the unused `Route6CallUseSourceIndex` parameter from the adjacent direct-global
  select call-argument materialization API and its sole call site.

## Suggested Next

- Continue Plan Step 2 with one narrow comparison or ALU authority-family
  packet selected by the supervisor; keep the packet limited to one semantic
  route-index consumer family and its focused proof.

## Watchouts

- The call-boundary source path now accepts only the common same-block named
  producer record or prepared current-block publication consumption; the raw
  instruction visitor fallback was removed so missing authority fails closed.
- Comparison and ALU route-index construction remains outside this completed
  packet and still belongs to Step 2.
- The delegated subset retains the known baseline failure in test 354 (`bl
  printf` missing); all other 34 tests pass, including the call-boundary
  scalability and prepared-memory records coverage.

## Proof

- Ran `set -o pipefail; { cmake --build --preset default && ctest --test-dir
  build -j --output-on-failure -R
  '^(backend_aarch64_prepared_memory_operand_records|backend_(codegen_route|cli)_aarch64_)';
  } 2>&1 | tee test_after.log`. Build succeeded; 34/35 tests passed, with only
  the known test 354 failure (`bl printf` missing). The delegated proof is
  sufficient relative to that baseline. Proof log: `test_after.log`.
