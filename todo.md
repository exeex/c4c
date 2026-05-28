Status: Active
Source Idea Path: ideas/open/58_aarch64_prepared_authority_regression_recovery.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Repair The Remaining `00196` Family

# Current Packet

## Just Finished

Completed idea 58 Step 3 repair for the remaining `00196` family. The AArch64
block-entry edge-copy consumer now suppresses an out-of-SSA block-entry move
when that move would write a general-purpose register that the current prepared
edge-publication source materializer must still read.

The repair is semantic to prepared edge publication: it uses the prepared
publication's source producer and `edge_value_publication_may_read_register_index`
instead of matching test names, labels, temporary names, fixed registers, or
stack offsets. In the observed short-circuit RHS-call shape, this prevents the
preservation move from clobbering the call result register before the
`select_materialization` publication rematerializes the phi/select source.

## Suggested Next

Next packet: supervisor/reviewer should decide whether Step 3 needs a broader
acceptance sweep or whether this slice is ready to commit with the focused
proof now green.

## Watchouts

- Do not reopen closed idea 60 seams unless the `00196` investigation proves a
  shared semantic owner.
- The suppression check currently considers available block-entry edge
  publications for the current predecessor block and only general-purpose
  destination registers.
- The focused proof kept dispatch, dynamic-stack fixed-slot FP anchoring,
  `00207`, `00208`, and nearby `00033`/`00164` short-circuit probes green.

## Proof

Ran the delegated proof command exactly and wrote output to `test_after.log`:

```sh
(cmake --build build -j && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_instruction_dispatch|backend_codegen_route_aarch64_dynamic_stack_fixed_slot_uses_fp_anchor|c_testsuite_aarch64_backend_src_00033_c|c_testsuite_aarch64_backend_src_00164_c|c_testsuite_aarch64_backend_src_00196_c|c_testsuite_aarch64_backend_src_00207_c|c_testsuite_aarch64_backend_src_00208_c)$') > test_after.log 2>&1
```

Recorded result: build passed and all 7 selected tests passed, including
`c_testsuite_aarch64_backend_src_00196_c`,
`c_testsuite_aarch64_backend_src_00033_c`,
`c_testsuite_aarch64_backend_src_00164_c`,
`c_testsuite_aarch64_backend_src_00207_c`,
`c_testsuite_aarch64_backend_src_00208_c`,
`backend_aarch64_instruction_dispatch`, and
`backend_codegen_route_aarch64_dynamic_stack_fixed_slot_uses_fp_anchor`.
