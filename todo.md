Status: Active
Source Idea Path: ideas/open/409_prepared_packed_fp128_global_initializer_admission.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Classify Packed And FP128 Global Admission

# Current Packet

## Just Finished

Step 1 classified the current `20040709-2.c` packed aggregate / `fp128`
global admission blocker. The exact delegated proof completed successfully and
produced artifacts under
`build/agent_state/409_step1_packed_fp128_global/`.

Status and diagnostic:

- `c4cll_status=0`.
- `prepared_status=1`.
- First diagnostic:
  `error: backend BIR dump requires semantic lir_to_bir lowering before the prepared handoff: bootstrap lir_to_bir only supports scalar integer/pointer globals, linear integer-array globals, and aggregate-backed globals with honest byte-address semantics right now`.
- This fails before prepared handoff. No prepared global-memory facts reach
  RV64 object emission, so this packet remains producer/global admission only.

Rejected visible global forms:

- Packed aggregate type declarations `%struct.A` through `%struct.Z` all use
  LLVM packed struct spelling `<{ ... }>` rather than ordinary `{ ... }`.
- Integer-only packed aggregate globals include:
  `%struct.A = type <{ i16 }>` with `@sA = global %struct.A <{ i16 0 }>`,
  `%struct.B = type <{ i16, i32 }>` with `@sB = global %struct.B <{ i16 0, i32 zeroinitializer }>`,
  `%struct.C = type <{ i32, i16 }>` with `@sC = global %struct.C <{ i32 zeroinitializer, i16 0 }>`,
  and wider mixed integer variants such as `%struct.G = type <{ i16, i64 }>`
  with `@sG = global %struct.G <{ i16 0, i64 zeroinitializer }>`.
- Small integer-only packed globals also include `%struct.J`/`%struct.T`/`%struct.V`
  as `<{ i16, i16 }>` and `%struct.M` as `<{ i32, i16, i16, i16 }>`.
- `fp128` packed aggregate globals include:
  `%struct.W = type <{ fp128, i32 }>` with `@sW = global %struct.W <{ fp128 zeroinitializer, i32 0 }>`,
  `%struct.X = type <{ i32, fp128 }>` with `@sX = global %struct.X <{ i32 0, fp128 zeroinitializer }>`,
  `%struct.Y = type <{ i32, fp128 }>` with `@sY = global %struct.Y <{ i32 0, fp128 zeroinitializer }>`,
  and `%struct.Z = type <{ fp128, i32 }>` with `@sZ = global %struct.Z <{ fp128 zeroinitializer, i32 0 }>`.
- The lowered LLVM-like output also later contains `load fp128`, `store fp128`,
  `fpext ... to fp128`, and `fcmp une fp128`, but the current blocker is
  global admission before prepared BIR.

Producer area:

- `src/backend/bir/lir_to_bir/module.cpp` emits the current diagnostic while
  lowering module globals. The failing gate is `lower_minimal_global(...)`
  returning `std::nullopt` inside `lower_module(...)` before the global can be
  inserted into `global_types`.
- `src/backend/bir/lir_to_bir/globals.cpp` owns the support predicates and
  lowered `bir::Global` construction in `lower_minimal_global_impl(...)`.
  It first admits scalar globals, then linear integer arrays, then aggregate
  globals if layout lookup plus aggregate initializer lowering succeeds.
- `src/backend/bir/lir_to_bir/global_initializers.cpp` owns the aggregate
  initializer walker. It already has byte-oriented handling for aggregate
  `F128` zero/nonzero leaves, so Step 2 should verify whether the failing
  boundary is packed structured layout lookup/parity/admission before widening
  initializer byte handling.

Classification: this is a producer/global admission gap, not an RV64 object
emission issue. The first supportable Step 2 route is to admit packed
integer-only aggregate globals with honest byte-address semantics through
`lower_minimal_global_impl(...)`, preserving byte layout and linear addressing.
Once that narrow packed-integer path is proved, handle the `fp128` mixed packed
aggregate payloads as the next same-surface extension only if the existing
aggregate initializer byte machinery proves sufficient. Do not reconstruct
packed layout or `fp128` bytes in RV64 object emission.

## Suggested Next

Implement Step 2 as a producer-side packed aggregate global admission slice in
`src/backend/bir/lir_to_bir/globals.cpp` and adjacent focused tests. Start
with integer-only packed aggregate globals that can be represented as
byte-addressable aggregate-backed `bir::Global` storage. Keep `fp128` packed
aggregate support as a same-surface follow-up inside the producer only if the
layout/initializer proof shows it is not a separate capability.

## Watchouts

- Do not reconstruct packed aggregate layout or `fp128` initializer bytes in
  RV64 object emission.
- Keep producer/global admission separate from target global-data lowering.
- Route unrelated direct-call, memset, or vector local alloca failures to their
  own owners.
- Do not use filename-specific handling, expectation rewrites, unsupported
  downgrades, or allowlist filtering as progress.
- Current proof does not reach prepared handoff, so no RV64 object-emission
  inference or consumer patch is justified.

## Proof

Exact command run:

```sh
cmake --build --preset default && mkdir -p build/agent_state/409_step1_packed_fp128_global && (build/c4cll --target riscv64-linux-gnu tests/c/external/gcc_torture/src/20040709-2.c > build/agent_state/409_step1_packed_fp128_global/20040709-2.out 2> build/agent_state/409_step1_packed_fp128_global/20040709-2.err; printf 'c4cll_status=%s\n' "$?" > build/agent_state/409_step1_packed_fp128_global/20040709-2.status) && (build/c4cll --dump-prepared-bir --target riscv64-linux-gnu tests/c/external/gcc_torture/src/20040709-2.c > build/agent_state/409_step1_packed_fp128_global/20040709-2.prepared.txt 2> build/agent_state/409_step1_packed_fp128_global/20040709-2.prepared.err; printf 'prepared_status=%s\n' "$?" > build/agent_state/409_step1_packed_fp128_global/20040709-2.prepared.status) && cat build/agent_state/409_step1_packed_fp128_global/20040709-2.status build/agent_state/409_step1_packed_fp128_global/20040709-2.prepared.status && rg -n 'bootstrap global admission|only scalar integer/pointer globals|linear integer-array globals|aggregate-backed globals|honest byte-address semantics|unsupported|error|fatal|global|fp128|packed|<\{|%struct\.|zeroinitializer|initializer|align|type \{|type <\{|prepared|handoff' build/agent_state/409_step1_packed_fp128_global/20040709-2.out build/agent_state/409_step1_packed_fp128_global/20040709-2.err build/agent_state/409_step1_packed_fp128_global/20040709-2.prepared.txt build/agent_state/409_step1_packed_fp128_global/20040709-2.prepared.err || true && rg -n 'bootstrap global admission|only scalar integer/pointer globals|linear integer-array globals|aggregate-backed globals|honest byte-address|global admission|initializer|fp128|F128|packed|is_packed|llvm_type_ref|LirGlobal|GlobalInfo|global_types|global initializer|global storage' src/backend src/codegen tests/backend/bir | sed -n '1,260p' && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log
```

Result:

- Command completed successfully.
- `cmake --build --preset default`: no work to do.
- `c4cll_status=0`.
- `prepared_status=1` with the pre-handoff global admission diagnostic above.
- Source/code search located the producer area in
  `src/backend/bir/lir_to_bir/module.cpp`,
  `src/backend/bir/lir_to_bir/globals.cpp`, and
  `src/backend/bir/lir_to_bir/global_initializers.cpp`.
- `test_after.log`: backend subset passed, `100% tests passed, 0 tests failed
  out of 326`.
