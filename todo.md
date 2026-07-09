Status: Active
Source Idea Path: ideas/open/649_pointer_global_local_publication_authority.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Run Broader Validation And Close Or Park

# Current Packet

## Just Finished

Repaired the Step 5 RV64 regression-guard blocker from the Step 3 code slice.

The new failing test was
`backend_cli_riscv64_local_string_constant_label_pointer_compound_arg`, which
failed at `pass_compound_literal` instruction 0 with
`unsupported_local_memory_access ... access_base=none`.

Root cause: the Step 3 tightening of
`prepared_memory_access_for_local_instruction(...)` required a named stored
value before accepting the indexed `StoreLocalInst` access. That preserved
pointer/global publication exactness for named stores, but accidentally dropped
valid anonymous same-instruction frame-slot accesses used by existing
string-label/compound local-memory support for zero-initializing compound
literal fields.

Repair: keep named stores exact, and restore the indexed access only when both
the BIR store value and prepared access have no result/stored value identity.
This lets anonymous compound-literal frame-slot stores find their prepared
access without broadening unknown local-memory access or weakening the
pointer/global publication checks.

The regressed compound string-label object test now passes again. The idea 649
focused positive object route, focused dump route, live-load expected-failure
negative route, and representative `pr57861.c` object emission all still pass.

## Suggested Next

Proceed with the supervisor-selected broader RV64 regression guard for Step 5.
This blocker repair has a green focused proof, but it does not replace the
matched before/after broader validation needed for lifecycle close.

## Watchouts

- Do not treat scalar frame-slot local-memory facts from idea 640 as
  pointer/global local-publication authority.
- Do not reopen direct global-symbol local-memory support from idea 631 or
  generic pointer freshness from idea 600 unless fresh evidence proves a new
  local-publication boundary.
- Do not use the `main` call-argument direct-global select-chain evidence as
  the owner for this idea; the representative owner is inside `foo` around
  `%lv.l`.
- Do not rewrite prepared provenance or mark all unknown local pointer slots as
  supported. The discovered positive route depends on exact publication,
  direct-global identity, slot identity, and ordering.
- Keep named `StoreLocalInst` lookup exact for pointer/global publication; only
  anonymous same-instruction accesses with no prepared value identity are
  restored by this repair.
- Keep the live-load expected-failure coverage intact; the representative dead
  reload is safely elided, but arbitrary reloaded pointer publications are not
  supported.
- Do not infer authority from source spelling, final assembly order,
  diagnostics, testcase identity, local/global names, or stack-slot shape.
- Do not edit expectations, unsupported markers, allowlists, timeouts,
  runtime-comparison policy, or pass/fail accounting.

## Proof

Blocker repair proof passed and was written to `test_after.log`:

```sh
bash -lc 'set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(backend_cli_riscv64_local_string_constant_label_pointer_compound_arg|backend_(dump|cli|cli_failure)_riscv64_pointer_global_local_publication)" && mkdir -p build/agent_state/649_step4_representative_integration && build/c4cll -I tests/c/external/gcc_torture --target riscv64-linux-gnu --codegen obj tests/c/external/gcc_torture/src/pr57861.c -o build/agent_state/649_step4_representative_integration/pr57861.o; } 2>&1 | tee test_after.log'
```

Result: build succeeded, all four focused tests passed, including the
previously regressed compound string-label object test, and representative
`pr57861.c` still emitted
`build/agent_state/649_step4_representative_integration/pr57861.o`.
