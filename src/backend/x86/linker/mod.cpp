// Mechanical C++ translation of ref/claudes-c-compiler/src/backend/x86/linker/mod.rs
// Native x86-64 ELF linker module surface.

#include "mod.hpp"

namespace c4c::backend::x86::linker {

// The Rust module exposes the ELF helpers, the x86-specific symbol type, the
// input loader, the PLT/GOT builder, and the two public linker entry points.
// In C++ we keep the same module-level shape through declarations in mod.hpp
// and define the real logic in the sibling translation units.

}  // namespace c4c::backend::x86::linker

