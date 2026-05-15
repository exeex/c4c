Status: Active
Source Idea Path: ideas/open/243_aarch64_cache_hint_builtin_intrinsic_carriers.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Broader Backend Validation And Closure Readiness

# Current Packet

## Just Finished

Plan-owner review after Plan Step 4 accepted the builtin-address ownership
boundary and advanced the route to Plan Step 6 validation/closure readiness.

Step 5 boundary status:

- Existing dispatch and machine-printer tests already prove complete DC CVAU
  cache-maintenance and hint-yield carriers remain non-selected and unprinted.
- No selected-machine records, dispatch cases, or printer spelling were added
  for cache-maintenance, pause/hint, or builtin-address families.
- No additional Step 5 implementation packet is needed unless the broader
  backend validation exposes a regression.

Ownership decision:

- No concrete non-materialization builtin-address representative is currently
  owned by this intrinsic-carrier route.
- Existing direct, GOT, string, and label address cases already flow through
  structured address-materialization carriers and selected AArch64 address
  materialization records.
- Existing thread-local address cases already flow through TLS materialization
  facts, including the local-exec thread-pointer-relative `tpidr_el0` sequence.
- Those cases must stay with address-materialization or TLS ownership instead
  of being reclassified as builtin-address intrinsic carrier support.
- Builtin-address support should not be claimed from archived legacy intrinsic
  text, generic call plans, scratch-register conventions, or rendered assembly.

No accepted builtin-address carrier was added. This route should not claim
builtin-address intrinsic-carrier support beyond the recorded ownership
boundary unless a later source idea names a concrete non-materialization
representative.

## Suggested Next

Run Plan Step 6 broader backend validation from the supervisor side:

```sh
set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'; } 2>&1 | tee test_after.log
```

If the broader backend subset is green, ask the plan owner to close
`ideas/open/243_aarch64_cache_hint_builtin_intrinsic_carriers.md`. No split is
needed right now because builtin-address has no concrete non-materialization
representative in this source idea.

## Watchouts

- Keep this route limited to cache-maintenance, pause/hint, and
  builtin-address carrier authority.
- Do not add selected-machine records, dispatch support, or printer spelling in
  the carrier packets.
- DC CVAU and hint-yield support are carrier-only. Later machine-node routes
  must consume the structured carriers rather than rediscovering support from
  intrinsic names or rendered text.
- Do not treat prepared address materialization or TLS metadata as
  builtin-address intrinsic authority. Existing direct/GOT/TLS global, string,
  and label address materializations already have their own carrier and
  selected-machine routes.
- Builtin-address remains a recorded ownership boundary unless a later source
  idea names a concrete non-materialization representative.
- `llvm.aarch64.hint` malformed LIR cases can lower without producing
  `HintYield` authority in this harness; the Step 3 BIR test explicitly guards
  against producing a semantic carrier for unsupported immediate/result cases.

## Proof

Latest delegated proof before plan-owner review:

```sh
set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_(lir_to_bir_notes|prepared_printer|aarch64_instruction_dispatch|aarch64_machine_printer)'; } 2>&1 | tee test_after.log
```

Result: passed. Build was up to date and all 4 delegated tests passed:
`backend_lir_to_bir_notes`, `backend_prepared_printer`,
`backend_aarch64_instruction_dispatch`, and
`backend_aarch64_machine_printer`.

Log path: `test_after.log`.
