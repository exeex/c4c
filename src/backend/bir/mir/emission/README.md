# Target Emission Boundary

Status: scaffold.

Emission consumes final verified MIR, target object-data products, and explicit
relocation/symbol information. It performs encoding, assembly/object writing,
and read-only rendering; it cannot query canonical BIR, preparation internals,
or the legacy capsule to recover missing decisions.

Legacy coverage: object data, global/static bytes, symbols, relocations,
constant pools, target assemblers, ELF writers, object emission, debug machine
printing, and unsupported-encoding diagnostics.

Per-target coverage is tracked under [`targets/`](../targets/README.md).
