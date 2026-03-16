#pragma once

#include <string>

namespace tinyc2ll::frontend_cxx {

// Translation-unit source language.
enum class SourceProfile {
  C,          // .c files
  CppSubset,  // .cpp files (restricted C++ subset)
  C4,         // .c4 files (future extension language)
};

// Lexer keyword classification profile.
enum class LexProfile {
  C,          // Standard C keywords only
  CppSubset,  // C keywords + template/constexpr/consteval
  C4,         // CppSubset keywords + future c4-only keywords
};

// Semantic analysis profile.
enum class SemaProfile {
  C,          // Standard C semantics
  CppSubset,  // C semantics + cpp-subset extensions
  C4,         // CppSubset semantics + c4 extensions
};

// Header file classification.
enum class HeaderKind {
  DotH,       // .h — inherits includer's profile
  DotHpp,     // .hpp — requires CppSubset or C4 profile
  Other,      // unknown header extension
};

// Infer SourceProfile from file extension. Returns C for unknown extensions.
inline SourceProfile source_profile_from_extension(const std::string& path) {
  auto dot = path.rfind('.');
  if (dot == std::string::npos) return SourceProfile::C;
  auto ext = path.substr(dot);
  if (ext == ".c") return SourceProfile::C;
  if (ext == ".cpp" || ext == ".cc" || ext == ".cxx") return SourceProfile::CppSubset;
  if (ext == ".c4") return SourceProfile::C4;
  return SourceProfile::C;
}

// Classify a header file extension.
inline HeaderKind header_kind_from_extension(const std::string& path) {
  auto dot = path.rfind('.');
  if (dot == std::string::npos) return HeaderKind::Other;
  auto ext = path.substr(dot);
  if (ext == ".h") return HeaderKind::DotH;
  if (ext == ".hpp" || ext == ".hxx" || ext == ".hh") return HeaderKind::DotHpp;
  return HeaderKind::Other;
}

// Convert SourceProfile to LexProfile.
inline LexProfile lex_profile_from(SourceProfile sp) {
  switch (sp) {
    case SourceProfile::C:         return LexProfile::C;
    case SourceProfile::CppSubset: return LexProfile::CppSubset;
    case SourceProfile::C4:        return LexProfile::C4;
  }
  return LexProfile::C;
}

// Convert SourceProfile to SemaProfile.
inline SemaProfile sema_profile_from(SourceProfile sp) {
  switch (sp) {
    case SourceProfile::C:         return SemaProfile::C;
    case SourceProfile::CppSubset: return SemaProfile::CppSubset;
    case SourceProfile::C4:        return SemaProfile::C4;
  }
  return SemaProfile::C;
}

// String representation for diagnostics.
inline const char* source_profile_name(SourceProfile sp) {
  switch (sp) {
    case SourceProfile::C:         return "C";
    case SourceProfile::CppSubset: return "C++ (subset)";
    case SourceProfile::C4:        return "c4";
  }
  return "unknown";
}

}  // namespace tinyc2ll::frontend_cxx
