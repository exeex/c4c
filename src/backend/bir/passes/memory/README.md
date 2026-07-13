# Memory Canonicalization Pass

Status: scaffold.

Input: canonical SSA BIR. Output: explicit target-independent memory and address
semantics.

Owns local/global load-store forms, semantic GEP/address expressions, atomics,
memory intrinsic semantics, and representation needed for recomputable
provenance/effect analysis. It does not choose address modes, stack offsets, or
physical storage.

Legacy coverage to review: legacy memory access/provenance views; prealloc
addressing, atomics, dynamic stack semantics, pointer freshness, local/global
memory paths, and object-data references.

Exact audit anchors include `pointer_value_memory_freshness.hpp` and every
memory/addressing producer that currently reconstructs freshness indirectly.
