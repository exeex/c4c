Status: Active
Source Idea Path: ideas/open/354_rv64_gcc_torture_prepared_module_shape_classification.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Refresh Or Validate Classification Evidence

# Current Packet

## Just Finished

Step 2 validated the classification evidence for closure readiness.

Evidence reviewed:

- `review/354_prepared_shape_classification.md`: original full-scan
  classification of `1012` opaque prepared-module-shape failures into
  `540` multi-block CFG, `378` globals/non-string global addresses, `70`
  strings, `11` general calls, `6` non-i32/pointer local memory width, `3`
  variadic/vararg, `2` declaration control-flow entries, `1` FPR ABI value,
  and `1` local memory addressing/home bucket.
- `build/agent_state/354_step3_representative_refresh.log`: representative
  refresh over `18` bucket examples, with `11` pass and `7` fail at that point;
  these failures produced the later child chain rather than unclassified
  umbrella work.
- Current scan artifacts:
  `build/agent_state/rv64_gcc_c_torture_backend_summary.tsv` and
  `build/agent_state/rv64_gcc_c_torture_backend_failed.txt`, timestamped
  2026-06-26 06:09. The summary covers `1467` cases: `208` pass and `1259`
  fail.

Current scan diagnostic summary:

| Current family | Count | Classification/owner status |
| --- | ---: | --- |
| `semantic_lir_to_bir` / other semantic handoff failures | 452 | Non-prepared-object handoff family, outside idea 354 repair scope. |
| `unsupported_instruction_fragment` | 383 | Classified object-route instruction fragment family; covered by closed 377/378 continuation owners. |
| `unsupported_move_bundle_target_shape` plus `missing_move_bundle` | 90 | Classified prepared move-bundle family; covered by closed 376 and consumer-contract work under 359. |
| `unsupported_terminator_fragment` | 86 | Classified terminator/CFG lowering family; covered by closed 369 and later CFG/publication children. |
| `unsupported_stack_frame` | 78 | Classified stack-frame / callee-save / frame-shape family; covered by closed object-route architecture and parameter/home children. |
| `unsupported_param_home` | 43 | Classified ABI/home family; covered by closed 363/374 and related parameter-home children. |
| `unsupported_scalar_compare_trunc` | 36 | Covered by closed 375. |
| `unsupported_global_data` | 29 | Covered by closed 357, 383, and 384 global/data publication children. |
| `unsupported_local_memory_access` | 26 | Covered by closed 368 and 382 local-memory/addressing children. |
| runtime abort/mismatch representatives | 28 | Later runtime owner chain, covered by closed 379-381 and 387-394 where relevant. |
| `unsupported_floating_cast` | 6 | Classified ABI/FPR edge, covered by the original ABI-width edge child 358. |
| `unsupported_variadic_helper_lowering` / variadic admission | 2 | Covered by closed variadic chain 360-367, 388-394. |

Conclusion: existing evidence is current enough for closure decision. The
current full scan still has failures, but they are structured diagnostics or
semantic handoff/runtime families rather than opaque unclassified
prepared-module-shape buckets. No unowned prepared-module-shape bucket was
found after comparing residual diagnostics with the closed child set
355-359, 360-367, 368-375, 376-382, 383, 384, 386, and 387-394. A fresh full
scan is not needed for the classification umbrella closure question unless the
plan owner wants a newer progress number for documentation.

## Suggested Next

Proceed to Step 4 close decision / plan-owner handoff. No Step 3 residual-owner
split is recommended from this evidence because no unowned prepared-shape
bucket was found.

## Watchouts

- Do not implement RV64 lowering repairs from this umbrella.
- Do not edit idea 385 or mix EV64 `.insn.d` encoding work into idea 354.
- If fresh scan evidence exposes a new repairable backend family, split it to
  a focused `ideas/open/` child idea instead of expanding 354.
- Keep source idea edits rare; routine audit notes belong here or under
  `review/`.
- The current scan is not green; closure readiness here means the
  prepared-shape classification umbrella has classified and routed the
  remaining families, not that every gcc_torture object-route case passes.
- Idea 385 remains unrelated EV64 `.insn.d` encoding work and should not be
  mixed into idea 354 closure.

## Proof

Read-only validation only; no build, test, repair, full scan, or root log write
was run. Checks performed:

- Read `review/354_prepared_shape_classification.md`.
- Read `build/agent_state/354_step3_representative_refresh.log`.
- Inspected `build/agent_state/rv64_gcc_c_torture_backend_summary.tsv` and
  `build/agent_state/rv64_gcc_c_torture_backend_failed.txt`.
- Parsed current case logs referenced by the summary to count residual
  diagnostics and sampled representative residual logs for
  `unsupported_function_admission`, `missing_move_bundle`,
  `unsupported_floating_cast`, `unsupported_stack_frame`, and semantic
  LIR-to-BIR handoff failures.
