# Canonical BIR Passes

Status: scaffold.

These passes mutate target-independent semantic BIR. They run only through the
ordered pipeline and only through contracted editors. They may request
revision-bound analyses, but cannot access preparation, MIR, target, or legacy
compatibility state.

Every pass document must define accepted raw/canonical forms, mutation scope,
required analyses, preservation/invalidation, verifier postconditions,
idempotence or fixed-point behavior, and exact legacy capability coverage.

The current order is `legalize -> scalar -> cfg -> ssa -> memory -> aggregate
-> intrinsics`.
