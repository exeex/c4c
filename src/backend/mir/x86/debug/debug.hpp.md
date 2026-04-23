# `debug/debug.hpp` contract

## Legacy Evidence

The legacy route-debug surface was mostly implicit through larger headers and
debug implementation files.

## Design Contract

This header is the canonical public debug seam for prepared x86 route
observation.

Owned inputs:

- `PreparedBirModule`
- optional function focus
- optional block focus

Owned outputs:

- summary text
- trace text

Invariants:

- debug output is observational
- the public debug API stays small even if internal route reporting grows

Must not own:

- lowering or emission
- module entry routing
