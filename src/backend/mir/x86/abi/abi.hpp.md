# `abi/abi.hpp` contract

## Legacy Evidence

The old backend relied on ad hoc naming and target helpers from larger helper
surfaces. This header exists to stop those concerns from floating back into
random x86 files.

## Design Contract

This header owns the tiny reusable target-resolution and naming-policy helpers
needed across x86 codegen.

Owned outputs:

- resolved target triple/profile
- x86-family target gating
- public/private symbol spelling helpers
- narrow i32 register spellings

Must not own:

- full calling convention policy
- object-format orchestration
- function/module lowering logic
