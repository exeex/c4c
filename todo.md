Status: Active
Source Idea Path: ideas/open/613_abi_call_result_stack_frame_lowering.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Add Supported Frame Or Return Handling

# Current Packet

## Just Finished

Completed Step 3, "Add Supported Frame Or Return Handling", refresh and
classification only. No implementation files, plan files, idea files,
expectations, unsupported markers, allowlists, or timeout/accounting files were
touched.

Current focused `build/c4cll --codegen obj --target riscv64-linux-gnu` probes
confirm the accepted Step 2 positives remain guard-stable:

- `src/20000603-1.c` remains past `unsupported_call_abi` and now stops at
  downstream `unsupported_terminator_fragment`.
- `src/20021219-1.c` remains past `unsupported_call_abi` and now stops at
  downstream `malformed_prepared_join_transfer_carrier`.
- `src/pr77767.c` still compiles through RV64 object codegen.

Refreshed Step 3 stack-frame residuals from the prior `unsupported_stack_frame`
bucket expose a real consumer packet with same-family breadth and explicit
prepared facts:

- Supported stack-frame consumer candidates with complete prepared frame facts:
  `src/20020314-1.c`, `src/20021113-1.c`, `src/20040223-1.c`,
  `src/20040811-1.c`, `src/920721-2.c`, `src/920929-1.c`,
  `src/alloca-1.c`, `src/pr36321.c`, `src/pr43220.c`, `src/strcpy-2.c`,
  and `src/vla-dealloc-1.c` still stop first at
  `unsupported_stack_frame: RV64 object route requires a supported prepared
  stack frame`.
- Representative prepared dumps show explicit frame sizes/alignments, dynamic
  stack metadata when present, and concrete callee-saved GPR slots or
  preservation facts. Examples: `src/20040811-1.c`, `src/pr43220.c`, and
  `src/vla-dealloc-1.c` publish `frame_size=24/40/24`,
  `frame_alignment=8`, `has_dynamic_stack=yes`, `requires_stack_save_restore=yes`,
  and `gpr:s1`/`gpr:s2` preservation facts.
- Some old frame-bucket rows are no longer Step 3 frame candidates:
  `src/20000603-1.c` is now a terminator guard; `src/20030209-1.c`,
  `src/20040313-1.c`, and `src/20040805-1.c` now stop at ambiguous
  non-parallel stack-destination move-bundle authority; others stop at compare
  publication, call ABI, global data, instruction fragments, or terminate
  successfully.

The prepared return rows do not expose a complete-authority RV64 return
consumer packet in this refresh:

- `src/20001130-2.c` and `src/20080719-1.c` still stop at
  `unsupported_move_bundle_target_shape` for before-return
  `return_stack_to_register` moves.
- Their diagnostics report `destination_kind=function_return_abi` and
  `destination_storage=register`, but also `destination_home_kind=stack_slot`;
  current docs classify that as `prepared_return_abi_destination_home_authority`.
  Treat this as a missing prepared authority gap, not RV64 consumer progress.

Other refreshed residuals remain outside this Step 3 consumer packet: local and
global producers, runtime/library/variadic policy, FPR-heavy frame details,
generic move-bundle authority production, and unrelated instruction or
terminator owners.

## Suggested Next

Implement a Step 3 supported prepared stack-frame consumer packet for the
complete-facts GPR dynamic/fixed frame family. Use `src/20040811-1.c`,
`src/pr43220.c`, and `src/vla-dealloc-1.c` as representative positives because
they share explicit frame size/alignment, stack-save/restore metadata, and
callee-saved GPR preservation facts. Keep `src/20020314-1.c` as a compatibility
watchout because it includes FPR callee-saved details and dynamic stack facts.

## Watchouts

- Keep `src/20000808-1.c` under idea 624 and `src/20020529-1.c` under idea
  625; do not reclassify missing prepared authority as RV64 consumer progress.
- Do not infer return destination home authority for `src/20001130-2.c` or
  `src/20080719-1.c`; their before-return rows remain producer-authority gaps.
- Do not expand the next frame packet into FPR save/restore, generic
  move-bundle authority production, local/global producer repair,
  runtime/library/variadic policy, or named-case-only frame handling.
- Preserve Step 2 positive guards: `src/20000603-1.c`, `src/20021219-1.c`,
  and `src/pr77767.c`.

## Proof

Ran exactly:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`

Result: passed, `346/346` backend tests; `test_after.log` is the preserved
proof log.
