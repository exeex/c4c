Status: Active
Source Idea Path: ideas/open/621_rv64_prepared_global_value_location_consumer.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Validate Breadth And Guards

# Current Packet

## Just Finished

Step 4 validated the prepared-global consumer slice with fresh focused probes
after the Step 3 implementation.

Focused probe results:

- `src/pr36034-1.c`: `--dump-prepared-bir` succeeded and `--codegen obj`
  progressed past the old prepared-global value-location stop. Current stop is
  downstream move-bundle ownership:
  `unsupported_move_bundle_target_shape` with stack-to-stack double moves and
  `fragment_status=generic_move_bundle_materialization_failed`.
- `src/pr91137.c`: `--dump-prepared-bir` succeeded, but `--codegen obj` still
  stops at `unsupported_global_data: RV64 object route requires supported
  prepared global memory facts`. The prepared dump shows many direct-global
  select-chain `store_global_publication` sources, so this row is not proven
  fixed by the current value-location consumer slice and should stay classified
  to prepared/global producer or supported-access authority follow-up unless a
  narrower refreshed probe identifies a complete prepared access.
- `src/ieee/20001122-1.c`: guard row remains outside this route. It still
  fails closed at `unsupported_global_data: RV64 object route supports only 1-,
  2-, 4-, and 8-byte prepared global memory accesses`, so the width-policy
  guard was not absorbed by the consumer slice.
- `src/991030-1.c`: guard row remains outside this route because `--codegen
  obj` succeeds and emits an object file.

Close-readiness evidence: the slice has one representative row
(`src/pr36034-1.c`) progressing to a downstream owner and preserves both guard
classifications, but `src/pr91137.c` still needs supervisor classification
before idea closure because the old `requires supported prepared global memory
facts` stop remains for that row.

## Suggested Next

Supervisor should either route a narrow follow-up classification packet for
`src/pr91137.c` prepared-access completeness or ask plan-owner/reviewer whether
the current evidence is sufficient to close/split idea 621 with `src/pr91137.c`
assigned to a downstream producer/access-authority owner.

## Watchouts

- Do not treat `src/pr91137.c` as fixed by the consumer slice yet; its current
  object-route stop is still `requires supported prepared global memory facts`.
- Do not widen Step 3 into direct-global select-chain or producer-authority
  reconstruction inside RV64.
- Keep `src/ieee/20001122-1.c` guard-only unless a separate width-policy owner
  resolves the unsupported access size.
- Keep `src/991030-1.c` out of this route because it already emits an object
  successfully.

## Proof

Focused probes run:

`./build/c4cll -I tests/c/external/gcc_torture --target riscv64-linux-gnu --dump-prepared-bir <row>`

`./build/c4cll -I tests/c/external/gcc_torture --target riscv64-linux-gnu --codegen obj <row> -o /tmp/c4c_621_step4/<row>.o`

Rows probed: `src/pr36034-1.c`, `src/pr91137.c`,
`src/ieee/20001122-1.c`, and `src/991030-1.c`.

Delegated proof command run:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' | tee test_after.log`

Result: passed. The build was up to date; CTest reported
`100% tests passed, 0 tests failed out of 347`. Proof log: `test_after.log`.
