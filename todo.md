Status: Active
Source Idea Path: ideas/open/358_rv64_object_route_abi_width_edges.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Route Simple Call and FPR ABI Edges

# Current Packet

## Just Finished

Completed Plan Step 4 RV64 object-route direct-call lowering for simple
prepared FPR register call arguments/results.

The direct-call fragment now keeps existing GPR behavior and additionally
admits identity-backed FPR register-to-register call edges for simple
`SameModule`/`DirectExternFixedArity` calls: prepared FPR argument homes move to
ABI FPR argument placements with `fmv.s`/`fmv.d`, the call relocation is still
emitted through the existing AUIPC/JALR pair, and ABI FPR result placements move
to prepared FPR result homes. Unsupported banks, widths, missing placements, and
non-F32/F64 FPR moves still fail closed.

Focused object-emission coverage now proves the representative shape:
`ft0 -> fa0`, call relocation, and `fa0 -> fs1`.

## Suggested Next

Rerun the `src/20030125-1.c` representative to confirm the object route moves
past the prepared FPR call boundary and classify the next unsupported edge, if
any.

## Watchouts

- Treat `src/20030125-1.c` as a representative only. The semantic class is
  prepared FPR cast/call ABI lowering, not testcase-specific math folding.
- Admission is intentionally identity-backed for FPR homes. Do not add raw
  `register_name` fallback parsing for FPR spellings in object emission.
- The new FPR direct-call support is intentionally limited to register storage,
  contiguous width 1, ABI placements for call-side `fa*` registers, and prepared
  value-home identities for non-ABI FPR homes.

## Proof

```sh
cmake --build build --target c4cll backend_riscv_object_emission_test && ctest --test-dir build -R '^backend_riscv_object_emission$' --output-on-failure > test_after.log
```

Result: passed. Proof log: `test_after.log`.
