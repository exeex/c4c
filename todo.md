Status: Active
Source Idea Path: ideas/open/649_pointer_global_local_publication_authority.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Prove Representative Integration

# Current Packet

## Just Finished

Completed Step 4 representative integration proof for `pr57861.c`.

Focused proof remained green:

- `backend_dump_riscv64_pointer_global_local_publication` passed.
- `backend_cli_riscv64_pointer_global_local_publication` passed.
- `backend_cli_failure_riscv64_pointer_global_local_publication_live_load_rejection`
  passed, preserving the fail-closed live-load rejection.

Representative evidence:

- `tests/c/external/gcc_torture/src/pr57861.c` emitted an RV64 object at
  `build/agent_state/649_step4_representative_integration/pr57861.o`.
- Disassembly was captured at
  `build/agent_state/649_step4_representative_integration/pr57861.objdump.txt`.
- The `%lv.l` publication site advanced past the prior
  `unsupported_local_memory_access` owner. The representative object contains
  `.Lpcrel_hi_global_pointer_local_publication_2_10_0` with direct global
  address materialization into `t1` followed by the local pointer-slot store:
  `13c: auipc t1, 0x0`, `140: mv t1, t1`, `144: sd t1, 0x0(sp)`.
- Later representative code continues through scalar global/local accesses,
  including `lh`/`sh` sequences and returns; no separate downstream owner was
  exposed by this Step 4 object/disassembly proof.

Focused positive and negative coverage therefore still brackets the Step 3
implementation: direct global-address publication into the exact local pointer
slot is supported, while a reload used as a later memory-address base remains
rejected.

## Suggested Next

Proceed to Step 5 broader RV64 validation and lifecycle recommendation. The
focused and representative evidence both indicate the `%lv.l`
pointer/global local-publication owner is repaired.

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
- Keep the live-load expected-failure coverage intact; Step 4 proves the
  representative dead reload is safely elided, not that arbitrary reloaded
  pointer publications are supported.
- Do not infer authority from source spelling, final assembly order,
  diagnostics, testcase identity, local/global names, or stack-slot shape.
- Do not edit expectations, unsupported markers, allowlists, timeouts,
  runtime-comparison policy, or pass/fail accounting.

## Proof

Step 4 proof passed and was written to `test_after.log`:

```sh
bash -lc 'set -o pipefail; mkdir -p build/agent_state/649_step4_representative_integration && { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_(dump|cli|cli_failure)_riscv64_pointer_global_local_publication" && build/c4cll -I tests/c/external/gcc_torture --target riscv64-linux-gnu --codegen obj tests/c/external/gcc_torture/src/pr57861.c -o build/agent_state/649_step4_representative_integration/pr57861.o && llvm-objdump -d build/agent_state/649_step4_representative_integration/pr57861.o > build/agent_state/649_step4_representative_integration/pr57861.objdump.txt && rg -n "<foo>|global_pointer_local_publication|sd\s+t1|lh|sh|ret|auipc" build/agent_state/649_step4_representative_integration/pr57861.objdump.txt; } 2>&1 | tee test_after.log'
```

Result: build succeeded, all three focused tests passed, representative
`pr57861.c` emitted and disassembled successfully, and the evidence grep found
the direct publication sequence plus later representative `lh`/`sh`/`ret`
instructions.
