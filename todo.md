Status: Active
Source Idea Path: ideas/open/168_compatibility_bridge_retirement.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Retire LIR and HIR-to-LIR Final-Output Bridges

# Current Packet

## Just Finished

Step 5 - Retire LIR and HIR-to-LIR Final-Output Bridges:
Fenced the LIR extern declaration raw/rendered-name map as a legacy
compatibility and output boundary. `LirModule::record_extern_decl` now resolves
an already-known `LinkNameId` before falling back to `extern_decl_name_map`, so
complete link-name metadata remains authoritative when a later caller records
the same declaration through the raw spelling path.

The retained `extern_decl_name_map` path is now documented as legacy
raw/rendered compatibility in `src/codegen/lir/ir.hpp` and at the HIR-to-LIR
finalization boundary. The focused extern-decl test now proves that a known
`LinkNameId` preempts legacy raw-name fallback while preserving return-type
metadata and output spelling behavior.

Changed files:

| File | Result |
| --- | --- |
| `src/codegen/lir/ir.hpp` | Documented the raw/rendered extern map as legacy compatibility and routed known raw-name records back through existing `LinkNameId` entries. |
| `src/codegen/lir/hir_to_lir/hir_to_lir.cpp` | Documented final extern-decl vectorization as a printer/output boundary with `LinkNameId` authority first. |
| `tests/frontend/frontend_lir_extern_decl_type_ref_test.cpp` | Added focused proof that `LinkNameId` extern declarations preempt legacy raw-name fallback. |
| `todo.md` | Recorded this Step 5 packet result and proof. |

## Suggested Next

Continue Step 5 by inspecting the next LIR/HIR-to-LIR final-output bridge:
prefer a narrow packet around any printer/verifier or final LLVM text path that
still recovers semantic facts from rendered strings instead of using structured
ids or typed mirrors.

## Watchouts

- Preserve final emitted spelling and diagnostics unless the Step 5 packet
  explicitly documents an intentional rendering change.
- `extern_decl_name_map` remains intentionally present for declarations that
  arrive without complete `LinkNameId` metadata; do not treat it as ordinary
  semantic lookup in later packets.
- This packet touched both LIR data structure behavior and the HIR-to-LIR
  final-output boundary comment; supervisor may choose whether this narrow
  proof is enough or whether Step 5 needs a broader acceptance subset later.
- The pre-existing untracked `review/168_step4_hir_bridge_route_review.md`
  remains transient and was not modified.
- No current blockers.

## Proof

Proof command:
`cmake --build build --target frontend_lir_extern_decl_type_ref_test && ctest --test-dir build -j --output-on-failure -R '^frontend_lir_extern_decl_type_ref$'`

Result: passed. Output was written to `test_after.log`.

Also ran `git diff --check`: passed.
