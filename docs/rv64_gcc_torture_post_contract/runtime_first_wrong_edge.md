# RV64 Runtime First Wrong Edge Evidence

This note records first-wrong-edge evidence for one still-reproducing
representative per live RV64 runtime mode from Step 3.

Artifact root:
`build/agent_state/423_step3_first_wrong_edge/`

## Summary

| Case | Mode | clang qemu rc | c4c qemu rc | Classification | Earliest observable wrong edge |
| --- | --- | ---: | ---: | --- | --- |
| `src/pr38533.c` | abort | 0 | 134 | enough evidence | Inline asm tied-output result materialization uses an uninitialized register in emitted RV64 code even though the tied input is zero. |
| `src/20080506-2.c` | segfault | 0 | 139 | enough evidence | Prepared call-argument publication marks the second frame-slot address argument as missing before RV64 emission. |
| `src/pr81503.c` | exit 255 | 0 | 255 | enough evidence | Binary/comparison operand materialization is wrong in emitted RV64 code; prepared BIR keeps the intended value graph. |

All three `--dump-mir` files report `route owner: x86/debug` even with
`--target riscv64-linux-gnu`, so MIR is treated only as a route/debug signal.
The producer-facing evidence below comes from semantic BIR, prepared BIR, and
final linked disassembly/runtime behavior.

## `src/pr38533.c` - Abort

Runtime reproduction:

- clang: `build/agent_state/423_step3_first_wrong_edge/src_pr38533.c.clang-qemu.rc` = `0`
- c4c: `build/agent_state/423_step3_first_wrong_edge/src_pr38533.c.c4c-qemu.rc` = `134`

Primary artifacts:

- `build/agent_state/423_step3_first_wrong_edge/src_pr38533.c.bir.txt`
- `build/agent_state/423_step3_first_wrong_edge/src_pr38533.c.prepared-bir.txt`
- `build/agent_state/423_step3_first_wrong_edge/src_pr38533.c.mir.txt`
- `build/agent_state/423_step3_first_wrong_edge/src_pr38533.c.c4c-bin-objdump.txt`
- `build/agent_state/423_step3_first_wrong_edge/src_pr38533.c.clang-bin-objdump.txt`

Evidence:

- Semantic BIR lowers each empty inline asm as
  `bir.call i32 llvm.inline_asm(i32 0) [asm="", constraints="=r,0", sideeffect]`,
  then ORs the returned value into `e`.
- Prepared BIR publishes inline-asm carriers with `constraints="=r,0"`,
  `operand0[kind=register_output,...home=no]`, and
  `operand1[kind=tied_input,...home=yes]` for the repeated calls.
- The linked c4c disassembly for `foo` starts with `li t1,0`, stores `e = 0`,
  then immediately emits `mv t1,t0` and stores that value to `f`. No visible
  instruction first materializes the tied input zero into the output register.
- Since `foo()` should return zero and `main` aborts only when `foo()` is
  nonzero, the earliest observable wrong edge is the inline-asm tied-output
  result materialization, before the final abort call.

Classification: enough evidence. This looks producer-owned at the inline asm
carrier/result materialization boundary; do not infer a general RV64 branch or
abort lowering bug from this representative alone.

## `src/20080506-2.c` - Segfault

Runtime reproduction:

- clang: `build/agent_state/423_step3_first_wrong_edge/src_20080506-2.c.clang-qemu.rc` = `0`
- c4c: `build/agent_state/423_step3_first_wrong_edge/src_20080506-2.c.c4c-qemu.rc` = `139`

Primary artifacts:

- `build/agent_state/423_step3_first_wrong_edge/src_20080506-2.c.bir.txt`
- `build/agent_state/423_step3_first_wrong_edge/src_20080506-2.c.prepared-bir.txt`
- `build/agent_state/423_step3_first_wrong_edge/src_20080506-2.c.mir.txt`
- `build/agent_state/423_step3_first_wrong_edge/src_20080506-2.c.c4c-bin-objdump.txt`
- `build/agent_state/423_step3_first_wrong_edge/src_20080506-2.c.clang-bin-objdump.txt`

Evidence:

- Semantic BIR for `main` stores both `p1` and `p2` as addresses of local `a`,
  then calls `foo(ptr %lv.p1, ptr %lv.p2)`.
- Prepared BIR identifies arg 1 as a frame-slot address and records
  `missing_frame_slot_arg_publication=yes` for `function=main`,
  `call_inst=2`, `arg index=1`, with `arg.source_selection=frame_slot_address`.
- The same prepared dump later records a
  `call_arg_value_publication ... arg=1 ... payload=%lv.a`, so the producer
  knows the intended payload, but also flags the missing frame-slot argument
  publication needed by the lowering path.
- The linked c4c disassembly for `main` stores `sp` through `p1`, then stores
  `s1` through `p2` before the call. `s1` is not initialized as the address of
  `a` on that path, so `foo` receives a bad second pointer and can fault when
  it loads/stores through it.

Classification: enough evidence. The earliest wrong edge is the missing
frame-slot call-argument publication in prepared BIR, not the later segfault
or an RV64 memory instruction in isolation.

## `src/pr81503.c` - Exit 255

Runtime reproduction:

- clang: `build/agent_state/423_step3_first_wrong_edge/src_pr81503.c.clang-qemu.rc` = `0`
- c4c: `build/agent_state/423_step3_first_wrong_edge/src_pr81503.c.c4c-qemu.rc` = `255`

Primary artifacts:

- `build/agent_state/423_step3_first_wrong_edge/src_pr81503.c.bir.txt`
- `build/agent_state/423_step3_first_wrong_edge/src_pr81503.c.prepared-bir.txt`
- `build/agent_state/423_step3_first_wrong_edge/src_pr81503.c.mir.txt`
- `build/agent_state/423_step3_first_wrong_edge/src_pr81503.c.c4c-bin-objdump.txt`
- `build/agent_state/423_step3_first_wrong_edge/src_pr81503.c.clang-bin-objdump.txt`

Evidence:

- Semantic BIR computes the expected expression through `ne`, `xor`, `zext`,
  `mul`, `add`, and finally stores `%t18` to global `c`.
- Prepared BIR keeps the same value graph and records
  `store_source function=foo block=block_1 inst=8 source=%t18
  status=available ... source_producer=binary`.
- The linked c4c disassembly for `foo` loads globals `a` and `b`, but the
  first condition/expression sequence computes `xor s1,t3,t4` for `0 != 5`
  and then emits `mul s2,t3,t4`, multiplying the original `0` and `5` rather
  than the zero-extended `b` value and the `~(0 != 5)` value.
- Later, the c4c `main` compare also loads global `c` but compares using `bne
  t3,t4` while the loaded `c` is in `t0`, so the program returns `-1` and qemu
  reports rc `255`.

Classification: enough evidence. The earliest observable wrong edge is binary
operand/value materialization between prepared BIR and emitted RV64 code. The
prepared BIR has producer authority for the store source, so Step 4 should
group this with operand publication or lowering consumption rather than
expectation/accounting changes.

## Suggested Step 4 Grouping

Group the live runtime representatives by first wrong edge:

- Inline asm tied-output/result materialization: `src/pr38533.c`.
- Frame-slot call-argument publication/materialization: `src/20080506-2.c`.
- Binary operand/value materialization after prepared BIR: `src/pr81503.c`.

The evidence is sufficient for Step 4 grouping. No expectation, allowlist,
unsupported-marker, or runtime comparison changes were made.
