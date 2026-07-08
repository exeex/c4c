Status: Active
Source Idea Path: ideas/open/608_prepared_global_data_authority.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Prove residual handoff and close readiness

# Current Packet

## Just Finished

- Step 3 proved the narrowed residual handoff after the byte-storage
  layout-authority repair. Fresh prepared dumps for `src/pr36034-1.c` and
  `src/pr91137.c` are under
  `build/agent_state/608_step3_residual_handoff/`.
- `src/pr36034-1.c` now has complete prepared global-memory rows for the
  representative `x`/`tmp` accesses: direct global-symbol base, byte-storage
  aggregate layout authority, concrete 8-byte width/alignment, in-bounds range,
  and supported direct base-plus-offset addressing. The first concrete remaining
  owner is downstream RV64 prepared-global value-location consumption: the
  early F64 global loads move from register destinations to prepared FPR
  frame-slot homes, while `fragment_for_prepared_load_global(...)` only accepts
  an FPR register destination for floating global loads.
- `src/pr91137.c` also has complete prepared global-memory rows for the
  representative `c`/`d` aggregate accesses and scalar rows for `e`/`a`:
  direct global-symbol base, byte-storage/scalar layout authority, concrete
  4-/8-byte width and alignment, in-bounds range, and supported direct
  base-plus-offset addressing. Its remaining stop is likewise downstream of
  prepared fact publication, in RV64 prepared-global consumer/value-location
  handling for the register/frame-slot load-store sequence, not in producer
  authority.
- Direct RV64 object probes still fail for both residual files with the coarse
  `unsupported_global_data: RV64 object route requires supported prepared
  global memory facts` diagnostic. That string is not the owner proof; the
  owner proof is the current prepared rows plus the RV64 consumer predicates
  checked above.

## Suggested Next

- Supervisor/plan-owner close review for idea `608`. The narrowed
  prepared/global authority route is close-ready from this packet's evidence:
  the remaining work is a separate RV64 prepared-global consumer/value-location
  initiative if the project wants `src/pr36034-1.c` and `src/pr91137.c` to reach
  object emission.

## Watchouts

- This packet did not absorb selected object-data, direct global-symbol
  base-plus-offset, prepared move-bundle, ABI, RV64 emission/lowering,
  relocation-record, unsupported-policy, expectation, allowlist, timeout, or
  accounting work.
- The object probes are still red, so do not claim target object support for
  the two files as part of idea `608`. The close-readiness recommendation is
  only for the prepared/global authority route.
- A follow-up RV64 consumer packet should not change test expectations or
  unsupported markers. For `pr36034-1.c`, start from floating global loads whose
  prepared destinations are FPR frame-slot homes. For `pr91137.c`, start from
  the prepared global I32 load/store sequence that alternates register and
  frame-slot homes.

## Proof

- Focused movement probes:
  `./build/c4cll --dump-prepared-bir --target riscv64-linux-gnu
  tests/c/external/gcc_torture/src/pr36034-1.c` and
  `./build/c4cll --dump-prepared-bir --target riscv64-linux-gnu
  tests/c/external/gcc_torture/src/pr91137.c` wrote current prepared dumps
  under `build/agent_state/608_step3_residual_handoff/`.
- Direct object probes:
  `./build/c4cll --codegen obj --target riscv64-linux-gnu
  tests/c/external/gcc_torture/src/pr36034-1.c -o
  build/agent_state/608_step3_residual_handoff/pr36034-1.o` and the matching
  `src/pr91137.c` command both still fail with the coarse
  `unsupported_global_data` diagnostic; stderr/stdout logs are in the same
  directory.
- Delegated proof run:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'
```

- Result: passed. Proof log path: `test_after.log`.
