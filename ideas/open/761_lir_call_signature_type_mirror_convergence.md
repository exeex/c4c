# LIR call and signature type mirror convergence

## Intent

After the LIR typed ref enum foundation and string-constructor deprecation
migration, converge call and signature type carriers so preformatted string
fragments no longer act as semantic authority.

This idea depends on:

- `ideas/open/759_lir_typed_ref_enum_foundation.md`
- `ideas/open/760_lir_string_constructor_deprecation_migration.md`

It should run after 760 has classified the remaining string construction and
runtime text boundaries.

## Why This Exists

`src/codegen/lir/ir.hpp` already carries structured type mirrors for many call
and signature paths, but several legacy fields still keep rendered type
fragments next to typed refs:

- `LirCallSignature::fixed_param_types` beside `fixed_param_type_refs`
- `LirCallArg::type` beside `type_ref`
- `LirCallOp::args_str` beside `structured_args` and `arg_type_refs`
- `LirSwitch::selector_type` beside structured selector value authority
- `LirInlineAsmOp::args_str` beside ordinary typed value bindings

Those fields are useful emission or compatibility payloads, but after 759/760
they should not remain ambiguous semantic inputs. The next step is to make the
typed carriers the clear authority and push rendered fragments to the printer
or explicit compatibility boundary.

## In Scope

- Audit call/signature fields in `src/codegen/lir/ir.hpp` and their producers,
  verifier checks, printer use, and backend consumers.
- Prefer `LirTypeRef`-based call/signature carriers over
  `std::vector<std::string>` or preformatted argument text when semantic type
  identity is required.
- Convert straightforward call argument and fixed parameter paths so typed
  refs are produced directly and string fragments are rendered from them when
  needed.
- Make `LirCallOp::args_str` an explicit compatibility/emission payload, or
  replace local uses with rendering from `structured_args` where the structured
  data is complete.
- Convert `LirSwitch::selector_type` to a typed mirror such as `LirTypeRef`, or
  otherwise make the selector type authority derive from structured typed state
  rather than raw text.
- Treat `LirInlineAsmOp::args_str` carefully: ordinary value bindings may be
  structured, but assembly template and constraint strings remain opaque text.
- Add focused coverage proving that misleading call/signature type strings do
  not override typed refs when structured data is present.

## Out Of Scope

- Removing every compatibility string in one change.
- Replacing aggregate, vector, struct, or function type text with a full typed
  type tree.
- Changing inline assembly template or constraint semantics.
- Raw-BIR receiver work, target lowering, MIR, or LLVM emission rewrites beyond
  rendering from typed call/signature carriers.
- Generic value identity, PHI/CFG, local object pointer authority, memory/va
  pointer authority, or aggregate/vector identity; those belong to other open
  ideas.
- Downgrading verifier checks or test expectations to tolerate weaker typed
  authority.

## Acceptance Criteria

- Call and signature semantic consumers prefer `LirTypeRef` and structured
  argument/signature carriers over legacy type strings.
- `LirCallSignature::fixed_param_types` and `LirCallArg::type` are no longer
  the primary semantic source when their typed-ref mirrors are present.
- `LirCallOp::args_str` is either rendered from structured data at the boundary
  or clearly marked as compatibility-only where raw fallback is still required.
- `LirSwitch::selector_type` no longer creates a raw-string type authority gap.
- Inline assembly ordinary value typing uses `LirInlineAsmValueBinding` typed
  facts; opaque asm text and constraints remain text by design.
- Focused verifier/printer/backend tests reject or ignore misleading type text
  when authoritative typed call/signature state exists.
- The repository builds and the focused LIR/frontend/backend tests selected for
  call/signature type behavior pass.

## Suggested Starting Points

- `src/codegen/lir/ir.hpp`
- `src/codegen/lir/call_args.hpp`
- `src/codegen/lir/call_args_ops.hpp`
- `src/codegen/lir/hir_to_lir/call/`
- `src/codegen/lir/verify.cpp`
- `src/codegen/lir/print.cpp`
- frontend/backend tests that already cover `LirCallOp`, extern calls, inline
  asm, and switch lowering

## Reviewer Reject Signals

- The patch treats preformatted `args_str` as proof of call argument type
  identity when structured args or type refs are available.
- The patch removes compatibility text before all current raw fallback paths
  have an explicit replacement or fail-closed behavior.
- Inline assembly template or constraint text is parsed as semantic value/type
  authority.
- The change rewrites Raw-BIR, target lowering, MIR, or unrelated value
  identity families instead of converging call/signature type mirrors.
- Tests are weakened, expectations are downgraded, or verifier rules are
  relaxed to hide a remaining string-authority path.
- The result still lets mismatched string type fragments override typed refs in
  call/signature verification or backend consumption.
