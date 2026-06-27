Status: Active
Source Idea Path: ideas/open/399_rv64_object_route_global_data_and_symbol_memory.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Classify Current Global Data Rejections

# Current Packet

## Just Finished

Step 1 refreshed the current 399 global-data/symbol-memory bucket. The exact
delegated proof completed successfully and produced artifacts under
`build/agent_state/399_step1_global_refresh/`.

Representative status:

- `20010924-1`: `c4cll_status=0`, `prepared_status=0`. Current route passes.
  Prepared dump contains global loads from string/aggregate globals, including
  `@a1` offset `0` width `1`, `@a1` offset `8` width `8`, `.str0` offsets
  `0..2` width `1`, and `@a2`/`@a3`/`@a4` byte loads. Prepared global memory
  facts are present as `base=global_symbol`, `base_plus_offset=yes`,
  `range_verdict=proven_in_bounds`.
- `20001121-1`: `c4cll_status=0`, `prepared_status=0`. Current route passes.
  The representative is now a supported `double` global load:
  `@d = global double zeroinitializer, align 8`, `bir.load_global double @d`,
  prepared access `symbol=d offset=0 size=8 align=8`.
- `20031211-1`: `c4cll_status=0`, `prepared_status=0`. Current route passes.
  The global memory surface is `@x = global i32 zeroinitializer, align 4`,
  `bir.store_global @x, i32 48879`, `bir.load_global i32 @x`, with prepared
  accesses `symbol=x offset=0 size=4 align=4`.
- `pr57568`: `c4cll_status=0`, `prepared_status=0`. Current route passes.
  Globals include `@a = global [6 x [9 x i32]] ...`, `@b = global i32 1`,
  and `@c = global ptr getelementptr ... @a ... 3, 5`. Prepared facts include
  `@b` load `size=4 align=4`, `@c` pointer loads `size=8 align=8`, and `@a`
  load/store at offset `128` with `size=4 align=4`, all as
  `base=global_symbol base_plus_offset=yes`.

Nearby stale-log seeds:

- `20020404-1`: `c4cll_status=0`, `prepared_status=1`. First diagnostic is
  semantic `lir_to_bir` failure before prepared handoff:
  function `main` failed in `direct-call semantic family`. It has globals and
  strings, but this proof does not reach a prepared global-data/object lowering
  rejection.
- `20040709-2`: `c4cll_status=0`, `prepared_status=1`. First diagnostic is
  bootstrap global admission: only scalar integer/pointer globals, linear
  integer-array globals, and aggregate-backed globals with honest byte-address
  semantics are currently supported. The visible shape is a large set of
  packed aggregate globals such as `%struct.A <{ i16 0 }>` through `%struct.Z`,
  including fp128-containing globals. This is a producer/global-admission
  missing-fact route, not an RV64 object-emission inference route.
- `20041218-1`: `c4cll_status=0`, `prepared_status=1`. First diagnostic is
  semantic `lir_to_bir` failure in function `baz`, runtime/intrinsic family
  `memset runtime family`, before prepared handoff. The visible global is a
  nested static aggregate, but the current blocker is memset admission.
- `20050607-1`: `c4cll_status=0`, `prepared_status=1`. First diagnostic is
  semantic `lir_to_bir` failure in function `main`, `alloca local-memory
  semantic family`, for vector local memory (`alloca <2 x i32>` and
  `store <2 x i32> zeroinitializer`), not a global-data object route.

Classification: no seed in this refreshed set currently fails with
`unsupported_global_data`, missing prepared global memory facts, or unsupported
direct global-symbol base-plus-offset object lowering. The source idea
representatives are green through this proof. The only remaining stale-log
evidence is outside 399 object lowering: producer/frontend admission for
direct-call, packed/fp128 aggregate globals, memset, or vector local alloca.

## Suggested Next

Ask the plan owner to review whether 399 is closure-ready or should be
retired/replaced. No RV64 object-emission Step 2 implementation route is
justified by this Step 1 proof. If the lifecycle stays open, the first
defensible next route is a separate producer/global-admission split for
`20040709-2` packed aggregate/fp128 global initializer facts, not target-side
global-data inference.

## Watchouts

- Do not reconstruct global initializer bytes, symbol identity, string data, or
  memory-access facts in RV64 object emission.
- Route missing prepared global facts to lifecycle review instead of absorbing
  them into target lowering.
- Prove linked/qemu behavior for newly emitted runnable global-data cases.
- Do not use filename-specific branches, expectation rewrites, unsupported
  downgrades, or allowlist filtering as progress.
- The original 399 representatives now pass the exact proof surface; avoid
  implementing an object-emission patch without a fresh owned failing
  representative.

## Proof

Exact command run:

```sh
cmake --build --preset default && mkdir -p build/agent_state/399_step1_global_refresh && for case in 20010924-1 20001121-1 20031211-1 pr57568 20020404-1 20040709-2 20041218-1 20050607-1; do src="tests/c/external/gcc_torture/src/${case}.c"; (build/c4cll --target riscv64-linux-gnu "$src" > "build/agent_state/399_step1_global_refresh/${case}.out" 2> "build/agent_state/399_step1_global_refresh/${case}.err"; printf 'c4cll_status=%s\n' "$?" > "build/agent_state/399_step1_global_refresh/${case}.status"); (build/c4cll --dump-prepared-bir --target riscv64-linux-gnu "$src" > "build/agent_state/399_step1_global_refresh/${case}.prepared.txt" 2> "build/agent_state/399_step1_global_refresh/${case}.prepared.err"; printf 'prepared_status=%s\n' "$?" > "build/agent_state/399_step1_global_refresh/${case}.prepared.status"); done && for case in 20010924-1 20001121-1 20031211-1 pr57568 20020404-1 20040709-2 20041218-1 20050607-1; do printf '== %s ==\n' "$case"; cat "build/agent_state/399_step1_global_refresh/${case}.status" "build/agent_state/399_step1_global_refresh/${case}.prepared.status"; rg -n 'unsupported_global_data|immediate scalar and immediate linear global storage|prepared global memory accesses|prepared global memory facts|direct global-symbol base-plus-offset|global|symbol|string|initializer|storage|memory access|base|offset|reloc|\.data|\.rodata|unsupported|error|fatal' "build/agent_state/399_step1_global_refresh/${case}.out" "build/agent_state/399_step1_global_refresh/${case}.err" "build/agent_state/399_step1_global_refresh/${case}.prepared.txt" "build/agent_state/399_step1_global_refresh/${case}.prepared.err" || true; done && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log
```

Result:

- Command completed successfully.
- `cmake --build --preset default`: no work to do.
- Per-seed artifacts are in `build/agent_state/399_step1_global_refresh/`.
- `20010924-1`, `20001121-1`, `20031211-1`, and `pr57568` all have
  `c4cll_status=0` and `prepared_status=0`.
- `20020404-1`, `20040709-2`, `20041218-1`, and `20050607-1` have
  `c4cll_status=0` and `prepared_status=1` with the producer/frontend
  diagnostics recorded above.
- `test_after.log`: backend subset passed, `100% tests passed, 0 tests failed
  out of 326`.
