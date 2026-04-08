# HPS Code Review 2026-04-03

## Scope

- Review target: current `html_parser` project
- Goal: identify correctness, architecture, performance, and maintainability issues for a crawler-oriented HTML parsing library
- Baseline: C++20 only, no DOM re-serialization requirement

## Summary

The project already has a usable parser/query skeleton, but several high-priority issues affect parsing correctness and query semantics. Before broader C++20 modernization or aggressive `clang-tidy` cleanup, the parser behavior must be corrected and stabilized.

## Findings

### P0

#### 1. Whitespace-only text is dropped before whitespace policy runs

- Location: `src/parsing/tokenizer.cpp`
- Problem:
  - `consume_data_state()` discards all-whitespace text before `TreeBuilder` can apply `WhitespaceMode`.
  - As a result, `WhitespaceMode::Preserve`, `Normalize`, and `Trim` do not operate on real DOM input.
- Impact:
  - Loses DOM fidelity
  - Breaks crawler-oriented text extraction expectations
  - Makes whitespace options partially ineffective
- Fix status: fixed in this round

#### 2. RAWTEXT/RCDATA states are not entered for several tags

- Location: `src/parsing/tokenizer.cpp`
- Problem:
  - The tokenizer only transitions to special text parsing for `script`, and incorrectly for `svg`.
  - `style`, `title`, and `textarea` contents are parsed as normal markup instead of raw text / rcdata.
- Confirmed repro:
  - `<style><b>x</b></style>` creates a real descendant `<b>`
  - `<textarea><b>x</b></textarea>` creates a real descendant `<b>`
- Impact:
  - Incorrect DOM tree
  - Incorrect text extraction
  - Breaks real-world page parsing
- Fix status: fixed in this round

#### 3. Element-scoped CSS queries can include the context element itself

- Location: `src/query/css/css_matcher.cpp`
- Problem:
  - `find_all(const Element&, ...)` and `find_first(const Element&, ...)` currently match the starting element before traversing descendants.
  - This gives element-scoped queries non-standard semantics.
- Impact:
  - `Element::querySelector(All)` and `ElementQuery::css()` can return the context node itself
  - Causes subtle selector behavior differences versus common crawler libraries and browser DOM APIs
- Fix status: fixed in this round

### P1

#### 4. Comment nodes leak into text extraction

- Location: `src/core/element.cpp`, `src/core/document.cpp`, `src/core/comment_node.cpp`
- Problem:
  - `Element::text_content()` concatenates child `text_content()`
  - `CommentNode::text_content()` returns comment content
- Impact:
  - Comments pollute extracted crawler text
- Fix status: fixed in this round

#### 5. Boolean attribute semantics are lost in DOM construction

- Location: `src/parsing/tree_builder.cpp`
- Problem:
  - `TokenAttribute::has_value` is preserved by the tokenizer
  - DOM construction rebuilds attributes through `Element::add_attribute(name, value)` and loses the boolean attribute distinction
- Impact:
  - `<input checked>` becomes indistinguishable from `checked=""`
- Fix status: fixed in this round

#### 6. Current clang-tidy configuration is not enforceable as a quality gate

- Location: `.clang-tidy`
- Problem:
  - Contains an invalid option value for current clang-tidy
  - Escalates broad readability buckets to errors
  - Naming rules conflict heavily with current code style and create large amounts of non-actionable output
- Impact:
  - Blocks meaningful bug/perf cleanup
  - Produces noise instead of a manageable remediation queue
- Fix status: narrowed and integrated in this round

## Recommended Priority Order

1. Fix parser correctness issues affecting DOM construction and text semantics
2. Fix selector/query semantic mismatches
3. Add regression tests for the corrected behavior
4. Narrow and integrate `clang-tidy` so it becomes actionable
5. Then start broader C++20/architecture/performance refactors

## Planned Work In This Round

- [x] Record findings in documentation
- [x] Fix whitespace-only text handling
- [x] Fix RAWTEXT/RCDATA state transitions
- [x] Fix element-scoped CSS query behavior
- [x] Add regression tests and run them

## Follow-up Improvements

- Remaining high-priority parser work advanced further in this round:
  - tree building now auto-inserts `html`, `head`, and `body` containers for full-document parsing instead of hanging body content directly under the document root
  - head-content tags such as `meta`, `title`, `style`, and `script` are routed into `head` before `body` exists
  - non-whitespace text encountered while `head` is open now forces a fall-back into `body`, matching crawler expectations better than the old flat stack model
  - EOF cleanup now treats common optional end tags such as `html`, `head`, `body`, `p`, `li`, `dd`, `dt`, `td`, `th`, `tr`, and section containers as implicitly closable instead of always reporting unclosed-tag noise
  - content depth limiting now ignores the implicit `html/head/body` wrappers so `max_depth` continues to describe user content depth rather than parser-introduced scaffolding
- Tokenizer baseline coverage also advanced in this round:
  - tokenizer construction now supports html5lib-style `initialStates` and `lastStartTag` inputs for targeted state testing
  - `PLAINTEXT` and `CDATA section` states are now exposed in the tokenizer state machine
  - special-text end-tag matching now uses the actual last start tag in `RCDATA` / `RAWTEXT` / `ScriptData` style flows instead of hard-coded tag assumptions
  - RCDATA state now decodes character/entity references before emitting text tokens, which matches the expected behavior for tags such as `title` and `textarea`
- Tree-construction baseline coverage is now integrated as a first gate:
  - a new html5lib-style `.dat` runner now validates curated document-construction cases in the repository test suite
  - the current subset now covers both document and fragment trees the existing parser can meaningfully support today: implicit `html/head/body`, head routing, selected implicit end tags, stray end tags before body content, special-text DOM construction, one layer of table normalization, and fragment contexts such as `div`, `textarea`, `table`, and `tr`
  - full parse-error matching, DOCTYPE tree serialization, and the complete HTML5 insertion-mode surface remain explicitly out of scope for this first integration
- Tree builder follow-up also advanced one step into table handling:
  - `tr` and `td` / `th` now trigger implied `tbody` / `tr` insertion when they appear directly under `table`
  - repeated table cells now implicitly close the previous cell instead of nesting one `td` under another
  - `caption` is now closed before row-group/body tokens, and bare `col` tokens now imply a reusable `colgroup`
  - when malformed markup leaves the parser inside `colgroup`, non-`col` start tags and non-whitespace text now pop back to table mode and are reprocessed there instead of being incorrectly nested inside the column group
  - table row/section/cell lookup is now constrained to the nearest open table, which fixes valid nested-table-in-cell cases and prevents inner-table cleanup from accidentally closing outer `td` / `tr` / `tbody`
  - `select/option/optgroup` now use a dedicated select-scope closure path, so repeated `option` tags and `optgroup` rollover no longer create invalid nesting and fragment parsing under `select` follows the same DOM shape
  - repeated `button` tags now implicitly close the previous button, and nested `form` start tags are now ignored with a recoverable parse error instead of building invalid nested forms
  - repeated `<a>` start tags now trigger a recoverable close/reopen step before the new anchor is inserted, which prevents invalid nested anchors in common malformed markup without requiring a full active-formatting-elements implementation
  - DOM elements now carry a namespace kind / namespace URI; `svg` is intentionally handled as an opaque crawler-oriented block so the outer node remains queryable while inner markup is preserved as raw text instead of being half-parsed into an unreliable SVG sub-DOM, and `math` keeps lightweight namespace propagation
  - closing `table`, `tbody` / `thead` / `tfoot`, and `tr` now first collapse open table descendants so the final DOM tree is structurally saner even before full HTML5 insertion modes are implemented
  - non-whitespace character data that appears in `table` / `tbody` / `thead` / `tfoot` / `tr` contexts is now foster-parented out of the table structure instead of being left as an invalid text child under table containers
  - non-table elements such as `<b>` that appear directly in table structure contexts are now foster-parented before the table and popped back off the open-element stack before the next structural table token
  - selected formatting end tags now perform a bounded recovery step for common misnesting such as `<b><i>x</b>y</i>`, reopening the formatting chain outside the closed element instead of flattening everything with a generic pop-until-match
- Fragment parsing is now a first-class API surface:
  - `HTMLParser` and the top-level `hps` facade now expose `parse_fragment(...)` and `parse_fragment_with_error(...)`
  - fragment parsing seeds tokenizer state from the context tag, so `textarea` / `title` fragment input uses `RCDATA`, `style` uses `RAWTEXT`, `script` uses `ScriptData`, and `plaintext` stays in `PLAINTEXT`
  - fragment results no longer force an `html/head/body` shell; instead they return only the parsed fragment nodes, which is the behavior crawler-side snippet extraction actually needs
- File parsing is now more robust across toolchains:
  - `parse_file()` no longer relies on the previous direct `read()` path that produced all-zero content in the clang/Windows build; it now uses a stream-buffer iterator path and preserves the file source correctly in both build configurations
  - `parse_file()` is now intentionally UTF-8-only at the parser boundary: UTF-8 BOM is stripped, plain UTF-8 files continue to parse, and non-UTF-8 file inputs now report `UnsupportedEncoding` instead of silently transcoding inside the parser
  - encoding sniffing/decoding has been moved into a dedicated `utils/encoding` module, so parser orchestration and file-byte handling no longer live in the same translation unit
  - the public helper surface is now explicit: `sniff_html_encoding(...)` reports BOM / `meta charset` / UTF-8 heuristic hints, and `decode_html_bytes_to_utf8(...)` only supports a deliberately small crawler-oriented helper set (`utf-16le/be`, `gbk`, `windows-1252`, `shift_jis`)
  - broader legacy transcoding is now intentionally outside the default parse path; callers that need it should sniff/decode first and then hand UTF-8 to the parser
- Error locations now resolve `position` into real `line` / `column` values through `Location::from_position(...)`, and parser/tokenizer/tree-builder errors now carry those coordinates when source context is available.
- Previously dead parser limits are now effective:
  - `max_depth` skips overly deep subtrees in lenient mode and raises `TooDeep`
  - `max_attributes` drops excess attributes and raises `TooManyAttributes`
  - `max_attribute_name_length` / `max_attribute_value_length` reject or truncate oversize attributes and raise `AttributeTooLong`
  - `max_text_length` truncates oversize text nodes and raises `TextTooLong`
- Entity decoding is no longer limited to `&nbsp;`:
  - common named entities such as `&amp;`, `&lt;`, `&gt;`, `&quot;`, `&apos;`
  - selected high-frequency HTML entities such as `&copy;`, `&reg;`, `&mdash;`, `&hellip;`
  - decimal and hexadecimal numeric entities
- `decode_entities` now has an actual effect even when `text_processing_mode` remains `Raw`.
- Query layer follow-up completed in this round:
  - document-scoped selector traversal now covers all top-level element subtrees instead of only `document.root()`
  - `Document::querySelector()` / `Element::querySelector()` now use first-match traversal instead of materializing full result sets
  - direct helpers `get_element_by_id()` / `get_elements_by_tag_name()` / `get_elements_by_class_name()` no longer route through synthesized CSS selector strings, so special characters such as `a:b` and `x:y` work correctly
  - simple selector parsing now uses a shared cached path, and `ElementQuery` gained `find()` as a chainable alias of descendant CSS lookup
  - HTML tag matching in the query engine is now case-insensitive even when `preserve_case = true`, including `TypeSelector`, `has_tag()`, `:checked`, `:enabled`, `:link`, and `*-of-type` pseudo-classes
- Additional query improvements completed in the latest round:
  - `:has()` now supports relative selectors such as `:has(> .child)`, `:has(+ .next)`, `:has(~ .later)`, and comma-separated mixes of those forms
  - `:has()` specificity now follows the maximum specificity of its argument list even for relative selectors
  - pseudo-class argument capture now preserves original token spelling through lexer source spans, avoiding dropped prefixes such as `#` in functional pseudo arguments
  - query-heavy DOM traversal paths now use sibling-pointer traversal (`first_child` / `next_sibling` / `previous_sibling`) instead of repeatedly materializing `children()` vectors
  - attribute word-match selectors (`[attr~=value]`) no longer use `std::istringstream` and now scan `string_view` directly
  - document-level `id` / `class` / `tag` lookup now uses a lazy query index cache instead of full-tree traversal on every call
  - attached DOM mutations (`Document::add_child`, `Element::add_child`, `Element::add_attribute`) now invalidate query indexes automatically, so cached lookups stay correct after tree edits
  - metadata caches (`title`, `charset`) now piggyback on the same invalidation path to avoid stale values after structural or attribute mutations

## Verification

- Build: `cmake --build build_review --config Debug`
- Test: `ctest --test-dir build_review -C Debug --output-on-failure`
- Result: 33 / 33 test targets passed
- Clang-tidy configure: `cmake -S . -B build_tidy -G "Ninja Multi-Config" -DHPS_BUILD_TESTING=OFF -DHPS_BUILD_EXAMPLES=OFF -DHPS_BUILD_BENCHMARK=OFF -DHPS_ENABLE_CLANG_TIDY=ON`
- Clang-tidy build: `cmake --build build_tidy --config Debug --target hps_static parsing_html_parser_tests parsing_html5lib_tokenizer_baseline_tests parsing_html5lib_tree_construction_baseline_tests parsing_tree_builder_tests parsing_tokenizer_states_tests`
- Clang-tidy result: configuration accepted by clang-tidy 22, integrated build passed on the library target and the key P0 regression targets
