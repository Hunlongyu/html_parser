# Test Baselines

## Integrated today

- `html5lib-tests` tokenizer baseline
- Upstream: [html5lib/html5lib-tests](https://github.com/html5lib/html5lib-tests)
- Verified upstream commit: `8f43b7ec8c9d02179f5f38e0ea08cb5000fb9c9e`
- Local fixtures: `tests/baselines/html5lib/tokenizer/*.test`
- Test target: `parsing_html5lib_tokenizer_baseline_tests`
- `html5lib-tests` tree-construction baseline
- Upstream: [html5lib/html5lib-tests](https://github.com/html5lib/html5lib-tests)
- Verified upstream commit: `8f43b7ec8c9d02179f5f38e0ea08cb5000fb9c9e`
- Local fixtures: `tests/baselines/html5lib/tree-construction/*.dat`
- Test target: `parsing_html5lib_tree_construction_baseline_tests`

## Scope

The current integration keeps the original html5lib `.test` JSON format and runs a curated subset that matches the tokenizer surface area already implemented in this repository. The runner compares the emitted token stream after normalizing:

- `TEXT` -> `Character`
- `OPEN` / `CLOSE_SELF` -> `StartTag`
- adjacent text tokens are coalesced like html5lib expects
- curated parse errors are asserted by `code + line + column`

The current curated subset already covers:

- basic start/end/comment/text tokenization
- self-closing tag handling
- basic `DOCTYPE` name parsing and quirks-on-EOF behavior
- tokenizer `initialStates` / `lastStartTag` for `RCDATA`, `RAWTEXT`, and `PLAINTEXT`
- selected recoverable tokenizer errors such as missing attribute whitespace and EOF in comment/doctype
- partial end-tag handling in special text states

This lets the project adopt an official tokenizer baseline immediately without pretending that the library already supports the full HTML5 tokenizer API.

The tree-construction integration uses the original html5lib `.dat` section format and currently runs a curated subset of both document and fragment parsing. The runner already understands the core section markers:

- `#data`
- `#document`
- `#document-fragment`
- `#script-on`
- `#script-off`

The current tree subset asserts only the final serialized document tree, not the html5lib parse-error stream. That makes it useful as an implementation gate for the current tree builder without overclaiming full HTML5 algorithm coverage.

The current curated tree subset already covers:

- implicit insertion of `html`, `head`, and `body`
- routing of head-content elements into `head`
- head/body boundary fallback such as text appearing after `head`
- selected implicit end-tag behavior such as repeated `li`
- ignoring stray end tags before real body content
- RAWTEXT / RCDATA content surviving as text in the constructed DOM
- comment nodes before the root html element
- basic table normalization through implied `tbody` / `tr` insertion and implicit `td` closure
- selected table container handling through `caption`, explicit `colgroup`, and implied `colgroup` for bare `col`
- selected `colgroup` fallback behavior where non-`col` tokens pop back to table mode and are reprocessed
- valid nested `table` elements inside `td/th`, with row/section/cell lookup constrained to the nearest open table
- selected `select/option/optgroup` handling, including implicit `option` closure and `optgroup` rollover in both document and fragment parsing
- selected body-mode recovery for repeated `button` tags and ignored nested `form` start tags
- selected repeated-anchor recovery for nested `<a>` start tags in document and fragment parsing
- foster-parented non-whitespace text in table contexts
- foster-parented non-table elements in table contexts before structural tokens such as `tr`
- selected formatting-element recovery for common misnested cases such as `<b><i>x</b>y</i>`
- fragment parsing for generic container contexts such as `div`
- fragment parsing for special text contexts such as `textarea`
- fragment parsing for table contexts such as `table` and `tr`

## Current gaps

The baseline runner intentionally does not cover these html5lib features yet:

- `doubleEscaped`
- full DOCTYPE `PUBLIC` / `SYSTEM` identifier conformance checks
- the broader html5lib parse error catalog beyond the curated subset
- some tokenizer states still not covered by fixtures, such as more complete `CDATA section state` behavior
- document tree serialization for DOCTYPE nodes
- full namespace / foreign-content tree construction
- full adoption-agency handling and fuller insertion-mode behavior around malformed table/body interactions

Namespace-aware foreign-content checks are currently covered by direct unit tests rather than the html5lib tree baseline, because the baseline serializer does not yet emit namespace information. For crawler-oriented behavior, `svg` is also intentionally treated as an opaque block: the outer `svg` element is preserved, but its inner markup is retained as raw text instead of being asserted as a deep SVG DOM.

Those cases should be added only after the tokenizer surface is expanded to expose the missing state controls and metadata.

## Deferred baselines

These should be integrated next, but not forced in before the implementation is ready:

- WPT `querySelector` / selector semantics
- fuzzing with `ASan` / `UBSan`
- broader encoding fixtures beyond the current helper-level unit-test coverage for `sniff_html_encoding(...)` and `decode_html_bytes_to_utf8(...)`
  currently covered helper paths are UTF-8 BOM, UTF-16LE BOM, declared `gbk`, `iso-8859-1` -> `windows-1252`, and `windows-31j` / Shift_JIS
  parser-level file loading is now intentionally UTF-8-only and separately tested as a rejection path for non-UTF-8 inputs

The current tree builder is still intentionally lighter than the full HTML5 tree-construction algorithm, so the tree baseline remains curated rather than being used as a full conformance gate.
