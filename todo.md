Status: Active
Source Idea Path: ideas/open/380_rv64_object_route_short_circuit_call_argument_reload.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Short-Circuit Call Argument Loss

# Current Packet

## Just Finished

Step 1 audit completed for `src/20000112-1.c`: the first semantic failure is
the second `strchr` in `special_format`, reached from the first short-circuit
RHS after `strchr(fmt, '*')` returns NULL.  Prepared BIR keeps the semantic
argument as `%p.fmt` / value id 0, records later `strchr` arg0 as that value,
and also records preservation of `%p.fmt` into callee-saved `s1`; generated
object code instead emits the second call with arg0 still in ABI register `a0`,
which now contains the previous call result NULL.

## Suggested Next

Implement a general call-argument reload/preservation repair for prepared
call arguments whose semantic source value has a preserved/value home different
from the live ABI argument register after an earlier call result can clobber
that ABI register.  The first concrete shape to handle is `special_format`
callsite `block_index=1 inst_index=0`: reload/copy `%p.fmt` value id 0 from its
preserved home `s1` (or otherwise keep it live in a valid home) into arg0 `a0`
before calling `strchr(..., 'V')`.

## Watchouts

Do not key behavior on testcase name, `special_format`, a specific `strchr`
call, exact C expression spelling, block labels, value ids, instruction
indexes, physical registers, object addresses, or log text. The semantic
invariant is: when a call arg is sourced from a value that has been preserved or
moved out of a call-clobbered ABI/result register, call setup must source from
the current value home rather than assuming the old ABI arg register still
contains the incoming parameter.

Required prepared facts observed:
- `prepared.summary @special_format` has saved `s1` and `s2`, no frame homes,
  and four `strchr` callsites.
- `%p.fmt` storage is initially `register:a0`, value id 0.
- Later `strchr` callsites at `block_index=1`, `5`, and `9` preserve
  `%p.fmt#0` via `callee_saved_register:s1`.
- Prepared call plans still describe arg0 for those later callsites as
  `source_value_id=0 source_reg=a0 -> dest_reg=a0`, while also listing the
  preserve home in `s1`. That is the semantic reload gap.

Runtime/object evidence:
- c4c `special_format` does not copy/reload the incoming `fmt` before the
  second call. It executes `li a1,86; jalr strchr@plt` with `a0` unchanged from
  the first call result.
- QEMU trace shows entry to `special_format` with `a0=0x5555555568b0`, first
  `strchr('*')` returning NULL into `a0=0`, then the second `strchr('V')`
  entered with `x10/a0=0`, leading to SIGSEGV `si_addr=NULL`.
- clang stores the incoming `a0` to a frame slot and reloads it before every
  `strchr`, which is the expected semantic behavior but not necessarily the
  required implementation strategy.

## Proof

Audit/classification only; no build or root proof log was run or overwritten.
Artifacts and commands used:
- `sed -n '1,220p' build/rv64_gcc_c_torture_backend/src_20000112-1.c/case.log`
- `sed -n '1,220p' build/agent_state/379_step4_20000112.classification.txt`
- `rg -n 'strchr|call|arg|a0|value|home|20000112' build/agent_state/378_step1_20000112.prepared_bir.log build/agent_state/378_step1_20000112.semantic_bir.log build/agent_state/378_step1_20000112_probe_results.log`
- `sed -n '216,238p' build/agent_state/378_step1_20000112.prepared_bir.log`
- `sed -n '545,620p' build/agent_state/378_step1_20000112.prepared_bir.log`
- `sed -n '117,173p' build/agent_state/379_step4_20000112.c4c_bin_objdump.txt`
- `sed -n '138,173p' build/agent_state/379_step4_20000112.clang_bin_objdump.txt`
- `sed -n '42353,42750p' build/agent_state/379_step4_20000112.c4c_qemu_trace.log`
- `rg -n 'SIG|a0|strchr|main|call' build/agent_state/379_step4_20000112.c4c_qemu_L_strace.err build/agent_state/379_step4_20000112.c4c_qemu_trace.log build/agent_state/379_step4_20000112.clang_qemu_L_strace.err`
