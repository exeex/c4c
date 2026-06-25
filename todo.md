Status: Active
Source Idea Path: ideas/open/358_rv64_object_route_abi_width_edges.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Route Simple Call and FPR ABI Edges

# Current Packet

## Just Finished

Classified the remaining Plan Step 4 `unsupported_floating_cast` boundary for
`src/20030125-1.c` after prepared RV64 FPR `fpext` and `fptrunc` lowering.

The earliest failing object-route site is the first instruction in `@t`:

```text
function: @t
block: entry
instruction index: 0
instruction: %t0 = bir.fpext float %p.a to double
cast opcode/types: FPExt, F32 -> F64
operand home: %p.a value_id=0 kind=register reg=s0
result home: %t0 value_id=1 kind=register reg=d13, placement=fpr:caller_saved#0/w1
```

The cast type pair is already supported by `fragment_for_prepared_floating_cast`.
The remaining rejection is a prepared-home shape: `fpr_register_number_for_home`
requires an RV64 `PreparedTargetRegisterIdentity`, but the source parameter home
is published only as the ABI spelling `s0`, while the destination temporary has
the target FPR placement/identity shape used by supported casts.

The same source-parameter shape appears in `@q` and `@q1` at their entry
instruction index 0:

```text
@q:  %t0 = bir.fpext float %p.a to double, %p.a value_id=4 reg=s0 -> %t0 value_id=5 reg=d13
@q1: %t0 = bir.fpext float %p.a to double, %p.a value_id=8 reg=s0 -> %t0 value_id=9 reg=d13
```

## Suggested Next

Implement a semantic RV64 FPR-parameter cast-home bridge in
`src/backend/mir/riscv/codegen/object_emission.cpp` with focused coverage in
`tests/backend/mir/backend_riscv_object_emission_test.cpp`, then update
`todo.md`.

The narrow implementation should keep the supported cast type pairs unchanged
and teach the object route how to resolve a prepared FPR parameter home such as
`reg=s0` for `float` source operands when lowering `fcvt.d.s`. A good focused
proof command is:

```sh
cmake --build build --target backend_riscv_object_emission_test && ctest --test-dir build -R '^backend_riscv_object_emission$' --output-on-failure > test_after.log
```

## Watchouts

- Treat `src/20030125-1.c` as a representative only. The semantic class is
  prepared FPR cast/call ABI lowering, not testcase-specific math folding.
- The remaining cast is target-consumable RV64 FPR work; it should not remain a
  generic floating-cast diagnostic once the FPR parameter register contract is
  represented or resolved.
- Do not broaden the cast type matrix. The representative still only needs
  `FPExt F32 -> F64` and already-covered `FPTrunc F64 -> F32`.
- Preserve the object route's reliance on prepared register facts. If the fix
  maps ABI spellings like `s0`, keep it constrained to FPR parameter/float
  homes rather than falling back to register-name parsing for arbitrary homes.
- Floating calls, libm behavior, and stack-slot FPR materialization remain
  outside this classification packet.

## Proof

Delegated proof:

```sh
git diff --check -- todo.md
```

Result: passed.

Proof logs:
- no new root-level log expected for this todo-only diff-check proof
