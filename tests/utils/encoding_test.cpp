#include "hps/hps.hpp"
#include "hps/utils/encoding.hpp"

#include <algorithm>
#include <string>
#include <string_view>

#include <gtest/gtest.h>

namespace hps::tests {
namespace {

// ==================== 测试夹具构造 ====================

auto utf16le_with_bom(std::u16string_view text) -> std::string {
    std::string bytes("\xFF\xFE", 2);
    bytes.reserve(bytes.size() + text.size() * 2);
    for (const char16_t code_unit : text) {
        bytes.push_back(static_cast<char>(code_unit & 0xFF));
        bytes.push_back(static_cast<char>((code_unit >> 8) & 0xFF));
    }
    return bytes;
}

auto utf16be_with_bom(std::u16string_view text) -> std::string {
    std::string bytes("\xFE\xFF", 2);
    bytes.reserve(bytes.size() + text.size() * 2);
    for (const char16_t code_unit : text) {
        bytes.push_back(static_cast<char>((code_unit >> 8) & 0xFF));
        bytes.push_back(static_cast<char>(code_unit & 0xFF));
    }
    return bytes;
}

// 常用多字节字节序列（已在实现/历史测试中验证）
const std::string kGbkZhongWen("\xD6\xD0\xCE\xC4", 4);   // GBK “中文”
const std::string kBig5Zhong("\xA4\xA4", 2);             // Big5 “中”
const std::string kShiftJisNihon("\x93\xFA\x96\x7B", 4); // Shift_JIS “日本”
const std::string kUtf8Bom("\xEF\xBB\xBF", 3);

auto contains(std::string_view haystack, std::string_view needle) -> bool {
    return haystack.find(needle) != std::string_view::npos;
}

}  // namespace

// ==================================================================
// detect_encoding —— 仅检测
// ==================================================================

TEST(DetectEncoding, EmptyInputIsUnknown) {
    const auto d = detect_encoding("");
    EXPECT_FALSE(d.found());
    EXPECT_EQ(d.source, EncodingSource::Unknown);
    EXPECT_TRUE(d.label.empty());
    EXPECT_EQ(d.bom_length, 0u);
}

TEST(DetectEncoding, Utf8Bom) {
    const auto d = detect_encoding(kUtf8Bom + "<div>x</div>");
    EXPECT_TRUE(d.found());
    EXPECT_EQ(d.source, EncodingSource::ByteOrderMark);
    EXPECT_EQ(d.label, "utf-8");
    EXPECT_TRUE(d.supported);
    EXPECT_EQ(d.bom_length, 3u);
}

TEST(DetectEncoding, Utf16LeBom) {
    const auto d = detect_encoding(utf16le_with_bom(u"<x/>"));
    EXPECT_EQ(d.source, EncodingSource::ByteOrderMark);
    EXPECT_EQ(d.label, "utf-16le");
    EXPECT_TRUE(d.supported);
    EXPECT_EQ(d.bom_length, 2u);
}

TEST(DetectEncoding, Utf16BeBom) {
    const auto d = detect_encoding(utf16be_with_bom(u"<x/>"));
    EXPECT_EQ(d.source, EncodingSource::ByteOrderMark);
    EXPECT_EQ(d.label, "utf-16be");
    EXPECT_TRUE(d.supported);
    EXPECT_EQ(d.bom_length, 2u);
}

TEST(DetectEncoding, MetaCharsetGbkAliasIsCanonicalized) {
    const auto d = detect_encoding("<html><head><meta charset='gb2312'></head><body>x</body></html>");
    EXPECT_EQ(d.source, EncodingSource::MetaCharset);
    EXPECT_EQ(d.label, "gbk");  // gb2312 规范化为 gbk
    EXPECT_TRUE(d.supported);
    EXPECT_EQ(d.bom_length, 0u);
}

TEST(DetectEncoding, MetaCharsetBig5) {
    const auto d = detect_encoding("<meta charset=\"big5\"><p>x</p>");
    EXPECT_EQ(d.source, EncodingSource::MetaCharset);
    EXPECT_EQ(d.label, "big5");
    EXPECT_TRUE(d.supported);
}

TEST(DetectEncoding, MetaHttpEquivContentType) {
    const auto d = detect_encoding(
        R"(<meta http-equiv="Content-Type" content="text/html; charset=shift_jis">)");
    EXPECT_EQ(d.source, EncodingSource::MetaCharset);
    EXPECT_EQ(d.label, "shift_jis");
    EXPECT_TRUE(d.supported);
}

TEST(DetectEncoding, MetaCharsetUnsupportedIsReportedButFound) {
    const auto d = detect_encoding("<meta charset='euc-jp'><p>x</p>");
    EXPECT_TRUE(d.found());
    EXPECT_EQ(d.source, EncodingSource::MetaCharset);
    EXPECT_EQ(d.label, "euc-jp");
    EXPECT_FALSE(d.supported);
}

TEST(DetectEncoding, MetaCharsetIsCaseInsensitiveAndUnquoted) {
    const auto d = detect_encoding("<META CHARSET=GBK><body>x</body>");
    EXPECT_EQ(d.source, EncodingSource::MetaCharset);
    EXPECT_EQ(d.label, "gbk");
}

TEST(DetectEncoding, BomTakesPrecedenceOverMeta) {
    const auto d = detect_encoding(kUtf8Bom + "<meta charset='gbk'><p>x</p>");
    EXPECT_EQ(d.source, EncodingSource::ByteOrderMark);
    EXPECT_EQ(d.label, "utf-8");
}

TEST(DetectEncoding, MetaTakesPrecedenceOverUtf8Heuristic) {
    // 纯 ASCII 正文（本身是合法 UTF-8），但显式声明 gbk —— 应以声明为准。
    const auto d = detect_encoding("<html><head><meta charset='gbk'></head><body>abc</body></html>");
    EXPECT_EQ(d.source, EncodingSource::MetaCharset);
    EXPECT_EQ(d.label, "gbk");
}

TEST(DetectEncoding, Utf8HeuristicForAsciiWithoutDeclaration) {
    const auto d = detect_encoding("<p>hello</p>");
    EXPECT_EQ(d.source, EncodingSource::Utf8Heuristic);
    EXPECT_EQ(d.label, "utf-8");
    EXPECT_TRUE(d.supported);
}

TEST(DetectEncoding, Utf8HeuristicForValidMultibyteUtf8) {
    const auto d = detect_encoding("<p>\xE4\xB8\xAD\xE6\x96\x87</p>");  // UTF-8 “中文”
    EXPECT_EQ(d.source, EncodingSource::Utf8Heuristic);
    EXPECT_EQ(d.label, "utf-8");
}

TEST(DetectEncoding, UndeclaredNonUtf8IsUnknown) {
    const auto d = detect_encoding(kGbkZhongWen);  // 无 BOM、无 meta、非合法 UTF-8
    EXPECT_FALSE(d.found());
    EXPECT_EQ(d.source, EncodingSource::Unknown);
    EXPECT_TRUE(d.label.empty());
}

TEST(DetectEncoding, MetaBeyondScanWindowIsIgnored) {
    // <meta> 出现在预扫描窗口（4096 字节）之外，应不被采用；正文全 ASCII → UTF-8 启发式。
    std::string bytes(4100, 'a');
    bytes += "<meta charset='gbk'><body>x</body>";
    const auto d = detect_encoding(bytes);
    EXPECT_EQ(d.source, EncodingSource::Utf8Heuristic);
    EXPECT_EQ(d.label, "utf-8");
}

// ==================================================================
// is_encoding_supported / supported_encodings
// ==================================================================

TEST(EncodingSupport, KnownEncodingsAreSupported) {
    for (const std::string_view label : {
             "utf-8", "UTF-8", "utf-16le", "utf-16be",
             "gbk", "GBK", "gb2312", "gb18030",
             "big5", "BIG5", "shift_jis", "sjis", "windows-31j",
             "windows-1252", "iso-8859-1", "latin1"}) {
        EXPECT_TRUE(is_encoding_supported(label)) << "label=" << label;
    }
}

TEST(EncodingSupport, UnknownEncodingsAreNotSupported) {
    for (const std::string_view label : {"euc-jp", "euc-kr", "koi8-r", "", "definitely-not-a-charset"}) {
        EXPECT_FALSE(is_encoding_supported(label)) << "label=" << label;
    }
}

TEST(EncodingSupport, SupportedEncodingsListIsCoherent) {
    const auto list = supported_encodings();
    EXPECT_FALSE(list.empty());

    // 列表内每一项都应被 is_encoding_supported 认可。
    for (const auto label : list) {
        EXPECT_TRUE(is_encoding_supported(label)) << "label=" << label;
    }

    const auto has = [list](std::string_view name) {
        return std::ranges::find(list, name) != list.end();
    };
    EXPECT_TRUE(has("utf-8"));
    EXPECT_TRUE(has("utf-16le"));
    EXPECT_TRUE(has("utf-16be"));
    EXPECT_TRUE(has("gbk"));
    EXPECT_TRUE(has("big5"));
    EXPECT_TRUE(has("shift_jis"));
    EXPECT_TRUE(has("windows-1252"));
}

// ==================================================================
// decode_to_utf8 —— 自动检测路径
// ==================================================================

TEST(DecodeToUtf8Auto, EmptyInputIsOk) {
    const auto r = decode_to_utf8("");
    EXPECT_TRUE(r.ok());
    EXPECT_TRUE(r.text.empty());
    EXPECT_EQ(r.encoding, "utf-8");
    EXPECT_EQ(r.source, EncodingSource::Utf8Heuristic);
}

TEST(DecodeToUtf8Auto, Utf8PlainPassThrough) {
    const auto r = decode_to_utf8("<div>x</div>");
    EXPECT_TRUE(r.ok());
    EXPECT_EQ(r.text, "<div>x</div>");
    EXPECT_EQ(r.encoding, "utf-8");
    EXPECT_EQ(r.source, EncodingSource::Utf8Heuristic);
}

TEST(DecodeToUtf8Auto, Utf8BomIsStripped) {
    const auto r = decode_to_utf8(kUtf8Bom + "<div>x</div>");
    EXPECT_TRUE(r.ok());
    EXPECT_EQ(r.text, "<div>x</div>");
    EXPECT_EQ(r.encoding, "utf-8");
    EXPECT_EQ(r.source, EncodingSource::ByteOrderMark);
}

TEST(DecodeToUtf8Auto, Utf16LeBom) {
    const auto r = decode_to_utf8(utf16le_with_bom(u"<div>Hello</div>"));
    EXPECT_TRUE(r.ok());
    EXPECT_EQ(r.text, "<div>Hello</div>");
    EXPECT_EQ(r.encoding, "utf-16le");
    EXPECT_EQ(r.source, EncodingSource::ByteOrderMark);
}

TEST(DecodeToUtf8Auto, Utf16BeBom) {
    const auto r = decode_to_utf8(utf16be_with_bom(u"<div>Hello</div>"));
    EXPECT_TRUE(r.ok());
    EXPECT_EQ(r.text, "<div>Hello</div>");
    EXPECT_EQ(r.encoding, "utf-16be");
    EXPECT_EQ(r.source, EncodingSource::ByteOrderMark);
}

TEST(DecodeToUtf8Auto, GbkViaMetaCharset) {
    std::string bytes = "<html><head><meta charset='gbk'></head><body>";
    bytes += kGbkZhongWen;
    bytes += "</body></html>";

    const auto r = decode_to_utf8(bytes);
    EXPECT_TRUE(r.ok());
    EXPECT_EQ(r.encoding, "gbk");
    EXPECT_EQ(r.source, EncodingSource::MetaCharset);
    EXPECT_TRUE(contains(r.text, "中文"));
}

TEST(DecodeToUtf8Auto, Big5ViaMetaCharset) {
    std::string bytes = "<meta charset='big5'><p>";
    bytes += kBig5Zhong;
    bytes += "</p>";

    const auto r = decode_to_utf8(bytes);
    EXPECT_TRUE(r.ok());
    EXPECT_EQ(r.encoding, "big5");
    EXPECT_TRUE(contains(r.text, "中"));
}

TEST(DecodeToUtf8Auto, ShiftJisViaMetaCharset) {
    std::string bytes = "<meta charset='shift_jis'><p>";
    bytes += kShiftJisNihon;
    bytes += "</p>";

    const auto r = decode_to_utf8(bytes);
    EXPECT_TRUE(r.ok());
    EXPECT_EQ(r.encoding, "shift_jis");
    EXPECT_TRUE(contains(r.text, "日本"));
}

TEST(DecodeToUtf8Auto, Windows1252ViaMetaCharset) {
    std::string bytes = "<meta charset='iso-8859-1'><p>";
    bytes.push_back(static_cast<char>(0x80));  // windows-1252 中的 €
    bytes += "</p>";

    const auto r = decode_to_utf8(bytes);
    EXPECT_TRUE(r.ok());
    EXPECT_EQ(r.encoding, "windows-1252");  // iso-8859-1 规范化为 windows-1252
    EXPECT_TRUE(contains(r.text, "€"));
}

TEST(DecodeToUtf8Auto, UndeclaredNonUtf8IsUnknownEncoding) {
    const auto r = decode_to_utf8(kGbkZhongWen);
    EXPECT_FALSE(r.ok());
    EXPECT_EQ(r.status, DecodeStatus::UnknownEncoding);
    EXPECT_TRUE(r.text.empty());
}

TEST(DecodeToUtf8Auto, DeclaredUnsupportedEncoding) {
    const auto r = decode_to_utf8("<meta charset='euc-jp'><p>x</p>");
    EXPECT_FALSE(r.ok());
    EXPECT_EQ(r.status, DecodeStatus::UnsupportedEncoding);
    EXPECT_EQ(r.encoding, "euc-jp");
}

TEST(DecodeToUtf8Auto, DeclaredUtf8ButInvalidBytes) {
    std::string bytes = "<meta charset='utf-8'>";
    bytes.push_back(static_cast<char>(0xFF));  // 非法 UTF-8 字节

    const auto r = decode_to_utf8(bytes);
    EXPECT_FALSE(r.ok());
    EXPECT_EQ(r.status, DecodeStatus::InvalidBytes);
    EXPECT_EQ(r.encoding, "utf-8");
}

// ==================================================================
// decode_to_utf8 —— 显式强制编码（override）
// ==================================================================

TEST(DecodeToUtf8Override, ForcedGbkBypassesDetection) {
    // 同样的裸字节在自动模式下是 UnknownEncoding；显式指定即可成功解码。
    const auto r = decode_to_utf8(kGbkZhongWen, "gbk");
    EXPECT_TRUE(r.ok());
    EXPECT_EQ(r.text, "中文");
    EXPECT_EQ(r.encoding, "gbk");
    EXPECT_EQ(r.source, EncodingSource::UserOverride);
}

TEST(DecodeToUtf8Override, LabelAliasAndCaseAreNormalized) {
    EXPECT_EQ(decode_to_utf8(kGbkZhongWen, "GB2312").text, "中文");
    EXPECT_EQ(decode_to_utf8(kGbkZhongWen, "GBK").text, "中文");
    EXPECT_EQ(decode_to_utf8(kGbkZhongWen, "gb18030").encoding, "gbk");
}

TEST(DecodeToUtf8Override, ForcedBig5) {
    const auto r = decode_to_utf8(kBig5Zhong, "big5");
    EXPECT_TRUE(r.ok());
    EXPECT_EQ(r.text, "中");
    EXPECT_EQ(r.source, EncodingSource::UserOverride);
}

TEST(DecodeToUtf8Override, ForcedShiftJis) {
    const auto r = decode_to_utf8(kShiftJisNihon, "windows-31j");
    EXPECT_TRUE(r.ok());
    EXPECT_EQ(r.text, "日本");
    EXPECT_EQ(r.encoding, "shift_jis");
}

TEST(DecodeToUtf8Override, ForcedUtf8StripsBom) {
    const auto r = decode_to_utf8(kUtf8Bom + "<x/>", "utf-8");
    EXPECT_TRUE(r.ok());
    EXPECT_EQ(r.text, "<x/>");
    EXPECT_EQ(r.source, EncodingSource::UserOverride);
}

TEST(DecodeToUtf8Override, ForcedUnsupportedEncoding) {
    const auto r = decode_to_utf8(kGbkZhongWen, "euc-kr");
    EXPECT_FALSE(r.ok());
    EXPECT_EQ(r.status, DecodeStatus::UnsupportedEncoding);
}

TEST(DecodeToUtf8Override, ForcedUtf8OnInvalidBytes) {
    const auto r = decode_to_utf8(std::string("\xFF\xFE\xFF", 3), "utf-8");
    EXPECT_FALSE(r.ok());
    EXPECT_EQ(r.status, DecodeStatus::InvalidBytes);
    EXPECT_EQ(r.encoding, "utf-8");
}

// ==================================================================
// UTF-16 解码边界
// ==================================================================

TEST(DecodeUtf16, SupplementaryPlaneSurrogatePair) {
    // U+1F600（😀）以 UTF-16LE 代理对表示，应正确合成为 4 字节 UTF-8。
    const auto r = decode_to_utf8(utf16le_with_bom(std::u16string{0xD83D, 0xDE00}));
    EXPECT_TRUE(r.ok());
    EXPECT_EQ(r.text, "\xF0\x9F\x98\x80");  // 😀
}

TEST(DecodeUtf16, UnpairedHighSurrogateBecomesReplacementChar) {
    // 高代理后跟普通字符 'A'：高代理→U+FFFD，'A' 正常输出。
    const auto r = decode_to_utf8(utf16le_with_bom(std::u16string{0xD83D, 0x0041}));
    EXPECT_TRUE(r.ok());
    EXPECT_EQ(r.text, "\xEF\xBF\xBD" "A");
}

TEST(DecodeUtf16, OddTrailingByteBecomesReplacementChar) {
    // BOM + 'A'(0x0041) + 悬挂的半个码元(0x42)。
    std::string bytes("\xFF\xFE", 2);
    bytes.append("\x41\x00", 2);
    bytes.push_back('\x42');

    const auto r = decode_to_utf8(bytes);
    EXPECT_TRUE(r.ok());
    EXPECT_EQ(r.encoding, "utf-16le");
    EXPECT_EQ(r.text, "A\xEF\xBF\xBD");
}

}  // namespace hps::tests
