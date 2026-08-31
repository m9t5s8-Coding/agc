#pragma once
#include <string_view>

namespace ag::error {

// ============================================================
// Lexer / tokenization errors
// ============================================================
inline constexpr std::string_view InvalidCharacter      = "\"Invalid character\"";
inline constexpr std::string_view InvalidNumberLiteral  = "\"Invalid number literal\"";
inline constexpr std::string_view InvalidEscapeSequence = "\"Invalid escape sequence\"";
inline constexpr std::string_view InvalidHexLiteral     = "\"Invalid hexadecimal literal\"";
inline constexpr std::string_view InvalidOctalLiteral   = "\"Invalid octal literal\"";
inline constexpr std::string_view InvalidBinaryLiteral  = "\"Invalid binary literal\"";
inline constexpr std::string_view InvalidFloatLiteral   = "\"Invalid floating-point literal\"";
inline constexpr std::string_view NumberOutOfRange      = "\"Number literal out of range\"";
inline constexpr std::string_view UnterminatedString    = "\"Unterminated string literal\"";
inline constexpr std::string_view UnterminatedChar      = "\"Unterminated character literal\"";
inline constexpr std::string_view UnterminatedComment   = "\"Unterminated comment\"";
inline constexpr std::string_view EmptyCharLiteral      = "\"Empty character literal\"";
inline constexpr std::string_view CharLiteralTooLong    = "\"Character literal contains too many characters\"";
inline constexpr std::string_view UnexpectedToken       = "\"Unexpected token\"";
inline constexpr std::string_view UnexpectedEndOfFile   = "\"Unexpected end of file\"";
inline constexpr std::string_view UnexpectedNewline     = "\"Unexpected newline\"";
inline constexpr std::string_view UnrecognizedEscape    = "\"Unrecognized escape sequence\"";

// ============================================================
// Expected: identifiers / names
// ============================================================
inline constexpr std::string_view ExpectedIdentifier       = "\"Expected: an identifier\"";
inline constexpr std::string_view ExpectedFunctionName     = "\"Expected: function name\"";
inline constexpr std::string_view ExpectedParameterName    = "\"Expected: parameter name\"";
inline constexpr std::string_view ExpectedVariableName     = "\"Expected: variable name\"";
inline constexpr std::string_view ExpectedTypeName         = "\"Expected: type name\"";
inline constexpr std::string_view ExpectedFieldName        = "\"Expected: field name\"";
inline constexpr std::string_view ExpectedMemberName       = "\"Expected: member name\"";
inline constexpr std::string_view ExpectedModuleName       = "\"Expected: module name\"";
inline constexpr std::string_view ExpectedNamespaceName    = "\"Expected: namespace name\"";
inline constexpr std::string_view ExpectedLabelName        = "\"Expected: label name\"";
inline constexpr std::string_view ExpectedEnumName         = "\"Expected: enum name\"";
inline constexpr std::string_view ExpectedEnumMemberName   = "\"Expected: enum member name\"";
inline constexpr std::string_view ExpectedStructName       = "\"Expected: struct name\"";
inline constexpr std::string_view ExpectedClassName        = "\"Expected: class name\"";
inline constexpr std::string_view ExpectedInterfaceName    = "\"Expected: interface name\"";
inline constexpr std::string_view ExpectedAliasName        = "\"Expected: alias name\"";
inline constexpr std::string_view ExpectedGenericParamName = "\"Expected: generic parameter name\"";

// ============================================================
// Expected: punctuation / symbols
// ============================================================
inline constexpr std::string_view ExpectedLeftParen       = "\"Expected: '('\"";
inline constexpr std::string_view ExpectedRightParen      = "\"Expected: ')'\"";
inline constexpr std::string_view ExpectedLeftBrace       = "\"Expected: '{'\"";
inline constexpr std::string_view ExpectedRightBrace      = "\"Expected: '}'\"";
inline constexpr std::string_view ExpectedLeftBracket     = "\"Expected: '['\"";
inline constexpr std::string_view ExpectedRightBracket    = "\"Expected: ']'\"";
inline constexpr std::string_view ExpectedLeftAngle       = "\"Expected: '<'\"";
inline constexpr std::string_view ExpectedRightAngle      = "\"Expected: '>'\"";
inline constexpr std::string_view ExpectedColon           = "\"Expected: ':'\"";
inline constexpr std::string_view ExpectedDoubleColon     = "\"Expected: '::'\"";
inline constexpr std::string_view ExpectedSemicolon       = "\"Expected: ';'\"";
inline constexpr std::string_view ExpectedComma           = "\"Expected: ','\"";
inline constexpr std::string_view ExpectedDot             = "\"Expected: '.'\"";
inline constexpr std::string_view ExpectedDotDot          = "\"Expected: '..'\"";
inline constexpr std::string_view ExpectedEllipsis        = "\"Expected: '...'\"";
inline constexpr std::string_view ExpectedEqual           = "\"Expected: '='\"";
inline constexpr std::string_view ExpectedEqualEqual      = "\"Expected: '=='\"";
inline constexpr std::string_view ExpectedArrow           = "\"Expected: '->'\"";
inline constexpr std::string_view ExpectedFatArrow        = "\"Expected: '=>'\"";
inline constexpr std::string_view ExpectedPipe            = "\"Expected: '|'\"";
inline constexpr std::string_view ExpectedAmpersand       = "\"Expected: '&'\"";
inline constexpr std::string_view ExpectedQuestionMark    = "\"Expected: '?'\"";
inline constexpr std::string_view ExpectedAt              = "\"Expected: '@'\"";
inline constexpr std::string_view ExpectedHash            = "\"Expected: '#'\"";
inline constexpr std::string_view ExpectedUnderscore      = "\"Expected: '_'\"";
inline constexpr std::string_view ExpectedAfterReturnType = "\"Expected '{' or ';' after return type\"";

// ============================================================
// Expected: grammar constructs
// ============================================================
inline constexpr std::string_view ExpectedExpression       = "\"Expected: expression\"";
inline constexpr std::string_view ExpectedStatement        = "\"Expected: statement\"";
inline constexpr std::string_view ExpectedDeclaration      = "\"Expected: declaration\"";
inline constexpr std::string_view ExpectedType             = "\"Expected: type\"";
inline constexpr std::string_view ExpectedReturnType       = "\"Expected: return type\"";
inline constexpr std::string_view ExpectedString           = "\"Expected: string\"";
inline constexpr std::string_view ExpectedNumber           = "\"Expected: number\"";
inline constexpr std::string_view ExpectedBoolean          = "\"Expected: boolean\"";
inline constexpr std::string_view ExpectedValue            = "\"Expected: value\"";
inline constexpr std::string_view ExpectedOperator         = "\"Expected: operator\"";
inline constexpr std::string_view ExpectedKeyword          = "\"Expected: keyword\"";
inline constexpr std::string_view ExpectedBlock            = "\"Expected: block\"";
inline constexpr std::string_view ExpectedCondition        = "\"Expected: condition\"";
inline constexpr std::string_view ExpectedBody             = "\"Expected: body\"";
inline constexpr std::string_view ExpectedArgumentList     = "\"Expected: argument list\"";
inline constexpr std::string_view ExpectedParameterList    = "\"Expected: parameter list\"";
inline constexpr std::string_view ExpectedGenericArgs      = "\"Expected: generic arguments\"";
inline constexpr std::string_view ExpectedGenericParams    = "\"Expected: generic parameters\"";
inline constexpr std::string_view ExpectedInitializer      = "\"Expected: initializer\"";
inline constexpr std::string_view ExpectedAssignmentTarget = "\"Expected: assignment target\"";
inline constexpr std::string_view ExpectedPattern          = "\"Expected: pattern\"";
inline constexpr std::string_view ExpectedCaseLabel        = "\"Expected: case label\"";
inline constexpr std::string_view ExpectedImportPath       = "\"Expected: import path\"";
inline constexpr std::string_view ExpectedAnnotation       = "\"Expected: annotation\"";

// ============================================================
// Mismatched / unbalanced delimiters
// ============================================================
inline constexpr std::string_view MismatchedParentheses   = "\"Mismatched parentheses\"";
inline constexpr std::string_view MismatchedBraces        = "\"Mismatched braces\"";
inline constexpr std::string_view MismatchedBrackets      = "\"Mismatched brackets\"";
inline constexpr std::string_view MismatchedAngleBrackets = "\"Mismatched angle brackets\"";
inline constexpr std::string_view UnbalancedDelimiter     = "\"Unbalanced delimiter\"";

// ============================================================
// Duplicate / redeclaration errors
// ============================================================
inline constexpr std::string_view DuplicateIdentifier    = "\"Duplicate identifier\"";
inline constexpr std::string_view DuplicateParameterName = "\"Duplicate parameter name\"";
inline constexpr std::string_view DuplicateFieldName     = "\"Duplicate field name\"";
inline constexpr std::string_view DuplicateEnumMember    = "\"Duplicate enum member\"";
inline constexpr std::string_view DuplicateCaseLabel     = "\"Duplicate case label\"";
inline constexpr std::string_view DuplicateDefaultCase   = "\"Duplicate default case\"";
inline constexpr std::string_view RedefinitionOfSymbol   = "\"Redefinition of symbol\"";
inline constexpr std::string_view RedefinitionOfFunction = "\"Redefinition of function\"";
inline constexpr std::string_view RedefinitionOfType     = "\"Redefinition of type\"";

// ============================================================
// Semantic / type-checking errors
// ============================================================
inline constexpr std::string_view UndefinedIdentifier     = "\"Undefined identifier\"";
inline constexpr std::string_view UndefinedFunction       = "\"Undefined function\"";
inline constexpr std::string_view UndefinedType           = "\"Undefined type\"";
inline constexpr std::string_view UndefinedVariable       = "\"Undefined variable\"";
inline constexpr std::string_view UndefinedMember         = "\"Undefined member\"";
inline constexpr std::string_view TypeMismatch            = "\"Type mismatch\"";
inline constexpr std::string_view IncompatibleTypes       = "\"Incompatible types\"";
inline constexpr std::string_view CannotInferType         = "\"Cannot infer type\"";
inline constexpr std::string_view InvalidArgumentCount    = "\"Invalid argument count\"";
inline constexpr std::string_view InvalidArgumentType     = "\"Invalid argument type\"";
inline constexpr std::string_view InvalidReturnType       = "\"Invalid return type\"";
inline constexpr std::string_view MissingReturnStatement  = "\"Missing return statement\"";
inline constexpr std::string_view UnreachableCode         = "\"Unreachable code\"";
inline constexpr std::string_view ImmutableAssignment     = "\"Cannot assign to immutable variable\"";
inline constexpr std::string_view UseOfUninitialized      = "\"Use of uninitialized variable\"";
inline constexpr std::string_view InvalidOperandTypes     = "\"Invalid operand types\"";
inline constexpr std::string_view DivisionByZero          = "\"Division by zero\"";
inline constexpr std::string_view IndexOutOfBounds        = "\"Index out of bounds\"";
inline constexpr std::string_view CircularDependency      = "\"Circular dependency detected\"";
inline constexpr std::string_view RecursiveTypeDefinition = "\"Recursive type definition\"";
inline constexpr std::string_view InvalidCast             = "\"Invalid type cast\"";
inline constexpr std::string_view AmbiguousOverload       = "\"Ambiguous overload resolution\"";
inline constexpr std::string_view NoMatchingOverload      = "\"No matching overload found\"";
inline constexpr std::string_view AbstractInstantiation   = "\"Cannot instantiate abstract type\"";

// ============================================================
// Control-flow errors
// ============================================================
inline constexpr std::string_view BreakOutsideLoop      = "\"'break' outside of loop\"";
inline constexpr std::string_view ContinueOutsideLoop   = "\"'continue' outside of loop\"";
inline constexpr std::string_view ReturnOutsideFunction = "\"'return' outside of function\"";
inline constexpr std::string_view YieldOutsideGenerator = "\"'yield' outside of generator\"";
inline constexpr std::string_view AwaitOutsideAsync     = "\"'await' outside of async function\"";

} // namespace ag::error
