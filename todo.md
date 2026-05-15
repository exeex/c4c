Status: Active
Source Idea Path: ideas/open/243_aarch64_cache_hint_builtin_intrinsic_carriers.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Define Builtin-Address Ownership And Carriers

# Current Packet

## Just Finished

Plan Step 4 - Define Builtin-Address Ownership And Carriers completed the
builtin-address ownership decision without implementation changes.

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

No accepted builtin-address carrier was added. This step asks for lifecycle
review before claiming builtin-address support from this route.

## Suggested Next

Ask the plan owner to review whether this route should close after the cache,
pause/hint, and builtin-address handoff work, or split any remaining
builtin-address investigation into a separate idea with a concrete
non-materialization representative.

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
- Builtin-address remains a recorded ownership boundary unless a later
  lifecycle decision names a concrete non-materialization representative.
- `llvm.aarch64.hint` malformed LIR cases can lower without producing
  `HintYield` authority in this harness; the Step 3 BIR test explicitly guards
  against producing a semantic carrier for unsupported immediate/result cases.

## Proof

Delegated proof:

```sh
set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_(lir_to_bir_notes|prepared_printer|aarch64_instruction_dispatch|aarch64_machine_printer)'; } 2>&1 | tee test_after.log
```

Result: passed. Build was up to date and all 4 delegated tests passed:
`backend_lir_to_bir_notes`, `backend_prepared_printer`,
`backend_aarch64_instruction_dispatch`, and
`backend_aarch64_machine_printer`.

Log path: `test_after.log`.
