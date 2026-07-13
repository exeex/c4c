# Legacy Compatibility Quarantine

Status: scaffold.

Compatibility is a temporary, observational boundary for legacy display names,
route snapshots, lookup fallbacks, pointer/index observations, and debug
publication records. It is not writable by canonical passes and cannot affect
verification, preparation, MIR, or emission decisions.

Review must define an exact field manifest, closed writer/reader allowlist,
monotonically decreasing checkpoints, and a terminal zero-field/zero-reader
deletion gate. New design documents may cite legacy behavior but may not add a
new compatibility authority.
