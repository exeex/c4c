from dataclasses import dataclass
from enum import Enum, auto
from typing import List

from errors import CompileError
class TokenType(Enum):
    KW_INT = auto()
    KW_CHAR = auto()
    KW_VOID = auto()
    KW_EXTERN = auto()
    KW_RETURN = auto()
    KW_IF = auto()
    KW_ELSE = auto()
    KW_WHILE = auto()
    KW_FOR = auto()
    KW_DO = auto()
    KW_SWITCH = auto()
    KW_CASE = auto()
    KW_DEFAULT = auto()
    KW_BREAK = auto()
    KW_CONTINUE = auto()
    KW_GOTO = auto()
    KW_STRUCT = auto()
    KW_TYPEDEF = auto()
    KW_STATIC = auto()
    KW_SIZEOF = auto()
    KW_LONG = auto()
    KW_SHORT = auto()
    KW_UNSIGNED = auto()
    KW_SIGNED = auto()
    KW_DOUBLE = auto()
    KW_FLOAT = auto()
    KW_CONST = auto()
    KW_VOLATILE = auto()
    KW_RESTRICT = auto()
    KW_INLINE = auto()
    KW_ENUM = auto()
    KW_UNION = auto()
    KW_REGISTER = auto()
    ID = auto()
    NUM = auto()
    STRING = auto()
    ELLIPSIS = auto()
    LBRACE = auto()
    RBRACE = auto()
    LPAREN = auto()
    RPAREN = auto()
    LBRACKET = auto()
    RBRACKET = auto()
    SEMI = auto()
    COMMA = auto()
    DOT = auto()
    COLON = auto()
    QUESTION = auto()
    ASSIGN = auto()
    EQ = auto()
    NE = auto()
    LT = auto()
    LE = auto()
    GT = auto()
    GE = auto()
    PLUS = auto()
    PLUSEQ = auto()
    PLUSPLUS = auto()
    MINUS = auto()
    MINUSEQ = auto()
    MINUSMINUS = auto()
    ARROW = auto()
    STAR = auto()
    STAREQ = auto()
    SLASH = auto()
    SLASHEQ = auto()
    PERCENT = auto()
    PERCENTEQ = auto()
    AMP = auto()
    AMPAMP = auto()
    AMPEQ = auto()
    PIPE = auto()
    PIPEPIPE = auto()
    PIPEEQ = auto()
    CARET = auto()
    CARETEQ = auto()
    BANG = auto()
    TILDE = auto()
    LSHIFT = auto()
    RSHIFT = auto()
    LSHIFTEQ = auto()
    RSHIFTEQ = auto()
    EOF = auto()


@dataclass(frozen=True)
class Token:
    typ: TokenType
    text: str
    line: int
    col: int


SINGLE_CHAR_TOKENS = {
    "{": TokenType.LBRACE,
    "}": TokenType.RBRACE,
    "(": TokenType.LPAREN,
    ")": TokenType.RPAREN,
    "[": TokenType.LBRACKET,
    "]": TokenType.RBRACKET,
    ";": TokenType.SEMI,
    ",": TokenType.COMMA,
    ":": TokenType.COLON,
    "?": TokenType.QUESTION,
    "=": TokenType.ASSIGN,
    "+": TokenType.PLUS,
    "-": TokenType.MINUS,
    "*": TokenType.STAR,
    "/": TokenType.SLASH,
    "%": TokenType.PERCENT,
    "&": TokenType.AMP,
    "|": TokenType.PIPE,
    "^": TokenType.CARET,
    "!": TokenType.BANG,
    "<": TokenType.LT,
    ">": TokenType.GT,
    "~": TokenType.TILDE,
}


MULTI_CHAR_TOKENS = {
    "==": TokenType.EQ,
    "!=": TokenType.NE,
    "<=": TokenType.LE,
    ">=": TokenType.GE,
    "+=": TokenType.PLUSEQ,
    "-=": TokenType.MINUSEQ,
    "*=": TokenType.STAREQ,
    "/=": TokenType.SLASHEQ,
    "%=": TokenType.PERCENTEQ,
    "++": TokenType.PLUSPLUS,
    "--": TokenType.MINUSMINUS,
    "&&": TokenType.AMPAMP,
    "||": TokenType.PIPEPIPE,
    "->": TokenType.ARROW,
    "<<": TokenType.LSHIFT,
    ">>": TokenType.RSHIFT,
    "&=": TokenType.AMPEQ,
    "|=": TokenType.PIPEEQ,
    "^=": TokenType.CARETEQ,
}

THREE_CHAR_TOKENS = {
    "<<=": TokenType.LSHIFTEQ,
    ">>=": TokenType.RSHIFTEQ,
    "...": TokenType.ELLIPSIS,
}


class Lexer:
    def __init__(self, src: str):
        self.src = src
        self.i = 0
        self.line = 1
        self.col = 1

    def cur(self) -> str:
        if self.i >= len(self.src):
            return ""
        return self.src[self.i]

    def peek(self, n: int = 1) -> str:
        j = self.i + n
        if j >= len(self.src):
            return ""
        return self.src[j]

    def advance(self) -> str:
        ch = self.cur()
        if not ch:
            return ""
        self.i += 1
        if ch == "\n":
            self.line += 1
            self.col = 1
        else:
            self.col += 1
        return ch

    def scan_identifier(self) -> Token:
        start_line = self.line
        start_col = self.col
        buf = [self.advance()]
        while True:
            ch = self.cur()
            if ch.isalnum() or ch == "_" or ch == "$":
                buf.append(self.advance())
            else:
                break
        text = "".join(buf)
        kw_map = {
            "int": TokenType.KW_INT,
            "char": TokenType.KW_CHAR,
            "void": TokenType.KW_VOID,
            "extern": TokenType.KW_EXTERN,
            "return": TokenType.KW_RETURN,
            "if": TokenType.KW_IF,
            "else": TokenType.KW_ELSE,
            "while": TokenType.KW_WHILE,
            "for": TokenType.KW_FOR,
            "do": TokenType.KW_DO,
            "switch": TokenType.KW_SWITCH,
            "case": TokenType.KW_CASE,
            "default": TokenType.KW_DEFAULT,
            "break": TokenType.KW_BREAK,
            "continue": TokenType.KW_CONTINUE,
            "goto": TokenType.KW_GOTO,
            "struct": TokenType.KW_STRUCT,
            "typedef": TokenType.KW_TYPEDEF,
            "static": TokenType.KW_STATIC,
            "sizeof": TokenType.KW_SIZEOF,
            "long": TokenType.KW_LONG,
            "short": TokenType.KW_SHORT,
            "unsigned": TokenType.KW_UNSIGNED,
            "signed": TokenType.KW_SIGNED,
            "double": TokenType.KW_DOUBLE,
            "float": TokenType.KW_FLOAT,
            "const": TokenType.KW_CONST,
            "volatile": TokenType.KW_VOLATILE,
            "restrict": TokenType.KW_RESTRICT,
            "inline": TokenType.KW_INLINE,
            "enum": TokenType.KW_ENUM,
            "union": TokenType.KW_UNION,
            "register": TokenType.KW_REGISTER,
            "_Bool": TokenType.KW_INT,
        }
        return Token(kw_map.get(text, TokenType.ID), text, start_line, start_col)

    def scan_number(self) -> Token:
        start_line = self.line
        start_col = self.col
        buf = []
        # Handle binary: 0b...
        if self.cur() == "0" and self.peek() in ("b", "B"):
            buf.append(self.advance())  # 0
            buf.append(self.advance())  # b
            while self.cur() and self.cur() in "01":
                buf.append(self.advance())
            suffix_buf = []
            while self.cur() and self.cur() in "uUlL":
                suffix_buf.append(self.advance())
            return Token(TokenType.NUM, str(int("".join(buf), 2)) + "".join(suffix_buf), start_line, start_col)
        # Handle hex: 0x...
        if self.cur() == "0" and self.peek() in ("x", "X"):
            buf.append(self.advance())  # 0
            buf.append(self.advance())  # x
            # Hex float: 0x1.8p+1
            hex_digits = []
            while self.cur() and self.cur() in "0123456789abcdefABCDEF":
                hex_digits.append(self.advance())
            if self.cur() == "." and self.peek() and self.peek() in "0123456789abcdefABCDEF":
                # hex float literal: 0xHH.HHp±EE
                hex_digits.append(self.advance())  # .
                while self.cur() and self.cur() in "0123456789abcdefABCDEF":
                    hex_digits.append(self.advance())
                # mandatory exponent p/P
                exp_buf = []
                if self.cur() and self.cur() in "pP":
                    hex_digits.append(self.advance())
                    if self.cur() and self.cur() in "+-":
                        hex_digits.append(self.advance())
                    while self.cur() and self.cur().isdigit():
                        hex_digits.append(self.advance())
                while self.cur() and self.cur() in "fFlL":
                    self.advance()
                val = float.fromhex("0x" + "".join(hex_digits))
                return Token(TokenType.NUM, repr(val), start_line, start_col)
            buf.extend(hex_digits)
            # Include suffixes (L, U, LL, UL etc.) so parser can determine type
            suffix_buf = []
            while self.cur() and self.cur() in "uUlL":
                suffix_buf.append(self.advance())
            return Token(TokenType.NUM, str(int("".join(buf), 16)) + "".join(suffix_buf), start_line, start_col)
        # Decimal
        buf.append(self.advance())
        while self.cur() and self.cur().isdigit():
            buf.append(self.advance())
        # Handle float literals like 1.5, 1.0f
        is_float = False
        if self.cur() == "." and self.peek().isdigit():
            is_float = True
            buf.append(self.advance())  # .
            while self.cur() and self.cur().isdigit():
                buf.append(self.advance())
        # Exponent
        if self.cur() and self.cur() in "eE":
            is_float = True
            buf.append(self.advance())
            if self.cur() and self.cur() in "+-":
                buf.append(self.advance())
            while self.cur() and self.cur().isdigit():
                buf.append(self.advance())
        if is_float:
            # skip float suffixes (f, F, l, L) without recording them
            while self.cur() and self.cur() in "fFlL":
                self.advance()
            return Token(TokenType.NUM, "".join(buf), start_line, start_col)
        # For integer literals, include the suffix so the parser can determine type
        suffix_buf = []
        while self.cur() and self.cur() in "uUlL":
            suffix_buf.append(self.advance())
        return Token(TokenType.NUM, "".join(buf) + "".join(suffix_buf), start_line, start_col)

    def scan_string(self) -> Token:
        start_line = self.line
        start_col = self.col
        self.advance()  # eat opening "
        buf = []
        while True:
            ch = self.cur()
            if not ch:
                raise CompileError(f"{start_line}:{start_col}: unterminated string literal")
            if ch == '"':
                self.advance()
                break
            if ch == "\\":
                self.advance()
                esc = self.cur()
                if not esc:
                    break
                self.advance()
                escape_map = {
                    "n": "\n", "t": "\t", "r": "\r",
                    "\\": "\\", '"': '"', "'": "'",
                    "0": "\0", "a": "\a", "b": "\b", "f": "\f", "v": "\v",
                }
                if esc in escape_map:
                    buf.append(escape_map[esc])
                elif esc.isdigit():
                    # octal escape \NNN
                    oct_buf = [esc]
                    for _ in range(2):
                        if self.cur() and self.cur() in "01234567":
                            oct_buf.append(self.advance())
                    buf.append(chr(int("".join(oct_buf), 8)))
                elif esc == "x":
                    # hex escape \xNN
                    hex_buf = []
                    while self.cur() and self.cur() in "0123456789abcdefABCDEF":
                        hex_buf.append(self.advance())
                    if hex_buf:
                        buf.append(chr(int("".join(hex_buf), 16) & 0xFF))
                elif esc in ("u", "U"):
                    # Unicode escape: \uXXXX or \UXXXXXXXX — encode as UTF-8 bytes
                    n_digits = 4 if esc == "u" else 8
                    hex_buf = []
                    for _ in range(n_digits):
                        if self.cur() and self.cur() in "0123456789abcdefABCDEF":
                            hex_buf.append(self.advance())
                    if hex_buf:
                        codepoint = int("".join(hex_buf), 16)
                        for byte in chr(codepoint).encode("utf-8"):
                            buf.append(chr(byte))
                else:
                    buf.append(esc)
            else:
                buf.append(self.advance())
        return Token(TokenType.STRING, "".join(buf), start_line, start_col)

    def skip_paren_group(self) -> None:
        """Skip a balanced (possibly nested) parenthesized group starting at '('."""
        if self.cur() != "(":
            return
        depth = 0
        while self.cur():
            if self.cur() == "(":
                depth += 1
            elif self.cur() == ")":
                depth -= 1
                if depth == 0:
                    self.advance()
                    return
            self.advance()

    def scan(self) -> List[Token]:
        tokens: List[Token] = []
        while True:
            ch = self.cur()
            if not ch:
                break

            if ch.isspace():
                self.advance()
                continue

            if ch == "/" and self.peek() == "/":
                while self.cur() and self.cur() != "\n":
                    self.advance()
                continue

            if ch == "/" and self.peek() == "*":
                start_line, start_col = self.line, self.col
                self.advance()
                self.advance()
                while True:
                    if not self.cur():
                        raise CompileError(
                            f"{start_line}:{start_col}: unterminated block comment"
                        )
                    if self.cur() == "*" and self.peek() == "/":
                        self.advance()
                        self.advance()
                        break
                    self.advance()
                continue

            if ch == '"':
                # Scan string literal; allow adjacent string concatenation
                tok = self.scan_string()
                # Concatenate adjacent string literals (with optional whitespace between)
                while True:
                    j = self.i
                    while j < len(self.src) and self.src[j] in ' \t\n\r':
                        j += 1
                    if j < len(self.src) and self.src[j] == '"':
                        # skip whitespace
                        while self.cur() and self.cur() in ' \t\n\r':
                            self.advance()
                        next_tok = self.scan_string()
                        tok = Token(TokenType.STRING, tok.text + next_tok.text, tok.line, tok.col)
                    else:
                        break
                tokens.append(tok)
                continue

            if ch == "'":
                start_line, start_col = self.line, self.col
                self.advance()  # eat '
                if self.cur() == "\\":
                    self.advance()
                    esc = self.cur()
                    if esc:
                        self.advance()
                    escape_map = {
                        "n": "\n", "t": "\t", "r": "\r",
                        "\\": "\\", "'": "'", '"': '"',
                        "0": "\0", "a": "\a", "b": "\b", "f": "\f", "v": "\v",
                    }
                    if esc in escape_map:
                        char_val = ord(escape_map[esc])
                    elif esc and esc.isdigit():
                        oct_buf = [esc]
                        for _ in range(2):
                            if self.cur() and self.cur() in "01234567":
                                oct_buf.append(self.advance())
                        char_val = int("".join(oct_buf), 8)
                    elif esc == "x":
                        hex_buf = []
                        while self.cur() and self.cur() in "0123456789abcdefABCDEF":
                            hex_buf.append(self.advance())
                        char_val = int("".join(hex_buf), 16) if hex_buf else 0
                    else:
                        char_val = ord(esc) if esc else 0
                else:
                    c = self.cur()
                    self.advance()
                    char_val = ord(c) if c else 0
                # eat closing '
                if self.cur() == "'":
                    self.advance()
                tokens.append(Token(TokenType.NUM, str(char_val), start_line, start_col))
                continue

            if ch.isalpha() or ch == "_" or ch == "$":
                tok = self.scan_identifier()
                # Handle wide/unicode string/char prefixes: L"...", u"...", U"...", u8"...", L'...', etc.
                if tok.text in ("L", "u", "U", "u8") and self.cur() in ('"', "'"):
                    if self.cur() == '"':
                        str_tok = self.scan_string()
                        # Concatenate adjacent string literals (with optional whitespace)
                        while True:
                            j = self.i
                            while j < len(self.src) and self.src[j] in ' \t\n\r':
                                j += 1
                            if j < len(self.src) and self.src[j] in ('"', "'") and self.src[j] == '"':
                                while self.cur() and self.cur() in ' \t\n\r':
                                    self.advance()
                                next_tok = self.scan_string()
                                str_tok = Token(TokenType.STRING, str_tok.text + next_tok.text, str_tok.line, str_tok.col)
                            else:
                                break
                        tokens.append(str_tok)
                    else:
                        # Wide char literal - treat same as regular char
                        char_tok = Token(TokenType.NUM, "0", tok.line, tok.col)
                        start_line, start_col = self.line, self.col
                        self.advance()  # eat '
                        char_val = 0
                        if self.cur() == "\\":
                            self.advance()
                            esc = self.cur()
                            if esc:
                                self.advance()
                            escape_map = {
                                "n": "\n", "t": "\t", "r": "\r",
                                "\\": "\\", "'": "'", '"': '"',
                                "0": "\0", "a": "\a", "b": "\b", "f": "\f", "v": "\v",
                            }
                            if esc in escape_map:
                                char_val = ord(escape_map[esc])
                            elif esc and esc.isdigit():
                                oct_buf = [esc]
                                for _ in range(2):
                                    if self.cur() and self.cur() in "01234567":
                                        oct_buf.append(self.advance())
                                char_val = int("".join(oct_buf), 8)
                            elif esc == "x":
                                hex_buf = []
                                while self.cur() and self.cur() in "0123456789abcdefABCDEF":
                                    hex_buf.append(self.advance())
                                char_val = int("".join(hex_buf), 16) if hex_buf else 0
                            elif esc in ("u", "U"):
                                # Unicode escape in wide char literal: value is the code point
                                n_digits = 4 if esc == "u" else 8
                                hex_buf = []
                                for _ in range(n_digits):
                                    if self.cur() and self.cur() in "0123456789abcdefABCDEF":
                                        hex_buf.append(self.advance())
                                char_val = int("".join(hex_buf), 16) if hex_buf else 0
                            else:
                                char_val = ord(esc) if esc else 0
                        elif self.cur() and self.cur() != "'":
                            c = self.cur()
                            self.advance()
                            char_val = ord(c) if c else 0
                        if self.cur() == "'":
                            self.advance()
                        tokens.append(Token(TokenType.NUM, str(char_val), tok.line, tok.col))
                    continue
                # Skip GCC extensions: __attribute__, __asm__, __asm, __declspec, etc.
                skip_names = {
                    "__attribute__", "__asm__", "__asm", "__declspec",
                    "__cdecl", "__stdcall", "__fastcall", "__thiscall",
                    "__volatile__", "__const__", "__restrict__",
                    "__extension__", "__inline__", "__inline",
                    "__forceinline",
                }
                if tok.text in skip_names:
                    # Skip the following ((...)) if present
                    # Skip optional whitespace
                    while self.cur() and self.cur().isspace():
                        self.advance()
                    if self.cur() == "(":
                        self.skip_paren_group()
                    continue
                # Map __restrict, __volatile, __const to ignore
                remap_names = {
                    "__restrict": None, "__volatile": None, "__const": None,
                    "__signed__": "signed", "__unsigned__": "unsigned",
                    "__builtin_va_list": "__va_list",
                    "__typeof__": None,  # skip typeof altogether
                }
                if tok.text in remap_names:
                    mapped = remap_names[tok.text]
                    if mapped is None:
                        # If it's __typeof__, skip (expr) too
                        if tok.text == "__typeof__":
                            while self.cur() and self.cur().isspace():
                                self.advance()
                            if self.cur() == "(":
                                self.skip_paren_group()
                        continue
                    # remap to the correct keyword token type
                    kw_remap = {
                        "signed": TokenType.KW_SIGNED,
                        "unsigned": TokenType.KW_UNSIGNED,
                        "int": TokenType.KW_INT,
                        "char": TokenType.KW_CHAR,
                        "void": TokenType.KW_VOID,
                        "float": TokenType.KW_FLOAT,
                        "double": TokenType.KW_DOUBLE,
                    }
                    new_typ = kw_remap.get(mapped, TokenType.ID)
                    tok = Token(new_typ, mapped, tok.line, tok.col)
                tokens.append(tok)
                continue

            if ch.isdigit():
                tokens.append(self.scan_number())
                continue

            # Handle .NN float literals (.5, .25, etc.)
            if ch == "." and self.peek().isdigit():
                start_line, start_col = self.line, self.col
                buf = ["0", self.advance()]  # prepend 0, consume dot
                while self.cur() and self.cur().isdigit():
                    buf.append(self.advance())
                # Exponent
                if self.cur() and self.cur() in "eE":
                    buf.append(self.advance())
                    if self.cur() and self.cur() in "+-":
                        buf.append(self.advance())
                    while self.cur() and self.cur().isdigit():
                        buf.append(self.advance())
                while self.cur() and self.cur() in "uUlLfF":
                    self.advance()
                tokens.append(Token(TokenType.NUM, "".join(buf), start_line, start_col))
                continue

            # Check 3-char tokens first
            three = ch + self.peek() + self.peek(2)
            tok_type = THREE_CHAR_TOKENS.get(three)
            if tok_type is not None:
                tokens.append(Token(tok_type, three, self.line, self.col))
                self.advance()
                self.advance()
                self.advance()
                continue

            two = ch + self.peek()
            tok_type = MULTI_CHAR_TOKENS.get(two)
            if tok_type is not None:
                tokens.append(Token(tok_type, two, self.line, self.col))
                self.advance()
                self.advance()
                continue

            # Handle dot specially (struct member access vs float)
            if ch == ".":
                tokens.append(Token(TokenType.DOT, ".", self.line, self.col))
                self.advance()
                continue

            tok_type = SINGLE_CHAR_TOKENS.get(ch)
            if tok_type is not None:
                tokens.append(Token(tok_type, ch, self.line, self.col))
                self.advance()
                continue

            # Skip unknown characters (for robustness with system headers)
            raise CompileError(f"{self.line}:{self.col}: unexpected character {ch!r}")

        tokens.append(Token(TokenType.EOF, "", self.line, self.col))
        return tokens
