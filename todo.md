Status: Active
Source Idea Path: ideas/open/60_aarch64_dispatch_prepared_publication_decomposition.md
Source Plan Path: plan.md
Current Step ID: Step 5
Current Step Title: Resume Implementation On The Narrowest Generic Seam

# Current Packet

## Just Finished

Completed Step 5 - Resume Implementation On The Narrowest Generic Seam.

Implemented the GOT-required prepared `LoadGlobal` materialization seam by
using a shared `prepared_load_global_address_policy` helper for prepared
global-load lowering. Explicit prepared address policies remain authoritative,
but an unspecified prepared address now falls back to the BIR global's explicit
address materialization policy before using the target-profile direct fallback.

Added and registered
`backend_codegen_route_aarch64_got_load_global_prepared_memory` with a focused
external global load probe that requires GOT page/low12 materialization and
forbids direct `:lo12:external_data_symbol` addressing.

## Suggested Next

Supervisor should decide whether Step 5 is acceptance-ready for commit or
whether the next packet should extract one of the remaining monolithic
dispatch assertions into a focused route probe before touching another owner.

## Watchouts

- The focused route probe requires GOT page/low12 materialization and a generic
  final word load signal (`ldr w`) without binding the test to temporary
  register names.
- The delegated proof no longer has the prior
  `expected GOT-required global load to use GOT page/low12 materialization`
  first-bad assertion in `backend_aarch64_instruction_dispatch`.
- The pre-existing dirty stack in do-not-touch files and the untracked
  `review/step3_dispatch_route_review.md` remain outside this packet.

## Proof

Passed:

```sh
(cmake --build build -j && ctest --test-dir build -j --output-on-failure -R '^(backend_codegen_route_aarch64_got_load_global_prepared_memory|backend_aarch64_instruction_dispatch)$') > test_after.log 2>&1
```

Proof log path: `test_after.log`.
