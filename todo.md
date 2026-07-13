Status: Active
Source Idea Path: ideas/open/730_post_legacy_bir_shell_bootstrap.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Implement builders, RawBir publication, and foundation verification

# Current Packet

## Just Finished

- Step 4A implemented noncopyable/nonmovable scoped module/function builders,
  monotonic nonzero module epochs, exact link-name declaration/definition
  merging, parameter construction, append-only blocks, checked foundation
  terminators, and explicit unsupported-opcode rejection.
- Added move-only `RawBir` ownership with borrowed `ModuleView`; its only
  constructor remains builder-private.
- Kept publication closed until Step 4B: `publish() &&` returns
  `PublishError::VerificationUnavailable` without consuming storage, so an
  unchecked `RawBir` cannot escape and the builder remains repairable.

## Suggested Next

- Execute Step 4B: implement the foundation verifier and connect successful
  `publish() &&` exclusively to zero-error verification.

## Watchouts

- `RawBir` construction exists only for Step 4B to call after verification;
  do not replace `VerificationUnavailable` with success until the complete
  foundation profile accepts the owned storage.
- `ModuleBuilder` restores `Open` after callback return or exception and
  invalidates the scope token before returning; future builder operations must
  preserve that synchronous capability boundary.
- The default build still stops at the separately owned missing
  `bir/lir_to_bir.hpp` backend consumer seam.

## Proof

- Packet-local C++17 runtime executable covered link-name merge/rejections,
  variadic signature conflicts, nested edit/create/publish rejection,
  parameter/block lookup, foreign owners, condition and return types, all four
  terminators, unsupported append, and non-consuming unavailable publication —
  passed; temporary source and binary removed.
- Inline facade/header syntax and `core/builder.cpp` syntax — passed.
- Negative facade probes confirmed builder storage is private, `RawBir` cannot
  be default-constructed, and unqualified owning `ModuleData` is unavailable —
  passed; temporary files removed.
- `git diff --check`, builder legacy-reference search, and compile-database
  `src/backend/legacy/` search — passed with no matches.
- `cmake --build --preset default -j 2` — regenerated with `builder.cpp` in the
  backend target, then reached the known external seam and failed because
  `src/backend/backend.hpp` includes missing `bir/lir_to_bir.hpp`.
- The delegated packet excluded logs, so no `test_after.log` was written.
