# `x86_target_abi.cpp` Contract

## Legacy Evidence

The extraction below records the old convenience-policy role for target
resolution, register narrowing, and symbol/label spelling.

## Purpose And Current Responsibility

This file is a small policy hub for x86 backend target facts that are needed
during code emission but are not tied to one instruction-selection step. It
normalizes target identity from `PreparedBirModule`, answers whether the
resolved target is x86, and renders a few target-sensitive names used in
assembly output.

The implementation is not a full ABI model. It is a convenience layer that
mixes:

- target resolution fallback
- coarse target-family classification
- register-name width narrowing
- object-format naming quirks for exported symbols and private labels

## Important APIs And Contract Surfaces

Essential surface from the paired header:

```cpp
[[nodiscard]] std::string resolve_target_triple(
    const c4c::backend::prepare::PreparedBirModule& module);
[[nodiscard]] c4c::TargetProfile resolve_target_profile(
    const c4c::backend::prepare::PreparedBirModule& module);
[[nodiscard]] bool is_x86_target(const c4c::TargetProfile& target_profile);
```

```cpp
[[nodiscard]] bool is_apple_darwin_target(std::string_view target_triple);
[[nodiscard]] std::string narrow_i32_register_name(std::string_view wide_register);
[[nodiscard]] std::string render_asm_symbol_name(std::string_view target_triple,
                                                 std::string_view logical_name);
[[nodiscard]] std::string render_private_data_label(std::string_view target_triple,
                                                    std::string_view pool_name);
```

Behavior contracts:

- `resolve_target_triple` prefers `module.module.target_triple`; otherwise it
  falls back to `default_host_target_triple()`.
- `resolve_target_profile` prefers `module.target_profile` when its `arch` is
  already known; otherwise it derives a profile from the resolved triple.
- `is_x86_target` only recognizes `X86_64` and `I686`.
- `narrow_i32_register_name` maps 64-bit GPR spellings to their 32-bit forms
  and leaves unknown names unchanged.
- `render_asm_symbol_name` prepends `_` on Apple Darwin and leaves other
  targets unchanged.
- `render_private_data_label` strips leading `@` and leading `.` from the pool
  name, then emits Darwin-local labels as `L...` and non-Darwin labels as
  `.L....`

## Dependency Direction And Hidden Inputs

Direct dependencies flow inward from generic backend state:

- `PreparedBirModule`
- `TargetProfile`
- `TargetArch`
- `default_host_target_triple()`
- `target_profile_from_triple(...)`

Hidden inputs and assumptions:

- Host fallback is environment-sensitive because `default_host_target_triple()`
  can change behavior based on the build/runtime host.
- Darwin detection is substring-based on `"apple-darwin"`, so target-object
  formatting is driven by text shape rather than a structured object-format
  enum.
- Register narrowing is string-driven, with a hardcoded table for GPR names.
- Private-label rendering assumes callers may pass pool names with leading `@`
  or `.` noise that should be sanitized locally.

## Responsibility Buckets

### 1. Target Resolution

- Resolve a usable triple from `PreparedBirModule`
- Resolve a usable `TargetProfile`
- Prefer already-known profile data over reparsing the triple

### 2. Backend Gating

- Answer whether the resolved target belongs to the x86 backend family

### 3. Register Spelling Adaptation

- Convert wide x86 GPR names into their 32-bit spellings for emission paths
  that need explicit i32 register names

### 4. Assembly Naming Policy

- Render public symbol names
- Render backend-private data labels
- Hide Darwin vs non-Darwin spelling differences from callers

## Fast Paths, Compatibility Paths, And Overfit Pressure

Fast paths:

- `resolve_target_profile` short-circuits when `module.target_profile.arch` is
  already known.
- `is_x86_target` is a narrow enum check instead of any richer capability test.

Compatibility paths:

- Host-triple fallback keeps codegen alive when the module does not carry an
  explicit target triple.
- Darwin-specific symbol underscore and local-label spelling support platform
  assembly compatibility.
- Label sanitization accepts legacy caller inputs with `@` or `.` prefixes.

Overfit pressure:

- Darwin detection by substring is expedient but fragile if target spelling or
  object-format handling broadens.
- Register narrowing is a manual whitelist and will drift if new register forms
  or aliases start flowing through the backend.
- Label rendering currently bundles target detection, sanitization, and format
  policy in one helper surface, which invites more target-specific string
  exceptions instead of a structured naming policy.

## Design Contract

Current rebuild role:

- resolve target triple/profile for x86 codegen consumers
- provide a tiny naming-policy surface for public symbols and private labels
- keep width-narrowing helpers for the small register cases that the rebuilt
  codegen currently needs

Owned inputs:

- `PreparedBirModule`
- target triple strings
- logical symbol or pool names

Owned outputs:

- resolved target triple/profile
- narrow i32 register spellings
- assembly symbol and private-label names

Must not own:

- a full ABI or calling-convention model
- object-format orchestration
- target-specific workaround growth unrelated to naming or target resolution

Deferred gaps:

- richer object-format policy should move to a more structured surface if the
  backend grows beyond these minimal helpers

## Rebuild Ownership Boundary

This file should own small, explicit target-sensitive naming and fallback
helpers that are reused across x86 code emission.

This file should not own a full ABI model, calling-convention decisions,
object-format abstraction, or expanding tables of target- and testcase-shaped
string special cases.
