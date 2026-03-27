# RV64 Backend Port Plan

## Relationship To Roadmap

Umbrella source: `ideas/open/__backend_port_plan.md`

Depends on:

- `ideas/open/01_backend_lir_adapter_plan.md`
- shared lessons from earlier target ports

## Goal

Port the rv64 backend from ref into C++ once the common backend path and earlier targets have removed most structural unknowns.

## Why This Is Later

- lower immediate product value than AArch64 and x86-64
- more likely to rely on QEMU-driven validation
- benefits from shared backend patterns being proven first

## Scope

- LP64D call and return lowering
- integer ALU and memory operations
- control-flow emission
- global addressing sequences
- float basics
- register allocation and cleanup work needed for useful testsuite progress

## Validation

- QEMU user-mode execution works for trivial programs
- arithmetic and control-flow tests pass
- c-testsuite subset runs for supported features

## Good First Patch

Port the minimal return/arithmetic/control-flow slice from the ref rv64 backend and validate through the existing external toolchain path plus QEMU execution.
