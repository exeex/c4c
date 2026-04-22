# `shared_call_support.cpp`

Primary role: shared state and helper plumbing used by call lowering and related emitters.

Key surfaces:

```cpp
X86CodegenState::X86CodegenState();
void append_asm_line(X86CodegenOutput* out, std::string line);
const char* load_dest_reg(IrType ty);
```

- Houses `X86CodegenState` construction/copy/move behavior and small helper routines used across calls, atomics, and other emitters.
- Provides the low-level output and register helper layer that larger lowering files build on top of.
- Serves as shared infrastructure rather than owning a standalone lowering family.
- Special-case classification: `core lowering` support surface.
