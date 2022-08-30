// Copyright The Go Authors.

#include <string>
#include <string_view>
#include <tuple>
#include <vector>

#include <catch2/catch.hpp>

#include "bongo/bongo.h"
#include "bongo/fmt.h"
#include "bongo/strings.h"
#include "bongo/unicode/utf8.h"

namespace utf8 = bongo::unicode::utf8;
using namespace std::string_literals;
using namespace std::string_view_literals;

namespace bongo::fmt {

template <typename T>
auto check(std::string_view format, T&& v, std::string_view exp) -> void {
  auto s = sprintf(format, v);
  CAPTURE(format, v, s);
  auto i = strings::index(exp, "PTR");
  if (i != std::string_view::npos) {
    std::string_view pattern, chars;
    if (strings::has_prefix(std::string_view{exp}.substr(i), "PTR_b")) {
      pattern = "PTR_b";
      chars = "01";
    } else if (strings::has_prefix(std::string_view{exp}.substr(i), "PTR_o")) {
      pattern = "PTR_o";
      chars = "01234567";
    } else if (strings::has_prefix(std::string_view{exp}.substr(i), "PTR_d")) {
      pattern = "PTR_d";
      chars = "0123456789";
    } else if (strings::has_prefix(std::string_view{exp}.substr(i), "PTR_x")) {
      pattern = "PTR_x";
      chars = "0123456789abcdef";
    } else if (strings::has_prefix(std::string_view{exp}.substr(i), "PTR_X")) {
      pattern = "PTR_X";
      chars = "0123456789ABCDEF";
    } else {
      pattern = "PTR";
      chars = "0123456789abcdefABCDEF";
    }
    auto p = s.substr(0, i) + std::string{pattern};
    for (auto j = i; j < s.size(); ++j) {
      if (!strings::contains(chars, utf8::to_rune(s[j]))) {
        p += s.substr(j);
        break;
      }
    }
    s = p;
  }
  CHECK(s == exp);
}

auto zero_fill(std::string prefix, long width, std::string suffix) -> std::string {
  return prefix + strings::repeat("0", width-suffix.size()) + suffix;
}

struct I {
  long value;
  std::string str() const { return sprintf("<%d>", value); }
};

struct F {
  long value;
  template <State T>
  void format(T& state, rune c) const { fprintf(state, "<%c=F(%d)>", c, value); }
};

struct G {
  long value;
  std::string bongostr() const { return sprintf("BongoString(%d)", value); }
};

constexpr static auto nan = std::numeric_limits<double>::quiet_NaN();
constexpr static auto inf = std::numeric_limits<double>::infinity();

long intvar = 0;

TEST_CASE("Sprintf", "[fmt]") {
  check("%d", 12345, "12345");
  check("%v", 12345, "12345");
  check("%t", true, "true");

  // basic string
  check("%s", "abc", "abc");
  check("%q", "abc", R"("abc")");
  check("%x", "\xff\xf0\x0f\xff", "fff00fff");
  check("%X", "\xff\xf0\x0f\xff", "FFF00FFF");
  check("%x", "", "");
  check("% x", "", "");
  check("%#x", "", "");
  check("%# x", "", "");
  check("%x", "xyz", "78797a");
  check("%X", "xyz", "78797A");
  check("% x", "xyz", "78 79 7a");
  check("% X", "xyz", "78 79 7A");
  check("%#x", "xyz", "0x78797a");
  check("%#X", "xyz", "0X78797A");
  check("%# x", "xyz", "0x78 0x79 0x7a");
  check("%# X", "xyz", "0X78 0X79 0X7A");

  // basic bytes
  check("%s", std::vector<uint8_t>{'a', 'b', 'c'}, "abc");
  check("%s", std::array<uint8_t, 3>{'a', 'b', 'c'}, "abc");
  check("%q", bytes::to_bytes("abc"sv), R"("abc")");
  check("%x", bytes::to_bytes("abc"sv), "616263");
  check("%x", bytes::to_bytes("\xff\xf0\x0f\xff"sv), "fff00fff");
  check("%X", bytes::to_bytes("\xff\xf0\x0f\xff"sv), "FFF00FFF");
  check("%x", bytes::to_bytes(""sv), "");
  check("% x", bytes::to_bytes(""sv), "");
  check("%#x", bytes::to_bytes(""sv), "");
  check("%# x", bytes::to_bytes(""sv), "");
  check("%x", bytes::to_bytes("xyz"sv), "78797a");
  check("%X", bytes::to_bytes("xyz"sv), "78797A");
  check("% x", bytes::to_bytes("xyz"sv), "78 79 7a");
  check("% X", bytes::to_bytes("xyz"sv), "78 79 7A");
  check("%#x", bytes::to_bytes("xyz"sv), "0x78797a");
  check("%#X", bytes::to_bytes("xyz"sv), "0X78797A");
  check("%# x", bytes::to_bytes("xyz"sv), "0x78 0x79 0x7a");
  check("%# X", bytes::to_bytes("xyz"sv), "0X78 0X79 0X7A");

  // escaped strings
  check("%q", "", R"("")");
  check("%#q", "", R"-(R"()")-");
  check("%q", "\"", R"("\"")");
  check("%#q", "\"", R"-(R"(")")-");
  check("%q", ")\"", R"-(")\"")-");
  check("%#q", ")\"", R"-(")\"")-");
  check("%q", "\n", R"("\n")");
  check("%#q", "\n", R"("\n")");
  check("%q", R"(\n)", R"("\\n")");
  check("%#q", R"(\n)", R"-(R"(\n)")-");
  check("%q", "abc", R"("abc")");
  check("%#q", "abc", R"-(R"(abc)")-");
  check("%q", "日本語", R"("日本語")");
  check("%+q", "日本語", R"("\u65e5\u672c\u8a9e")");
  check("%#q", "日本語", R"-(R"(日本語)")-");
  check("%#+q", "日本語", R"-(R"(日本語)")-");
  check("%q", "\a\b\f\n\r\t\v\"\\", R"("\a\b\f\n\r\t\v\"\\")");
  check("%+q", "\a\b\f\n\r\t\v\"\\", R"("\a\b\f\n\r\t\v\"\\")");
  check("%#q", "\a\b\f\n\r\t\v\"\\", R"("\a\b\f\n\r\t\v\"\\")");
  check("%#+q", "\a\b\f\n\r\t\v\"\\", R"("\a\b\f\n\r\t\v\"\\")");
  check("%q", "☺", R"("☺")");
  check("% q", "☺", R"("☺")");
  check("%+q", "☺", R"("\u263a")");
  check("%#q", "☺", R"-(R"(☺)")-");
  check("%#+q", "☺", R"-(R"(☺)")-");
  check("%10q", "⌘", R"(       "⌘")");
  check("%+10q", "⌘", R"(  "\u2318")");
  check("%-10q", "⌘", R"("⌘"       )");
  check("%+-10q", "⌘", R"("\u2318"  )");
  check("%010q", "⌘", R"(0000000"⌘")");
  check("%+010q", "⌘", R"(00"\u2318")");
  check("%-010q", "⌘", R"("⌘"       )");
  check("%+-010q", "⌘", R"("\u2318"  )");
  check("%#8q", "\n", R"(    "\n")");
  check("%#+8q", "\r", R"(    "\r")");
  check("%#-8q", "\t", R"-(R"(	)"  )-");
  check("%#+-8q", "\b", R"("\b"    )");
  check("%q", "abc\xfd""def", R"("abc\xfd""def")");
  check("%+q", "abc\xfd""def", R"("abc\xfd""def")");
  check("%#q", "abc\xfd""def", R"("abc\xfd""def")");
  check("%#+q", "abc\xfd""def", R"("abc\xfd""def")");
  // Runes that are not printable
  check("%q", "\U0010ffff", R"("\U0010ffff")");
  check("%+q", "\U0010ffff", R"("\U0010ffff")");
  check("%#q", "\U0010ffff", R"-(R"(􏿿)")-");
  check("%#+q", "\U0010ffff", R"-(R"(􏿿)")-");
  // Runes that are not valid
  check("%q", utf8::encode(0x110000), R"("�")");
  check("%+q", utf8::encode(0x110000), R"("\ufffd")");
  check("%#q", utf8::encode(0x110000), R"-(R"(�)")-");
  check("%#+q", utf8::encode(0x110000), R"-(R"(�)")-");

  // characters
  check("%c", static_cast<unsigned>('x'), "x");
  check("%c", 0xe4, "ä");
  check("%c", 0x672c, "本");
  check("%c", u'日', "日");
  check("%.0c", u'⌘', "⌘"); // Specifying precision should have no effect.
  check("%3c", u'⌘', "  ⌘");
  check("%-3c", u'⌘', "⌘  ");
  // Runes that are not printable.
  check("%c", U'\U00000e00', "\u0e00");
  check("%c", U'\U0010ffff', "\U0010ffff");
  // Runes that are not valid.
  check("%c", -1, "�");
  check("%c", 0xDC80, "�");
  check("%c", static_cast<rune>(0x110000), "�");
  check("%c", static_cast<int64_t>(0xFFFFFFFFF), "�");
  check("%c", static_cast<uint64_t>(0xFFFFFFFFF), "�");

  // escaped characters
  check("%q", static_cast<unsigned>(0), R"('\x00')");
  check("%+q", static_cast<unsigned>(0), R"('\x00')");
  check("%q", '"', R"('"')");
  check("%+q", '"', R"('"')");
  check("%q", '\'', R"('\'')");
  check("%+q", '\'', R"('\'')");
  check("%q", '`', "'`'");
  check("%+q", '`', "'`'");
  check("%q", 'x', R"('x')");
  check("%+q", 'x', R"('x')");
  check("%q", u'ÿ', R"(u'ÿ')");
  check("%+q", u'ÿ', R"(u'\u00ff')");
  check("%q", '\n', R"('\n')");
  check("%+q", '\n', R"('\n')");
  check("%q", u'☺', R"(u'☺')");
  check("%+q", u'☺', R"(u'\u263a')");
  check("% q", u'☺', R"(u'☺')");  // The space modifier should have no effect.
  check("%.0q", u'☺', R"(u'☺')"); // Specifying precision should have no effect.
  check("%10q", u'⌘', R"(      u'⌘')");
  check("%+10q", u'⌘', R"( u'\u2318')");
  check("%-10q", u'⌘', R"(u'⌘'      )");
  check("%+-10q", u'⌘', R"(u'\u2318' )");
  check("%010q", u'⌘', R"(000000u'⌘')");
  check("%+010q", u'⌘', R"(0u'\u2318')");
  check("%-010q", u'⌘', R"(u'⌘'      )"); // 0 has no effect when - is present.
  check("%+-010q", u'⌘', R"(u'\u2318' )");
  // Runes that are not printable.
  check("%q", U'\U00000e00', R"(u'\u0e00')");
  check("%q", U'\U0010ffff', R"(U'\U0010ffff')");
  // Runes that are not valid.
  check("%q", static_cast<int32_t>(-1), R"(u'�')");
  check("%q", 0xDC80, R"(u'�')");
  check("%q", static_cast<rune>(0x110000), R"(u'�')");
  check("%q", static_cast<int64_t>(0xFFFFFFFFF), R"(u'�')");
  check("%q", static_cast<uint64_t>(0xFFFFFFFFF), R"(u'�')");

  // width
  check("%5s", "abc", "  abc");
  check("%5s", bytes::to_bytes("abc"sv), "  abc");
  check("%2s", "\u263a", " ☺");
  check("%2s", bytes::to_bytes("\u263a"sv), " ☺");
  check("%-5s", "abc", "abc  ");
  check("%-5s", bytes::to_bytes("abc"sv), "abc  ");
  check("%05s", "abc", "00abc");
  check("%05s", bytes::to_bytes("abc"sv), "00abc");
  check("%5s", "abcdefghijklmnopqrstuvwxyz", "abcdefghijklmnopqrstuvwxyz");
  check("%5s", bytes::to_bytes("abcdefghijklmnopqrstuvwxyz"sv), "abcdefghijklmnopqrstuvwxyz");
  check("%.5s", "abcdefghijklmnopqrstuvwxyz", "abcde");
  check("%.5s", bytes::to_bytes("abcdefghijklmnopqrstuvwxyz"sv), "abcde");
  check("%.0s", "日本語日本語", "");
  check("%.0s", bytes::to_bytes("日本語日本語"sv), "");
  check("%.5s", "日本語日本語", "日本語日本");
  check("%.5s", bytes::to_bytes("日本語日本語"sv), "日本語日本");
  check("%.10s", "日本語日本語", "日本語日本語"sv);
  check("%.10s", bytes::to_bytes("日本語日本語"sv), "日本語日本語");
  check("%08q", "abc", R"(000"abc")");
  check("%08q", bytes::to_bytes("abc"sv), R"(000"abc")");
  check("%-8q", "abc", R"("abc"   )");
  check("%-8q", bytes::to_bytes("abc"sv), R"("abc"   )");
  check("%.5q", "abcdefghijklmnopqrstuvwxyz", R"("abcde")");
  check("%.5q", bytes::to_bytes("abcdefghijklmnopqrstuvwxyz"sv), R"("abcde")");
  check("%.5x", "abcdefghijklmnopqrstuvwxyz", "6162636465");
  check("%.5x", bytes::to_bytes("abcdefghijklmnopqrstuvwxyz"sv), "6162636465");
  check("%.3q", "日本語日本語", R"("日本語")");
  check("%.3q", bytes::to_bytes("日本語日本語"sv), R"("日本語")");
  check("%.1q", "日本語", R"("日")");
  check("%.1q", bytes::to_bytes("日本語"sv), R"("日")");
  check("%.1x", "日本語", "e6");
  check("%.1X", bytes::to_bytes("日本語"sv), "E6");
  check("%10.1q", "日本語日本語", R"(       "日")");
  check("%10.1q", bytes::to_bytes("日本語日本語"), R"(       "日")");
  check("%10v", nullptr, "     <nil>");
  check("%-10v", nullptr, "<nil>     ");

  // integers
  check("%d", 12345u, "12345");
  check("%d", -12345, "-12345");
  check("%d", std::numeric_limits<uint8_t>::max(), "255");
  check("%d", std::numeric_limits<uint16_t>::max(), "65535");
  check("%d", std::numeric_limits<uint32_t>::max(), "4294967295");
  check("%d", std::numeric_limits<uint64_t>::max(), "18446744073709551615");
  check("%d", std::numeric_limits<int8_t>::min(), "-128");
  check("%d", std::numeric_limits<int16_t>::min(), "-32768");
  check("%d", std::numeric_limits<int32_t>::min(), "-2147483648");
  check("%d", std::numeric_limits<int64_t>::min(), "-9223372036854775808");
  check("%.d", 0, "");
  check("%.0d", 0, "");
  check("%6.0d", 0, "      ");
  check("%06.0d", 0, "      ");
  check("% d", 12345, " 12345");
  check("%+d", 12345, "+12345");
  check("%+d", -12345, "-12345");
  check("%b", 7, "111");
  check("%b", -6, "-110");
  check("%#b", 7, "0b111");
  check("%#b", -6, "-0b110");
  check("%b", ~static_cast<uint32_t>(0), "11111111111111111111111111111111");
  check("%b", ~static_cast<uint64_t>(0), "1111111111111111111111111111111111111111111111111111111111111111");
  check("%o", 01234, "1234");
  check("%o", -01234, "-1234");
  check("%#o", 01234, "01234");
  check("%#o", -01234, "-01234");
  check("%O", 01234, "0o1234");
  check("%O", -01234, "-0o1234");
  check("%o", ~static_cast<uint32_t>(0), "37777777777");
  check("%o", ~static_cast<uint64_t>(0), "1777777777777777777777");
  check("%#X", 0, "0X0");
  check("%x", 0x12abcdef, "12abcdef");
  check("%X", 0x12abcdef, "12ABCDEF");
  check("%x", ~static_cast<uint32_t>(0), "ffffffff");
  check("%X", ~static_cast<uint64_t>(0), "FFFFFFFFFFFFFFFF");
  check("%.20b", 7, "00000000000000000111");
  check("%10d", 12345, "     12345");
  check("%10d", -12345, "    -12345");
  check("%+10d", 12345, "    +12345");
  check("%010d", 12345, "0000012345");
  check("%010d", -12345, "-000012345");
  check("%20.8d", 1234, "            00001234");
  check("%20.8d", -1234, "           -00001234");
  check("%020.8d", 1234, "            00001234");
  check("%020.8d", -1234, "           -00001234");
  check("%-20.8d", 1234, "00001234            ");
  check("%-20.8d", -1234, "-00001234           ");
  check("%-#20.8x", 0x1234abc, "0x01234abc          ");
  check("%-#20.8X", 0x1234abc, "0X01234ABC          ");
  check("%-#20.8o", 01234, "00001234            ");

  // Test correct detail::fmt::intbuf overflow checks.
  check("%068d", 1, zero_fill("", 68, "1"));
  check("%068d", -1, zero_fill("-", 67, "1"));
  check("%#.68x", 42, zero_fill("0x", 68, "2a"));
  check("%.68d", -42, zero_fill("-", 68, "42"));
  check("%+.68d", 42, zero_fill("+", 68, "42"));
  check("% .68d", 42, zero_fill(" ", 68, "42"));
  check("% +.68d", 42, zero_fill("+", 68, "42"));

  // unicode format
  check("%U", 0, "U+0000");
  check("%U", -1, "U+FFFFFFFFFFFFFFFF");
  check("%U", '\n', "U+000A");
  check("%#U", '\n', "U+000A");
  check("%+U", 'x', "U+0078");       // Plus flag should have no effect.
  check("%# U", 'x', "U+0078 'x'");  // Space flag should have no effect.
  check("%#.2U", 'x', "U+0078 'x'"); // Precisions below 4 should print 4 digits.
  check("%U", u'\u263a', "U+263A");
  check("%#U", u'\u263a', "U+263A '☺'");
  check("%U", U'\U0001D6C2', "U+1D6C2");
  check("%#U", U'\U0001D6C2', "U+1D6C2 '𝛂'");
  check("%#14.6U", u'⌘', "  U+002318 '⌘'");
  check("%#-14.6U", u'⌘', "U+002318 '⌘'  ");
  check("%#014.6U", u'⌘', "  U+002318 '⌘'");
  check("%#-014.6U", u'⌘', "U+002318 '⌘'  ");
  check("%.68U", static_cast<unsigned>(42), zero_fill("U+", 68, "2A"));
  check("%#.68U", u'日', zero_fill("U+", 68, "65E5") + " '日'");

  // floats
  check("%+.3e", 0.0, "+0.000e+00");
  check("%+.3e", 1.0, "+1.000e+00");
  check("%+.3x", 0.0, "+0x0.000p+00");
  check("%+.3x", 1.0, "+0x1.000p+00");
  check("%+.3f", -1.0, "-1.000");
  check("%+.3F", -1.0, "-1.000");
  check("%+.3F", static_cast<float>(-1.0), "-1.000");
  check("%+07.2f", 1.0, "+001.00");
  check("%+07.2f", -1.0, "-001.00");
  check("%-07.2f", 1.0, "1.00   ");
  check("%-07.2f", -1.0, "-1.00  ");
  check("%+-07.2f", 1.0, "+1.00  ");
  check("%+-07.2f", -1.0, "-1.00  ");
  check("%-+07.2f", 1.0, "+1.00  ");
  check("%-+07.2f", -1.0, "-1.00  ");
  check("%+10.2f", +1.0, "     +1.00");
  check("%+10.2f", -1.0, "     -1.00");
  check("% .3E", -1.0, "-1.000E+00");
  check("% .3e", 1.0, " 1.000e+00");
  check("% .3X", -1.0, "-0X1.000P+00");
  check("% .3x", 1.0, " 0x1.000p+00");
  check("%+.3g", 0.0, "+0");
  check("%+.3g", 1.0, "+1");
  check("%+.3g", -1.0, "-1");
  check("% .3g", -1.0, "-1");
  check("% .3g", 1.0, " 1");
  check("%b", static_cast<float>(1.0), "8388608p-23");
  check("%b", 1.0, "4503599627370496p-52");
  // Test sharp flag used with floats.
  check("%#g", 1e-323, "1.00000e-323");
  check("%#g", -1.0, "-1.00000");
  check("%#g", 1.1, "1.10000");
  check("%#g", 123456.0, "123456.");
  check("%#g", 1234567.0, "1.234567e+06");
  check("%#g", 1230000.0, "1.23000e+06");
  check("%#g", 1000000.0, "1.00000e+06");
  check("%#.0f", 1.0, "1.");
  check("%#.0e", 1.0, "1.e+00");
  check("%#.0x", 1.0, "0x1.p+00");
  check("%#.0g", 1.0, "1.");
  check("%#.0g", 1100000.0, "1.e+06");
  check("%#.4f", 1.0, "1.0000");
  check("%#.4e", 1.0, "1.0000e+00");
  check("%#.4x", 1.0, "0x1.0000p+00");
  check("%#.4g", 1.0, "1.000");
  check("%#.4g", 100000.0, "1.000e+05");
  check("%#.4g", 1.234, "1.234");
  check("%#.4g", 0.1234, "0.1234");
  check("%#.4g", 1.23, "1.230");
  check("%#.4g", 0.123, "0.1230");
  check("%#.4g", 1.2, "1.200");
  check("%#.4g", 0.12, "0.1200");
  check("%#.4g", 10.2, "10.20");
  check("%#.4g", 0.0, "0.000");
  check("%#.4g", 0.012, "0.01200");
  check("%#.0f", 123.0, "123.");
  check("%#.0e", 123.0, "1.e+02");
  check("%#.0x", 123.0, "0x1.p+07");
  check("%#.0g", 123.0, "1.e+02");
  check("%#.4f", 123.0, "123.0000");
  check("%#.4e", 123.0, "1.2300e+02");
  check("%#.4x", 123.0, "0x1.ec00p+06");
  check("%#.4g", 123.0, "123.0");
  check("%#.4g", 123000.0, "1.230e+05");
  check("%#9.4g", 1.0, "    1.000");
  // The sharp flag has no effect for binary float format.
  check("%#b", 1.0, "4503599627370496p-52");
  // Precision has no effect for binary float format.
  check("%.4b", static_cast<float>(1.0), "8388608p-23");
  check("%.4b", -1.0, "-4503599627370496p-52");
  // Test correct f.intbuf boundary checks.
  check("%.68f", 1.0, zero_fill("1.", 68, ""));
  check("%.68f", -1.0, zero_fill("-1.", 68, ""));
  // float infinites and NaNs
  check("%f", inf, "+Inf");
  check("%.1f", -inf, "-Inf");
  check("% f", nan, " NaN");
  check("%20f", inf, "                +Inf");
  check("% 20F", inf, "                 Inf");
  check("% 20e", -inf, "                -Inf");
  check("% 20x", -inf, "                -Inf");
  check("%+20E", -inf, "                -Inf");
  check("%+20X", -inf, "                -Inf");
  check("% +20g", -inf, "                -Inf");
  check("%+-20G", inf, "+Inf                ");
  check("%20e", nan, "                 NaN");
  check("%20x", nan, "                 NaN");
  check("% +20E", nan, "                +NaN");
  check("% +20X", nan, "                +NaN");
  check("% -20g", nan, " NaN                ");
  check("%+-20G", nan, "+NaN                ");
  // Zero padding does not apply to infinities and NaN.
  check("%+020e", inf, "                +Inf");
  check("%+020x", inf, "                +Inf");
  check("%-020f", -inf, "-Inf                ");
  check("%-020E", nan, "NaN                 ");
  check("%-020X", nan, "NaN                 ");

  // complex values
  check("%.f", std::complex<double>{0}, "(0+0i)");
  check("% .f", std::complex<double>{0, 0}, "( 0+0i)");
  check("%+.f", std::complex<double>{0, 0}, "(+0+0i)");
  check("% +.f", std::complex<double>{0, 0}, "(+0+0i)");
  check("%+.3e", std::complex<double>{0, 0}, "(+0.000e+00+0.000e+00i)");
  check("%+.3x", std::complex<double>{0, 0}, "(+0x0.000p+00+0x0.000p+00i)");
  check("%+.3f", std::complex<double>{0, 0}, "(+0.000+0.000i)");
  check("%+.3g", std::complex<double>{0, 0}, "(+0+0i)");
  check("%+.3e", std::complex<double>{1, 2}, "(+1.000e+00+2.000e+00i)");
  check("%+.3x", std::complex<double>{1, 2}, "(+0x1.000p+00+0x1.000p+01i)");
  check("%+.3f", std::complex<double>{1, 2}, "(+1.000+2.000i)");
  check("%+.3g", std::complex<double>{1, 2}, "(+1+2i)");
  check("%.3e", std::complex<double>{0, 0}, "(0.000e+00+0.000e+00i)");
  check("%.3x", std::complex<double>{0, 0}, "(0x0.000p+00+0x0.000p+00i)");
  check("%.3f", std::complex<double>{0, 0}, "(0.000+0.000i)");
  check("%.3F", std::complex<double>{0, 0}, "(0.000+0.000i)");
  check("%.3F", std::complex<float>{0, 0}, "(0.000+0.000i)");
  check("%.3g", std::complex<double>{0, 0}, "(0+0i)");
  check("%.3e", std::complex<double>{1, 2}, "(1.000e+00+2.000e+00i)");
  check("%.3x", std::complex<double>{1, 2}, "(0x1.000p+00+0x1.000p+01i)");
  check("%.3f", std::complex<double>{1, 2}, "(1.000+2.000i)");
  check("%.3g", std::complex<double>{1, 2}, "(1+2i)");
  check("%.3e", std::complex<double>{-1, -2}, "(-1.000e+00-2.000e+00i)");
  check("%.3x", std::complex<double>{-1, -2}, "(-0x1.000p+00-0x1.000p+01i)");
  check("%.3f", std::complex<double>{-1, -2}, "(-1.000-2.000i)");
  check("%.3g", std::complex<double>{-1, -2}, "(-1-2i)");
  check("% .3E", std::complex<double>{-1, -2}, "(-1.000E+00-2.000E+00i)");
  check("% .3X", std::complex<double>{-1, -2}, "(-0X1.000P+00-0X1.000P+01i)");
  check("%+.3g", std::complex<double>{1, 2}, "(+1+2i)");
  check("%+.3g", std::complex<float>{1, 2}, "(+1+2i)");
  check("%#g", std::complex<double>{1, 2}, "(1.00000+2.00000i)");
  check("%#g", std::complex<double>{123456, 789012}, "(123456.+789012.i)");
  check("%#g", std::complex<double>{0, 1e-10}, "(0.00000+1.00000e-10i)");
  check("%#g", std::complex<double>{-1e10, -1.11e100}, "(-1.00000e+10-1.11000e+100i)");
  check("%#.0f", std::complex<double>{1.23, 1.0}, "(1.+1.i)");
  check("%#.0e", std::complex<double>{1.23, 1.0}, "(1.e+00+1.e+00i)");
  check("%#.0x", std::complex<double>{1.23, 1.0}, "(0x1.p+00+0x1.p+00i)");
  check("%#.0g", std::complex<double>{1.23, 1.0}, "(1.+1.i)");
  check("%#.0g", std::complex<double>{0, 100000}, "(0.+1.e+05i)");
  check("%#.0g", std::complex<double>{1230000, 0}, "(1.e+06+0.i)");
  check("%#.4f", std::complex<double>{1, 1.23}, "(1.0000+1.2300i)");
  check("%#.4e", std::complex<double>{123, 1}, "(1.2300e+02+1.0000e+00i)");
  check("%#.4x", std::complex<double>{123, 1}, "(0x1.ec00p+06+0x1.0000p+00i)");
  check("%#.4g", std::complex<double>{123, 1.23}, "(123.0+1.230i)");
  check("%#12.5g", std::complex<double>{0, 100000}, "(      0.0000 +1.0000e+05i)");
  check("%#12.5g", std::complex<double>{1230000, -0}, "(  1.2300e+06     +0.0000i)");
  check("%b", std::complex<double>{1, 2}, "(4503599627370496p-52+4503599627370496p-51i)");
  check("%b", std::complex<float>{1, 2}, "(8388608p-23+8388608p-22i)");
  // The sharp flag has no effect for binary complex format.
  check("%#b", std::complex<double>{1, 2}, "(4503599627370496p-52+4503599627370496p-51i)");
  // Precision has no effect for binary complex format.
  check("%.4b", std::complex<double>{1, 2}, "(4503599627370496p-52+4503599627370496p-51i)");
  check("%.4b", std::complex<float>{1, 2}, "(8388608p-23+8388608p-22i)");
  // complex infinites and NaNs
  check("%f", std::complex<double>{inf, inf}, "(+Inf+Infi)");
  check("%f", std::complex<double>{-inf, -inf}, "(-Inf-Infi)");
  check("%f", std::complex<double>{nan, nan}, "(NaN+NaNi)");
  check("%.1f", std::complex<double>{inf, inf}, "(+Inf+Infi)");
  check("% f", std::complex<double>{inf, inf}, "( Inf+Infi)");
  check("% f", std::complex<double>{-inf, -inf}, "(-Inf-Infi)");
  check("% f", std::complex<double>{nan, nan}, "( NaN+NaNi)");
  check("%8e", std::complex<double>{inf, inf}, "(    +Inf    +Infi)");
  check("%8x", std::complex<double>{inf, inf}, "(    +Inf    +Infi)");
  check("% 8E", std::complex<double>{inf, inf}, "(     Inf    +Infi)");
  check("% 8X", std::complex<double>{inf, inf}, "(     Inf    +Infi)");
  check("%+8f", std::complex<double>{-inf, -inf}, "(    -Inf    -Infi)");
  check("% +8g", std::complex<double>{-inf, -inf}, "(    -Inf    -Infi)");
  check("% -8G", std::complex<double>{nan, nan}, "( NaN    +NaN    i)");
  check("%+-8b", std::complex<double>{nan, nan}, "(+NaN    +NaN    i)");
  // Zero padding does not apply to infinities and NaN.
  check("%08f", std::complex<double>{inf, inf}, "(    +Inf    +Infi)");
  check("%-08g", std::complex<double>{-inf, -inf}, "(-Inf    -Inf    i)");
  check("%-08G", std::complex<double>{nan, nan}, "(NaN     +NaN    i)");

  // old test/fmt_test.go
  check("%e", 1.0, "1.000000e+00");
  check("%e", 1234.5678e3, "1.234568e+06");
  check("%e", 1234.5678e-8, "1.234568e-05");
  check("%e", -7.0, "-7.000000e+00");
  check("%e", -1e-9, "-1.000000e-09");
  check("%f", 1234.5678e3, "1234567.800000");
  check("%f", 1234.5678e-8, "0.000012");
  check("%f", -7.0, "-7.000000");
  check("%f", -1e-9, "-0.000000");
  check("%g", 1234.5678e3, "1.2345678e+06");
  check("%g", static_cast<float>(1234.5678e3), "1.2345678e+06");
  check("%g", 1234.5678e-8, "1.2345678e-05");
  check("%g", -7.0, "-7");
  check("%g", -1e-9, "-1e-09");
  check("%g", static_cast<float>(-1e-9), "-1e-09");
  check("%E", 1.0, "1.000000E+00");
  check("%E", 1234.5678e3, "1.234568E+06");
  check("%E", 1234.5678e-8, "1.234568E-05");
  check("%E", -7.0, "-7.000000E+00");
  check("%E", -1e-9, "-1.000000E-09");
  check("%G", 1234.5678e3, "1.2345678E+06");
  check("%G", static_cast<float>(1234.5678e3), "1.2345678E+06");
  check("%G", 1234.5678e-8, "1.2345678E-05");
  check("%G", -7.0, "-7");
  check("%G", -1e-9, "-1E-09");
  check("%G", static_cast<float>(-1e-9), "-1E-09");
  check("%20.5s", "qwertyuiop", "               qwert");
  check("%.5s", "qwertyuiop", "qwert");
  check("%-20.5s", "qwertyuiop", "qwert               ");
  check("%20c", 'x', "                   x");
  check("%-20c", 'x', "x                   ");
  check("%20.6e", 1.2345e3, "        1.234500e+03");
  check("%20.6e", 1.2345e-3, "        1.234500e-03");
  check("%20e", 1.2345e3, "        1.234500e+03");
  check("%20e", 1.2345e-3, "        1.234500e-03");
  check("%20.8e", 1.2345e3, "      1.23450000e+03");
  check("%20f", 1.23456789e3, "         1234.567890");
  check("%20f", 1.23456789e-3, "            0.001235");
  check("%20f", 12345678901.23456789, "  12345678901.234568");
  check("%-20f", 1.23456789e3, "1234.567890         ");
  check("%20.8f", 1.23456789e3, "       1234.56789000");
  check("%20.8f", 1.23456789e-3, "          0.00123457");
  check("%g", 1.23456789e3, "1234.56789");
  check("%g", 1.23456789e-3, "0.00123456789");
  check("%g", 1.23456789e20, "1.23456789e+20");

  // spans
  check("%v", std::array<long, 5>{1, 2, 3, 4, 5}, "[1 2 3 4 5]");
  check("%v", std::vector<long>{1, 2, 3, 4, 5}, "[1 2 3 4 5]");

  // byte spans with %b,%c,%d,%o,%U and %v
  check("%b", std::array<char, 3>{65, 66, 67}, "[1000001 1000010 1000011]");
  check("%c", std::array<char, 3>{65, 66, 67}, "[A B C]");
  check("%d", std::array<char, 3>{65, 66, 67}, "[65 66 67]");
  check("%o", std::array<char, 3>{65, 66, 67}, "[101 102 103]");
  check("%U", std::array<char, 3>{65, 66, 67}, "[U+0041 U+0042 U+0043]");
  check("%v", std::array<char, 3>{65, 66, 67}, "[65 66 67]");
  check("%v", std::array<char, 1>{123}, "[123]");
  check("%012v", std::array<char, 0>{}, "[]");
  check("%6v", std::array<char, 3>{1, 11, 111}, "[     1     11    111]");
  check("%06v", std::array<char, 3>{1, 11, 111}, "[000001 000011 000111]");
  check("%-6v", std::array<char, 3>{1, 11, 111}, "[1      11     111   ]");
  check("%-06v", std::array<char, 3>{1, 11, 111}, "[1      11     111   ]");
  // f.space should and f.plus should not have an effect with %v.
  check("% v", std::array<char, 3>{1, 11, 111}, "[ 1  11  111]");
  check("%+v", std::array<char, 3>{1, 11, 111}, "[1 11 111]");
  // f.space and f.plus should have an effect with %d.
  check("% d", std::array<char, 3>{1, 11, 111}, "[ 1  11  111]");
  check("%+d", std::array<char, 3>{1, 11, 111}, "[+1 +11 +111]");
  check("%# -6d", std::array<char, 3>{1, 11, 111}, "[ 1      11     111  ]");
  check("%#+-6d", std::array<char, 3>{1, 11, 111}, "[+1     +11    +111  ]");

  // floats with %v
  check("%v", 1.2345678, "1.2345678");
  check("%v", static_cast<float>(1.2345678), "1.2345678");

  // complexes with %v
  check("%v", std::complex<float>{1, 2}, "(1+2i)");
  check("%v", std::complex<double>{1, 2}, "(1+2i)");

  // Stringer
  check("%s", I{23}, "<23>");
  check("%q", I{23}, R"("<23>")");
  check("%x", I{23}, "3c32333e");
  check("%#x", I{23}, "0x3c32333e");
  check("%# x", I{23}, "0x3c 0x32 0x33 0x3e");

  // Whole number floats are printed without decimals.
  check("%#v", 1.0, "1");
  check("%#v", 1000000.0, "1e+06");
  check("%#v", static_cast<float>(1.0), "1");
  check("%#v", static_cast<float>(1000000.0), "1e+06");

  // slices/maps with other formats
  check("%#x", std::array<long, 3>{1, 2, 15}, "[0x1 0x2 0xf]");
  check("%x", std::array<long, 3>{1, 2, 15}, "[1 2 f]");
  check("%d", std::array<long, 3>{1, 2, 15}, "[1 2 15]");
  check("%d", std::array<uint8_t, 3>{1, 2, 15}, "[1 2 15]");
  check("%q", std::array<std::string_view, 2>{"a", "b"}, R"(["a" "b"])");
  check("% 02x", std::array<uint8_t, 1>{1}, "01");
  check("% 02x", std::array<uint8_t, 3>{1, 2, 3}, "01 02 03");
  check("%v", std::map<long, long>{{1, 2}, {3, 4}}, "[1:2 3:4]");
  check("%v", std::make_tuple(1, 2, 3, 4), "[1 2 3 4]");

  // Padding with byte slices.
  check("%2x", std::array<uint8_t, 0>{}, "  ");
  check("%#2x", std::array<uint8_t, 0>{}, "  ");
  check("% 02x", std::array<uint8_t, 0>{}, "00");
  check("%# 02x", std::array<uint8_t, 0>{}, "00");
  check("%-2x", std::array<uint8_t, 0>{}, "  ");
  check("%-02x", std::array<uint8_t, 0>{}, "  ");
  check("%8x", std::array<uint8_t, 1>{0xab}, "      ab");
  check("% 8x", std::array<uint8_t, 1>{0xab}, "      ab");
  check("%#8x", std::array<uint8_t, 1>{0xab}, "    0xab");
  check("%# 8x", std::array<uint8_t, 1>{0xab}, "    0xab");
  check("%08x", std::array<uint8_t, 1>{0xab}, "000000ab");
  check("% 08x", std::array<uint8_t, 1>{0xab}, "000000ab");
  check("%#08x", std::array<uint8_t, 1>{0xab}, "00000xab");
  check("%# 08x", std::array<uint8_t, 1>{0xab}, "00000xab");
  check("%10x", std::array<uint8_t, 2>{0xab, 0xcd}, "      abcd");
  check("% 10x", std::array<uint8_t, 2>{0xab, 0xcd}, "     ab cd");
  check("%#10x", std::array<uint8_t, 2>{0xab, 0xcd}, "    0xabcd");
  check("%# 10x", std::array<uint8_t, 2>{0xab, 0xcd}, " 0xab 0xcd");
  check("%010x", std::array<uint8_t, 2>{0xab, 0xcd}, "000000abcd");
  check("% 010x", std::array<uint8_t, 2>{0xab, 0xcd}, "00000ab cd");
  check("%#010x", std::array<uint8_t, 2>{0xab, 0xcd}, "00000xabcd");
  check("%# 010x", std::array<uint8_t, 2>{0xab, 0xcd}, "00xab 0xcd");
  check("%-10X", std::array<uint8_t, 1>{0xab}, "AB        ");
  check("% -010X", std::array<uint8_t, 1>{0xab}, "AB        ");
  check("%#-10X", std::array<uint8_t, 2>{0xab, 0xcd}, "0XABCD    ");
  check("%# -010X", std::array<uint8_t, 2>{0xab, 0xcd}, "0XAB 0XCD ");
  // Same for strings
  check("%2x", "", "  ");
  check("%#2x", "", "  ");
  check("% 02x", "", "00");
  check("%# 02x", "", "00");
  check("%-2x", "", "  ");
  check("%-02x", "", "  ");
  check("%8x", "\xab", "      ab");
  check("% 8x", "\xab", "      ab");
  check("%#8x", "\xab", "    0xab");
  check("%# 8x", "\xab", "    0xab");
  check("%08x", "\xab", "000000ab");
  check("% 08x", "\xab", "000000ab");
  check("%#08x", "\xab", "00000xab");
  check("%# 08x", "\xab", "00000xab");
  check("%10x", "\xab\xcd", "      abcd");
  check("% 10x", "\xab\xcd", "     ab cd");
  check("%#10x", "\xab\xcd", "    0xabcd");
  check("%# 10x", "\xab\xcd", " 0xab 0xcd");
  check("%010x", "\xab\xcd", "000000abcd");
  check("% 010x", "\xab\xcd", "00000ab cd");
  check("%#010x", "\xab\xcd", "00000xabcd");
  check("%# 010x", "\xab\xcd", "00xab 0xcd");
  check("%-10X", "\xab", "AB        ");
  check("% -010X", "\xab", "AB        ");
  check("%#-10X", "\xab\xcd", "0XABCD    ");
  check("%# -010X", "\xab\xcd", "0XAB 0XCD ");

  // Formatter
  static_assert(Formatter<F, detail::printer>);
  check("%x", F{1}, "<x=F(1)>");

  // BongoStringer
  check("%#v", G{6}, "BongoString(6)");

  // %p with pointers
  check("%p", static_cast<long*>(nullptr), "0x0");
  check("%#p", static_cast<long*>(nullptr), "0");
  check("%p", &intvar, "0xPTR");
  check("%#p", &intvar, "PTR");
  check("%8.2p", static_cast<long*>(nullptr), "    0x00");
  check("%-20.16p", &intvar, "0xPTR  ");
  check("%p", 27, "%!p(int=27)");  // not a pointer at all
  // pointers with specified base
  check("%b", &intvar, "PTR_b");
  check("%d", &intvar, "PTR_d");
  check("%o", &intvar, "PTR_o");
  check("%x", &intvar, "PTR_x");
  check("%X", &intvar, "PTR_X");
  // %v on pointers
  check("%v", nullptr, "<nil>");
  check("%#v", nullptr, "<nil>");
  check("%v", static_cast<long*>(nullptr), "<nil>");
  check("%#v", static_cast<long*>(nullptr), "(long*)(nil)");
  check("%v", &intvar, "0xPTR");
  check("%#v", &intvar, "(long*)(0xPTR)");
  check("%8.2v", static_cast<long*>(nullptr), "   <nil>");
  check("%-20.16v", &intvar, "0xPTR  ");

  // erroneous things
  check("", nullptr, "%!(EXTRA <nil>)");
  check("", 2, "%!(EXTRA int=2)");
  check("no args", "hello", "no args%!(EXTRA string=hello)");
  check("%s %", "hello", "hello %!(NOVERB)");
  check("%s %.2", "hello", "hello %!(NOVERB)");
  check("%017091901790959340919092959340919017929593813360", 0, "%!(NOVERB)%!(EXTRA int=0)");
  check("%184467440737095516170v", 0, "%!(NOVERB)%!(EXTRA int=0)");
  // Extra argument errors should format without flags set.
  check("%010.2", "12345", "%!(NOVERB)%!(EXTRA string=12345)");

  // Comparison of padding rules with C printf.
  check("%.2f", 1.0, "1.00");
  check("%.2f", -1.0, "-1.00");
  check("% .2f", 1.0, " 1.00");
  check("% .2f", -1.0, "-1.00");
  check("%+.2f", 1.0, "+1.00");
  check("%+.2f", -1.0, "-1.00");
  check("%7.2f", 1.0, "   1.00");
  check("%7.2f", -1.0, "  -1.00");
  check("% 7.2f", 1.0, "   1.00");
  check("% 7.2f", -1.0, "  -1.00");
  check("%+7.2f", 1.0, "  +1.00");
  check("%+7.2f", -1.0, "  -1.00");
  check("% +7.2f", 1.0, "  +1.00");
  check("% +7.2f", -1.0, "  -1.00");
  check("%07.2f", 1.0, "0001.00");
  check("%07.2f", -1.0, "-001.00");
  check("% 07.2f", 1.0, " 001.00");
  check("% 07.2f", -1.0, "-001.00");
  check("%+07.2f", 1.0, "+001.00");
  check("%+07.2f", -1.0, "-001.00");
  check("% +07.2f", 1.0, "+001.00");
  check("% +07.2f", -1.0, "-001.00");

  // Complex numbers
  check("%7.2f", std::complex<double>{1, 2}, "(   1.00  +2.00i)");
  check("%+07.2f", std::complex<double>{-1, -2}, "(-001.00-002.00i)");

  // Incomplete format specification
  check("%.", 3, "%!.(int=3)");

  // Padding for complex numbers
  check("%+10.2f", std::complex<double>{104.66, 440.51}, "(   +104.66   +440.51i)");
  check("%+10.2f", std::complex<double>{-104.66, 440.51}, "(   -104.66   +440.51i)");
  check("%+10.2f", std::complex<double>{104.66, -440.51}, "(   +104.66   -440.51i)");
  check("%+10.2f", std::complex<double>{-104.66, -440.51}, "(   -104.66   -440.51i)");
  check("%+010.2f", std::complex<double>{104.66, 440.51}, "(+000104.66+000440.51i)");
  check("%+010.2f", std::complex<double>{-104.66, 440.51}, "(-000104.66+000440.51i)");
  check("%+010.2f", std::complex<double>{104.66, -440.51}, "(+000104.66-000440.51i)");
  check("%+010.2f", std::complex<double>{-104.66, -440.51}, "(-000104.66-000440.51i)");
}

TEST_CASE("Reorder tests", "[fmt]") {
  CHECK(sprintf("%[1]d", 1) == "1");
  CHECK(sprintf("%[2]d", 2, 1) == "1");
  CHECK(sprintf("%[2]d %[1]d", 1, 2) == "2 1");
  CHECK(sprintf("%[2]*[1]d", 2, 5) == "    2");
  CHECK(sprintf("%6.2f", 12.0) == " 12.00"); // Explicit version of next line.
  CHECK(sprintf("%[3]*.[2]*[1]f", 12.0, 2, 6) == " 12.00");
  CHECK(sprintf("%[1]*.[2]*[3]f", 6, 2, 12.0) == " 12.00");
  CHECK(sprintf("%10f", 12.0) == " 12.000000");
  CHECK(sprintf("%[1]*[3]f", 10, 99, 12.0) == " 12.000000");
  CHECK(sprintf("%.6f", 12.0) == "12.000000"); // Explicit version of next line.
  CHECK(sprintf("%.[1]*[3]f", 6, 99, 12.0) == "12.000000");
  CHECK(sprintf("%6.f", 12.0) == "    12"); // Explicit version of next line; empty precision means zero.
  CHECK(sprintf("%[1]*.[3]f", 6, 3, 12.0) == "    12");
  // An actual use! Print the same arguments twice.
  CHECK(sprintf("%d %d %d %#[1]o %#o %#o", 11, 12, 13) == "11 12 13 013 014 015");

  // Erroneous cases.
  CHECK(sprintf("%[d", 2, 1) == "%!d(BADINDEX)");
  CHECK(sprintf("%]d", 2, 1) == "%!](int=2)d%!(EXTRA int=1)");
  CHECK(sprintf("%[]d", 2, 1) == "%!d(BADINDEX)");
  CHECK(sprintf("%[-3]d", 2, 1) == "%!d(BADINDEX)");
  CHECK(sprintf("%[99]d", 2, 1) == "%!d(BADINDEX)");
  CHECK(sprintf("%[3]", 2, 1) == "%!(NOVERB)");
  CHECK(sprintf("%[1].2d", 5, 6) == "%!d(BADINDEX)");
  CHECK(sprintf("%[1]2d", 2, 1) == "%!d(BADINDEX)");
  CHECK(sprintf("%3.[2]d", 7) == "%!d(BADINDEX)");
  CHECK(sprintf("%.[2]d", 7) == "%!d(BADINDEX)");
  CHECK(sprintf("%d %d %d %#[1]o %#o %#o %#o", 11, 12, 13) == "11 12 13 013 014 015 %!o(MISSING)");
  CHECK(sprintf("%[5]d %[2]d %d", 1, 2, 3) == "%!d(BADINDEX) 2 3");
  CHECK(sprintf("%d %[3]d %d", 1, 2) == "1 %!d(BADINDEX) 2"); // Erroneous index does not affect sequence.
  CHECK(sprintf("%.[]") == R"(%!](BADINDEX))");               // Issue 10675
  CHECK(sprintf("%.-3d", 42) == "%!-(int=42)3d");             // TODO: Should this set return better error messages?
  CHECK(sprintf("%2147483648d", 42) == "%!(NOVERB)%!(EXTRA int=42)");
  CHECK(sprintf("%-2147483648d", 42) == "%!(NOVERB)%!(EXTRA int=42)");
  CHECK(sprintf("%.2147483648d", 42) == "%!(NOVERB)%!(EXTRA int=42)");
}

}  // namespace bongo::fmt
