Status: Active
Source Idea Path: ideas/open/358_rv64_object_route_abi_width_edges.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Route Simple Call and FPR ABI Edges

# Current Packet

## Just Finished

Completed Plan Step 4 object-emission support for simple prepared FPR
`before_return` register-to-register ABI moves.

The RV64 prepared move-bundle emitter now accepts FPR destinations only through
prepared target identities or ABI register placements, resolves the source value
type from BIR value metadata, and emits `fmv.s`/`fmv.d` for F32/F64
register-to-register moves such as `%t2` home `ft0` to return ABI `fa0`. The
return terminator now recognizes when an F32/F64 return value already has a
prepared FPR before-return ABI move in the same block and emits only the
epilogue/`ret`, avoiding a second GPR `a0` lowering attempt.

Focused object-emission tests cover `ft0 -> fa0` before-return F32 and F64
`fmv.*` encodings, and keep cycle-temp, multi-width, stack-destination, and
non-F32/F64 FPR move-bundle shapes fail-closed with the existing unsupported
move-bundle diagnostic.

## Suggested Next

Rerun the representative `src/20030125-1.c` RV64 backend progress proof to find
the next object-route boundary after the prepared FPR return ABI move.

## Watchouts

- Treat `src/20030125-1.c` as a representative only. The semantic class is
  prepared FPR cast/call ABI lowering, not testcase-specific math folding.
- FPR move-bundle admission remains identity/placement-backed; raw FPR
  `register_name` parsing is still intentionally unsupported in object
  emission.
- The new terminator skip depends on the prepared before-return FPR ABI move
  lookup for the same block and source value. If the representative advances,
  inspect the next `prepared.dump` boundary rather than weakening this guard.

## Proof

```sh
cmake --build build --target c4cll backend_riscv_object_emission_test && ctest --test-dir build -R '^backend_riscv_object_emission$' --output-on-failure > test_after.log
```

Result: passed. Proof log: `test_after.log`.
