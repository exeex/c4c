Status: Active
Source Idea Path: ideas/open/399_rv64_object_route_global_data_and_symbol_memory.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Classify Current Global Data Rejections

# Current Packet

## Just Finished

Lifecycle activation created the active runbook from
`ideas/open/399_rv64_object_route_global_data_and_symbol_memory.md`.

## Suggested Next

Execute Step 1: refresh the current RV64 gcc_torture global-data and
symbol-memory bucket for `20010924-1.c`, `20001121-1.c`, `20031211-1.c`,
`pr57568.c`, and supervisor-selected nearby same-bucket representatives.
Classify the first concrete global storage, global memory, direct-symbol
addressing, relocation, or producer missing-fact family before implementation.

## Watchouts

- Do not reconstruct global initializer bytes, symbol identity, string data, or
  memory-access facts in RV64 object emission.
- Route missing prepared global facts to lifecycle review instead of absorbing
  them into target lowering.
- Prove linked/qemu behavior for newly emitted runnable global-data cases.
- Do not use filename-specific branches, expectation rewrites, unsupported
  downgrades, or allowlist filtering as progress.

## Proof

No execution proof yet. This is lifecycle activation only.
