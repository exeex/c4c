# `prepared/prepared.hpp` contract

## Design Contract

This header is the single public header for `x86/prepared/`.

Owned outputs:

- bounded prepared query context
- fast-path classification result
- tiny prepared operand render records

Must not own:

- module emission
- semantic lowering policy
- hidden fallback orchestration
