# LIR Backend Legacy Type Surface Readiness Audit

Status: Step 3 follow-up scope mapped
Plan Step: Step 3 - Map Follow-Up Scope For Ideas 113, 114, And 115
Scope: report-only; no implementation edits

## Step 1 Inventory

| Surface | Code location | Reader/writer ownership | Consumer role | Candidate blocker class |
|---|---|---|---|---|
| `LirModule::type_decls` | `src/codegen/lir/ir.hpp:671` | Written by HIR-to-LIR through `build_type_decls` / module assembly; read by verifier and BIR lowering | Legacy struct declaration text, still the backend type-decl map source | `backend-blocked` |
| `LirModule::struct_decls` | `src/codegen/lir/ir.hpp:676` | Written through `record_struct_decl`; read by printer/verifier and lookup mirrors | Structured declaration authority for LLVM struct declaration printing | `legacy-proof-only` |
| struct declaration shadow proof | `src/codegen/lir/verify.cpp:409` | Verifier reads both `type_decls` and `struct_decls` | Compatibility proof that structured declarations still match legacy text | `legacy-proof-only` |
| struct declaration printer authority | `src/codegen/lir/lir_printer.cpp:529` | Printer reads `struct_decls`; no longer reads `type_decls` for declaration emission | Final rendering of struct declarations | `printer-only` |
| `LirStructuredLayoutObservation` | `src/codegen/lir/ir.hpp:560`, `src/codegen/lir/hir_to_lir/core.cpp:82` | HIR-to-LIR writes lookup observations; verifier reads them | Dual-path layout parity evidence | `legacy-proof-only` |
| `StructuredLayoutLookup::legacy_decl` | `src/codegen/lir/hir_to_lir/lowering.hpp:283`, `src/codegen/lir/hir_to_lir/core.cpp:116` | HIR-to-LIR lookup writes legacy `HirStructDef*`; selected lowering code reads size/align/field facts | HIR layout authority fallback | `layout-authority-blocked` |
| object alignment via legacy layout | `src/codegen/lir/hir_to_lir/core.cpp:466` | `object_align_bytes` reads `legacy_decl->align_bytes` | Alignment authority for aggregate objects | `layout-authority-blocked` |
| const-init aggregate lookup | `src/codegen/lir/hir_to_lir/const_init_emitter.cpp:94` | Const-init emitter prefers `legacy_decl` | Aggregate initializer layout lookup bridge | `layout-authority-blocked` |
| variadic aggregate argument sizing | `src/codegen/lir/hir_to_lir/call/args.cpp:166` | Call arg lowering reads `legacy_decl->size_bytes` | ABI payload sizing for variadic aggregate args | `layout-authority-blocked` |
| `va_arg` aggregate sizing | `src/codegen/lir/hir_to_lir/call/vaarg.cpp:148`, `src/codegen/lir/hir_to_lir/call/vaarg.cpp:264` | Vaarg lowering reads legacy struct size/fields | ABI payload sizing and zero aggregate handling | `layout-authority-blocked` |
| field-chain structured name mirror | `src/codegen/lir/hir_to_lir/types.cpp:43`, `src/codegen/lir/hir_to_lir/types.cpp:56` | Field lookup reads legacy `HirStructDef` and structured name id | Bridge from legacy field traversal to structured type refs | `bridge-required` |
| indexed GEP aggregate type ref mirror | `src/codegen/lir/hir_to_lir/lvalue.cpp:660` | Lvalue lowering reads structured lookup when available, falls back to raw text | Structured mirror for aggregate GEP element type | `needs-more-parity-proof` |
| `LirGlobal::llvm_type` | `src/codegen/lir/ir.hpp:540`, `src/codegen/lir/lir_printer.cpp:546` | HIR-to-LIR writes; printer and BIR globals read | Final emitted global type text and backend lowering input | `bridge-required` |
| `LirGlobal::llvm_type_ref` | `src/codegen/lir/ir.hpp:541`, `src/codegen/lir/verify.cpp:467` | HIR-to-LIR writes optional mirror; verifier reads | Structured mirror proof for global type text | `needs-more-parity-proof` |
| `LirFunction::signature_text` | `src/codegen/lir/ir.hpp:512`, `src/codegen/lir/lir_printer.cpp:587` | HIR-to-LIR writes preformatted signature; printer and BIR ABI parsing read | Final emitted function signature and backend ABI fallback | `bridge-required` |
| function signature mirrors | `src/codegen/lir/ir.hpp:506`, `src/codegen/lir/verify.cpp:575` | HIR-to-LIR writes return/param mirrors; verifier parses signature text | Shadow proof against preformatted signature text | `needs-more-parity-proof` |
| `LirExternDecl::return_type_str` | `src/codegen/lir/ir.hpp:491`, `src/codegen/lir/lir_printer.cpp:576` | Extern decl recording writes; printer and BIR decl lowering read | Final emitted extern return type and backend fallback | `bridge-required` |
| `LirExternDecl::return_type` | `src/codegen/lir/ir.hpp:492`, `src/codegen/lir/verify.cpp:445` | Extern decl recording writes mirror; verifier reads | Structured mirror proof for extern return type | `needs-more-parity-proof` |
| call return type text | `src/codegen/lir/ir.hpp:280`, `src/backend/bir/lir_to_bir/calling.cpp:438` | HIR-to-LIR writes `LirCallOp::return_type`; BIR lowering reads `str()` | Active call return ABI and lowering authority | `type-ref-authority-blocked` |
| call argument text and mirrors | `src/codegen/lir/ir.hpp:283`, `src/codegen/lir/ir.hpp:285`, `src/codegen/lir/verify.cpp:239` | HIR-to-LIR writes `callee_type_suffix`, `args_str`, and `arg_type_refs`; verifier and BIR parse text | Typed-call argument parsing and mirror proof | `type-ref-authority-blocked` |
| raw `LirTypeRef` identity/output | `src/codegen/lir/types.hpp:33`, `src/codegen/lir/types.hpp:90`, `src/codegen/lir/types.hpp:151` | All LIR producers construct; verifier/printer/backend consume via `str()`, comparisons, stream output | Text remains canonical identity for most operation types | `type-ref-authority-blocked` |
| raw string-only `LirTypeRef` construction | `src/codegen/lir/types.hpp:36`, `src/codegen/lir/ir.hpp:193` | LIR instruction fields hold type refs built from raw LLVM spelling | Broad instruction-type surface without semantic ids | `type-ref-authority-blocked` |
| BIR type-decl map builder | `src/backend/bir/lir_to_bir/types.cpp:73`, `src/backend/bir/lir_to_bir/module.cpp:789` | BIR module lowering builds `TypeDeclMap` from `module.type_decls` | Backend aggregate layout source | `backend-blocked` |
| BIR aggregate layout parser | `src/backend/bir/lir_to_bir/types.cpp:42`, `src/backend/bir/lir_to_bir/types.cpp:204` | BIR type helpers resolve `%struct` names through `TypeDeclMap` and parse text bodies | Active aggregate size/align/field authority | `backend-blocked` |
| BIR globals and initializers | `src/backend/bir/lir_to_bir/globals.cpp:56`, `src/backend/bir/lir_to_bir/global_initializers.cpp:521` | BIR globals read `global.llvm_type` and `type_decls` | Global type lowering, aggregate initializer, and pointer offset lowering | `backend-blocked` |
| BIR call ABI paths | `src/backend/bir/lir_to_bir/call_abi.cpp:224`, `src/backend/bir/lir_to_bir/calling.cpp:617` | BIR ABI lowering parses signature/call text and aggregate type text | sret/byval/HFA-ish call and return lowering | `backend-blocked` |
| BIR memory layout consumers | `src/backend/bir/lir_to_bir/memory/addressing.cpp:99`, `src/backend/bir/lir_to_bir/memory/local_gep.cpp:62`, `src/backend/bir/lir_to_bir/memory/local_slots.cpp:25`, `src/backend/bir/lir_to_bir/memory/intrinsics.cpp:173` | Memory lowering reads `TypeDeclMap` and LIR type text | GEP, slots, memcpy/memset, provenance, byte-addressing layout authority | `backend-blocked` |
| MIR/aarch64 legacy text consumers | `src/backend/mir/aarch64/codegen/emit.cpp:1040`, `src/backend/mir/aarch64/codegen/emit.cpp:4577`, `src/backend/mir/aarch64/codegen/emit.cpp:4672`, `src/backend/mir/aarch64/codegen/emit.cpp:5332` | MIR/aarch64 reads `module.type_decls`, `signature_text`, `return_type_str`, and `global.llvm_type` | Legacy backend implementation residue | `planned-rebuild` |

## Step 1 Notes

- The primary active blocker is BIR/backend aggregate layout: `module.type_decls`
  is transformed into `TypeDeclMap` and then used by globals, calls, memory
  addressing, local slots, local GEP, provenance, and intrinsics.
- The HIR-to-LIR layout blocker is narrower but still authoritative:
  `StructuredLayoutLookup::legacy_decl` remains the source for selected
  size/align and aggregate payload decisions even when structured mirrors are
  observed.
- `struct_decls` is printer authority for struct declaration emission, but
  `type_decls` remains mandatory as proof shadow and backend input.
- `LirTypeRef` still compares and renders by raw text. Struct-name ids are
  mirrors, not general authority.
- MIR/aarch64 findings are classification-only. `src/backend/mir/` is not a
  current migration target; if those files block compilation, the follow-up
  should be compile-target exclusion rather than MIR type-surface migration.

## Step 2 Blocker Ownership Map

| Surface | Step 2 classification | Blocker owner | Ownership note |
|---|---|---|---|
| `LirModule::type_decls` | `backend-blocked` | Active BIR/backend | Still feeds `TypeDeclMap`; cannot be demoted until BIR aggregate layout has a structured source. |
| `LirModule::struct_decls` | `legacy-proof-only` | LIR verifier/proof | Structured declarations are the printer source, but this surface remains tied to legacy parity proof until `type_decls` is no longer required. |
| struct declaration shadow proof | `legacy-proof-only` | LIR verifier/proof | Verifies structured declarations against legacy declaration text; proof-only after printer authority moved to `struct_decls`. |
| struct declaration printer authority | `printer-only` | LIR printer | This is final declaration rendering, not a backend blocker; preserve until the printer has a non-legacy declaration rendering path. |
| `LirStructuredLayoutObservation` | `legacy-proof-only` | HIR-to-LIR verifier/proof | Records dual-path layout evidence only; it should disappear with parity proof, not as an active backend dependency. |
| `StructuredLayoutLookup::legacy_decl` | `layout-authority-blocked` | HIR-to-LIR layout | Still provides authoritative size, alignment, and field fallback facts for selected lowering paths. |
| object alignment via legacy layout | `layout-authority-blocked` | HIR-to-LIR layout | `object_align_bytes` still falls back to `legacy_decl->align_bytes`; needs structured layout authority first. |
| const-init aggregate lookup | `layout-authority-blocked` | HIR-to-LIR layout | Aggregate initializer lowering still prefers legacy layout lookup; bridge must be replaced by structured layout facts. |
| variadic aggregate argument sizing | `layout-authority-blocked` | HIR-to-LIR call lowering | Variadic aggregate payload sizing reads `legacy_decl->size_bytes`; cannot be removed without ABI parity. |
| `va_arg` aggregate sizing | `layout-authority-blocked` | HIR-to-LIR `va_arg` lowering | `va_arg` payload and zero-aggregate handling still read legacy size and fields. |
| field-chain structured name mirror | `bridge-required` | HIR-to-LIR field lookup | Bridges legacy field traversal to structured type refs; demotion needs a structured field-chain source. |
| indexed GEP aggregate type ref mirror | `needs-more-parity-proof` | HIR-to-LIR lvalue proof gap | Structured mirror exists with raw-text fallback; needs indexed-GEP parity proof before demotion. |
| `LirGlobal::llvm_type` | `bridge-required` | LIR printer and BIR globals | Final global type text is still consumed by the printer and backend; needs a structured-to-text bridge before demotion. |
| `LirGlobal::llvm_type_ref` | `needs-more-parity-proof` | LIR verifier/proof gap | Mirror is optional proof, not authority; needs coverage showing mirror completeness against `llvm_type`. |
| `LirFunction::signature_text` | `bridge-required` | LIR printer and BIR ABI | Final function signature text remains printer output and backend ABI fallback. |
| function signature mirrors | `needs-more-parity-proof` | LIR verifier/proof gap | Return/parameter mirrors are shadow proof against formatted signatures; completeness is not proven enough for authority. |
| `LirExternDecl::return_type_str` | `bridge-required` | LIR printer and BIR decl lowering | Extern return text is still final output and backend input; structured return type needs a bridge. |
| `LirExternDecl::return_type` | `needs-more-parity-proof` | LIR verifier/proof gap | Structured mirror is proof-only until all extern return text paths are covered. |
| call return type text | `type-ref-authority-blocked` | Raw LIR type-ref/BIR call lowering | BIR reads `LirCallOp::return_type.str()` as active lowering authority. |
| call argument text and mirrors | `type-ref-authority-blocked` | Raw LIR type-ref/BIR call lowering | Typed-call text remains parsed by verifier/backend; mirrors do not yet replace argument text authority. |
| raw `LirTypeRef` identity/output | `type-ref-authority-blocked` | LIR type-ref authority | `str()`, text equality, and stream output remain canonical identity for most operation types. |
| raw string-only `LirTypeRef` construction | `type-ref-authority-blocked` | LIR producer/type-ref authority | Instruction fields can still be constructed from raw LLVM spelling without semantic ids. |
| BIR type-decl map builder | `backend-blocked` | Active BIR/backend | Converts `module.type_decls` into backend layout authority. |
| BIR aggregate layout parser | `backend-blocked` | Active BIR/backend | Resolves `%struct` text and parses declaration bodies for size/align/field facts. |
| BIR globals and initializers | `backend-blocked` | Active BIR/backend | Global lowering, aggregate initialization, and pointer offsets still consume type text and `TypeDeclMap`. |
| BIR call ABI paths | `backend-blocked` | Active BIR/backend | sret/byval/HFA-ish call and return lowering still parse signatures and aggregate type text. |
| BIR memory layout consumers | `backend-blocked` | Active BIR/backend | GEP, slots, memcpy/memset, provenance, and byte-addressing still use text-backed aggregate layout. |
| MIR/aarch64 legacy text consumers | `planned-rebuild` | Planned MIR rebuild residue | Classification-only; this route should not migrate MIR internals. Compile failures should become compile-target exclusion work. |

### Active Backend Blockers

- `backend-blocked`: `LirModule::type_decls`, the BIR type-decl map builder,
  the BIR aggregate layout parser, BIR globals/initializers, BIR call ABI
  paths, and BIR memory layout consumers. These are active BIR/backend
  authority and must be separated from legacy proof shadows.
- `layout-authority-blocked`: `StructuredLayoutLookup::legacy_decl` and its
  object alignment, const-init, variadic aggregate, and `va_arg` consumers.
  These are HIR-to-LIR layout authority blockers, not backend parser blockers.
- `type-ref-authority-blocked`: call return type text, call argument text, raw
  `LirTypeRef` identity/output, and raw string-only construction. These block
  type authority demotion even when aggregate declarations have structured
  mirrors.

### Non-Backend Residue

- `planned-rebuild`: MIR/aarch64 legacy text consumers only. They are excluded
  from current BIR/LIR cleanup and should not block backend structured layout
  work.
- `printer-only`: struct declaration printer authority. This is final output,
  not active backend layout authority.
- `legacy-proof-only`: `LirModule::struct_decls`, struct declaration shadow
  proof, and `LirStructuredLayoutObservation`. These remain as compatibility
  checks until the active authority surfaces are gone.

### Proof Gaps

- No Step 1 surface is classified `safe-to-demote` on current evidence.
- `needs-more-parity-proof`: indexed GEP aggregate type ref mirror,
  `LirGlobal::llvm_type_ref`, function signature mirrors, and
  `LirExternDecl::return_type`. These are intentionally recorded as proof gaps
  rather than demotion candidates.
- `bridge-required`: field-chain structured name mirror,
  `LirGlobal::llvm_type`, `LirFunction::signature_text`, and
  `LirExternDecl::return_type_str`. These need structured authority plus a
  final rendering or backend bridge before the legacy text can be demoted.

## Step 3 Follow-Up Scope Map

### Idea 113: Backend StructDecl Layout Table Dual Path

Idea 113 should own the active BIR/backend layout blockers that still consume
`module.type_decls` or parse aggregate type text as backend layout authority:

| Finding | Why it belongs to idea 113 | Required fallback/parity rule |
|---|---|---|
| `LirModule::type_decls` | It is still the source used to build BIR aggregate layout maps. | Do not remove it; keep legacy parsing as the fallback and parity source. |
| BIR type-decl map builder | It converts `module.type_decls` into backend `TypeDeclMap` authority. | Add structured table input from `module.struct_decls` only where legacy parity can be checked. |
| BIR aggregate layout parser | It parses `%struct` declaration bodies for size, alignment, and fields. | Preserve text parser behavior and record structured-vs-legacy mismatches. |
| BIR globals and initializers | Global lowering, aggregate initializer lowering, and pointer offsets still consume backend layout facts. | Prefer structured backend layout only after matching legacy output for touched paths. |
| BIR call ABI paths | sret/byval/HFA-ish call and return lowering still parse signatures and aggregate type text. | Keep signature/type-text fallback where ABI lowering is not yet structured-safe. |
| BIR memory layout consumers | GEP, slots, memcpy/memset, provenance, and byte-addressing still use text-backed layout. | Convert consumers behind a structured layout table with legacy fallback. |

Idea 113 should not own these Step 1 findings:

- `StructuredLayoutLookup::legacy_decl` consumers: these are HIR-to-LIR layout
  authority issues and belong to idea 115 after backend dual-path support
  exists.
- Raw `LirTypeRef` identity/output, raw string-only `LirTypeRef`
  construction, call return type text, and call argument text/mirrors: these
  are type-ref authority blockers or text-authority cleanup candidates for idea
  114 only after proof exists.
- `LirGlobal::llvm_type`, `LirFunction::signature_text`, and
  `LirExternDecl::return_type_str`: these are final emitted/printer/backend
  bridge surfaces, not the initial backend layout table conversion itself.
- `legacy-proof-only` verifier/proof surfaces: these remain compatibility
  proof until backend authority and layout authority blockers have structured
  alternatives.
- MIR/aarch64 legacy text consumers: MIR is planned-rebuild residue, not an
  idea 113 migration target or blocker.

### Idea 114: LIR Type Text Authority Demotion

Current evidence does not classify any Step 1 surface as fully
`safe-to-demote`. Idea 114 should therefore start with proof-only or
proof-gap-aware candidates and must not remove text that remains backend,
layout, bridge, or type-ref authority.

Suitable idea 114 candidates:

| Finding | Current class | Follow-up shape |
|---|---|---|
| `LirModule::struct_decls` | `legacy-proof-only` | Keep as structured declaration authority; only demote related legacy proof once `type_decls` no longer acts as backend authority. |
| struct declaration shadow proof | `legacy-proof-only` | Candidate for proof simplification after idea 113 proves backend layout no longer depends on declaration text authority. |
| `LirStructuredLayoutObservation` | `legacy-proof-only` | Candidate for removing or narrowing dual-path observation only after idea 115 converts the corresponding HIR-to-LIR consumers. |
| indexed GEP aggregate type ref mirror | `needs-more-parity-proof` | Proof-gap-aware candidate: strengthen parity coverage before any demotion. |
| `LirGlobal::llvm_type_ref` | `needs-more-parity-proof` | Proof-gap-aware candidate: prove mirror completeness against `llvm_type`; keep emitted text. |
| function signature mirrors | `needs-more-parity-proof` | Proof-gap-aware candidate: prove return/parameter mirrors cover signature text before structured-first authority. |
| `LirExternDecl::return_type` | `needs-more-parity-proof` | Proof-gap-aware candidate: prove structured return type coverage while keeping `return_type_str` for output/backend fallback. |

Idea 114 must leave these out of scope until their blockers are cleared:

- `backend-blocked`: `LirModule::type_decls` and BIR layout consumers remain
  idea 113 work.
- `layout-authority-blocked`: `StructuredLayoutLookup::legacy_decl` consumers
  remain idea 115 work.
- `type-ref-authority-blocked`: call return type text, call argument text and
  mirrors, raw `LirTypeRef` identity/output, and raw string-only construction
  are not demotion candidates until structured type identity replaces text
  authority for the touched operation families.
- `bridge-required`: field-chain structured name mirror,
  `LirGlobal::llvm_type`, `LirFunction::signature_text`, and
  `LirExternDecl::return_type_str` need structured authority plus final
  rendering/backend bridges before demotion.
- MIR/aarch64 legacy text consumers are not cleanup targets or blockers for
  idea 114.

### Idea 115: HIR-To-LIR Layout LegacyDecl Demotion

Idea 115 should own the HIR-to-LIR layout consumers that currently read
`StructuredLayoutLookup::legacy_decl` for size, alignment, field, or aggregate
payload facts. These are not backend parser blockers; they should move to
structured layout facts only where `StructNameId` is available and parity is
stable.

| Finding | HIR-to-LIR consumer scope | Required fallback |
|---|---|---|
| `StructuredLayoutLookup::legacy_decl` | Shared lookup bridge that still exposes legacy `HirStructDef*` layout facts. | Keep `legacy_decl` until all selected consumers can use structured layout facts first. |
| object alignment via legacy layout | Aggregate object alignment in HIR-to-LIR lowering. | Use structured alignment when available; fall back to `legacy_decl->align_bytes`. |
| const-init aggregate lookup | Aggregate initializer and field lookup. | Keep legacy lookup for incomplete structured coverage and mismatch proof. |
| variadic aggregate argument sizing | Variadic aggregate payload sizing in call argument lowering. | Use structured size only with ABI parity proof; keep legacy size fallback. |
| `va_arg` aggregate sizing | `va_arg` payload sizing and zero-aggregate handling. | Preserve legacy size/field fallback until structured handling covers zero and nested aggregate cases. |
| field-chain structured name mirror | Field-chain and nested struct traversal bridge. | Keep the legacy field traversal bridge until structured field-chain lookup is authoritative. |
| indexed GEP aggregate type ref mirror | Lvalue/GEP aggregate element type mirror. | Treat as proof-gap-aware: keep raw text fallback until parity proves structured element refs are complete. |

Idea 115 should not own BIR `TypeDeclMap` migration, raw `LirTypeRef`
identity demotion, or final emitted text fields such as
`LirGlobal::llvm_type`, `LirFunction::signature_text`, and
`LirExternDecl::return_type_str`; those remain idea 113 or 114 concerns as
classified above.

### MIR Boundary For Follow-Ups

MIR/aarch64 legacy text consumers are classification-only `planned-rebuild`
residue. MIR is not a current migration target for ideas 113, 114, or 115 and
must not be treated as a blocker for these follow-ups. If MIR files block
compilation while LIR/BIR work proceeds, the follow-up should be compile-target
exclusion, not MIR type-surface migration.

## Must Not Remove Yet

- `LirModule::type_decls`, because active BIR lowering still builds its
  aggregate layout map from it.
- `StructuredLayoutLookup::legacy_decl`, because HIR-to-LIR still consumes
  legacy size/align and payload facts.
- `LirGlobal::llvm_type`, `LirFunction::signature_text`,
  `LirExternDecl::return_type_str`, call formatted argument text, and raw
  `LirTypeRef::str()` authority, because printer and backend consumers still
  read those text forms.
- MIR/aarch64 legacy reads should not be migrated on this route; leave them as
  planned-rebuild residue.
