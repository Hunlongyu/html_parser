# CSS Module Optimization Plan (with StringPool)

## Phase 1: Establish Test Coverage
**Goal**: Ensure current behavior is verified before refactoring.
1.  **Create Directory**: `tests/query/`
2.  **Create Test File**: `tests/query/css_selector_test.cpp`
    *   Implement tests for Basic Selectors, Combinators, Attributes, and Pseudo-classes.

## Phase 2: Implement StringPool
**Goal**: Create a memory-efficient string storage mechanism.
1.  **Create File**: `include/hps/utils/string_pool.hpp`
    *   Implement a `StringPool` class.
    *   **Strategy**: Use a list/deque of memory blocks (chunks) to ensure pointer stability.
    *   **API**: `std::string_view add(std::string_view str)` that copies data into the pool and returns a view.

## Phase 3: Integrate StringPool into CSS Module
**Goal**: Reduce memory allocations by using `string_view`.
1.  **Refactor `CSSLexer`**:
    *   Inject `StringPool&` into constructor.
    *   Store preprocessed input in `StringPool`.
    *   Change `CSSToken::value` to `std::string_view`.
2.  **Refactor `CSSSelector` & AST Nodes**:
    *   Update `TypeSelector`, `ClassSelector`, `IdSelector`, `AttributeSelector` to store `std::string_view`.
    *   Update constructors to accept `std::string_view`.
3.  **Update `CSSParser`**:
    *   Manage `StringPool` instance (likely owned by Parser or passed in).
    *   Pass Pool to Lexer.

## Phase 4: Control Flow Optimization
1.  **Optimize `is_valid_selector`**:
    *   Add `CSSParser::validate()` or `try_parse()` (no-throw).
    *   Update `css_utils.hpp`.
2.  **Optimize `normalize_selector`**:
    *   Rewrite to write directly to StringPool or buffer to avoid temp `std::string`.

**Execution Order**: Phase 1 -> Phase 2 -> Phase 3 -> Phase 4.