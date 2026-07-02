# Current Packet

Status: Active
Source Idea Path: ideas/open/532_bir_local_array_semantic_gep_header_readiness.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Extract Only A Safe Analysis Header Boundary

## Just Finished

Step 2 of `plan.md` extracted the first safe local-array/semantic-GEP
analysis boundary into
`src/backend/bir/bir_local_array_semantic_gep.hpp`.

The new header is aggregator-only. It is included from `bir.hpp` immediately
after `bir_memory_provenance.hpp`, where `Value`, route prereqs, and memory
provenance declarations are already available. It contains the Step 1 safe
cluster only: local-array source/path declarations, local-array proof records
and early evaluators/matchers, local-address provenance records/evaluator,
local semantic-GEP records/evaluator, and global static-GEP
authority/semantic-GEP records/evaluator.

No direct consumer include replacements were made. `Function` storage, the
`Function`-taking `evaluate_local_array_interval_effect` overload, scalar
local-load declarations, and scalar local-load inline behavior remain in
`bir.hpp`. The scalar local-load declaration block was kept in `bir.hpp` but
ordered after the new aggregator include so it can continue referencing the
local-address provenance record without moving scalar-load behavior.

No build-system public header list was found or edited; `src/backend` uses
globbed sources and public include directories rather than an explicit BIR
header install/list entry.

## Suggested Next

Delegate Step 3: probe consumer include narrowing with temporary syntax-only
include checks. Keep consumers on `bir.hpp` where they still require complete
`Function`, `Inst`, route, lowering, or prepared-module declarations.

## Watchouts

- `bir_local_array_semantic_gep.hpp` is not standalone in this slice; it relies
  on the `bir.hpp` aggregator prerequisite order.
- Do not replace consumer includes without proving the consumer no longer
  needs complete core BIR model, route, lowering, or prealloc declarations.
- Keep scalar local-load inline behavior parked in `bir.hpp`; it still needs
  complete `LoadLocalInst` and `MemoryAddress`.
- The first proof attempt hit a transient `cc1plus` killed failure while
  compiling `backend_aarch64_instruction_dispatch_test.cpp`, outside the
  delegated slice. Rerunning the exact delegated proof completed cleanly.

## Proof

Ran the delegated proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_(lir_to_bir_notes|publication_plan_record|prepare_stack_layout)' > test_after.log 2>&1`

Result: passed on rerun. `test_after.log` records 3/3 tests passing:
`backend_lir_to_bir_notes`, `backend_publication_plan_record`, and
`backend_prepare_stack_layout`.
