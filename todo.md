Status: Active
Source Idea Path: ideas/open/358_rv64_object_route_abi_width_edges.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Route Simple Call and FPR ABI Edges

# Current Packet

## Just Finished

Completed Plan Step 4's classification audit for `src/20030125-1.c`. The
generic `unsupported_instruction_fragment` comes from the first prepared BIR
instruction in `@t`, block `entry`, instruction index `0`:

```text
%t0 = bir.fpext float %p.a to double
```

`fragment_for_prepared_instruction()` reaches the `CastInst` path, but
`fragment_for_prepared_cast()` only handles integer casts because it requires
`rv64_integer_type_bits()` for both operands. The current diagnostic helper has
no `CastInst` branch for floating casts, so this falls through to the generic
unsupported-instruction diagnostic. Prepared facts are present and target
consumable: `%p.a`, `%t0`, `%t1`, and `%t2` all have FPR register homes; the
same-module call to `sin` has FPR argument/result ABI plans; and the
before-instruction, before-call, after-call, and before-return move bundles are
published.

## Suggested Next

Implement the first Step 4 FPR object-route packet in
`src/backend/mir/riscv/codegen/object_emission.cpp` with focused coverage in
`tests/backend/mir/backend_riscv_object_emission_test.cpp`. The smallest
coherent slice is to classify or lower prepared FPR casts, starting with
`bir.fpext float -> double` in FPR register homes and preserving a structured
diagnostic for still-unsupported floating casts. If lowering `fpext` succeeds,
expect the same representative to expose the queued `%t2 = bir.fptrunc double
%t1 to float` boundary before the call/return sequence is fully executable.

Suggested focused proof:

```sh
cmake --build build --target backend_riscv_object_emission_test && ctest --test-dir build -R '^backend_riscv_object_emission$' --output-on-failure > test_after.log
```

Suggested representative proof after the focused slice:

```sh
tmp=$(mktemp); printf 'src/20030125-1.c\n' > "$tmp"; ALLOWLIST="$tmp" CASE_TIMEOUT_SEC=20 STOP_ON_FAILURE=1 scripts/check_progress_rv64_gcc_c_torture_backend.sh > test_after.log; rc=$?; rm -f "$tmp"; exit $rc
```

## Watchouts

- Treat `src/20030125-1.c` as a representative only. The semantic class is
  prepared FPR cast/call ABI lowering, not testcase-specific math folding.
- This is target-consumable RV64 FPR work, not an upstream prepared-semantics
  gap: prepared FPR homes, call plans, and move bundles are already published.
- The first failing instruction is `bir.fpext`; `bir.fptrunc` is the likely
  next same-family boundary in `@t`/`@q` once `fpext` is handled.
- The local-memory address predicate remains intentionally narrow: default
  address space, non-volatile, frame-slot base, published frame-slot id,
  base-plus-offset eligibility, matching access size, compatible alignment,
  non-negative offset, and signed 12-bit immediate reach.
- Narrow local loads use the same stack-load helper and therefore encode signed
  `lb`/`lh`; add an explicit load fixture before changing signedness behavior.
- `src/20001203-1.c` is now green in the representative harness; do not keep
  expanding this packet around that testcase.
- `src/20030125-1.c` remains out of scope for this packet and should still be
  classified separately before any FPR or floating-operation implementation.

## Proof

Delegated proof:

```sh
git diff --check -- todo.md
```

Result: passed.

Evidence inspected:

```sh
./build/c4cll --dump-bir --target riscv64-linux-gnu tests/c/external/gcc_torture/src/20030125-1.c
./build/c4cll --dump-prepared-bir --target riscv64-linux-gnu --mir-focus-function t tests/c/external/gcc_torture/src/20030125-1.c
```

Existing representative log: `build/rv64_gcc_c_torture_backend/src_20030125-1.c/case.log`.
