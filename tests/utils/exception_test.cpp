#include "hps/utils/exception.hpp"
#include <gtest/gtest.h>

namespace hps::tests {

TEST(ExceptionTest, HPSErrorConstruction) {
    Location loc(10, 2, 5);
    HPSError error(ErrorCode::InvalidToken, "Invalid token found", loc);
    
    EXPECT_EQ(error.code, ErrorCode::InvalidToken);
    EXPECT_EQ(error.message, "Invalid token found");
    EXPECT_EQ(error.location.position, 10u);
    EXPECT_EQ(error.location.line, 2u);
    EXPECT_EQ(error.location.column, 5u);
}

TEST(ExceptionTest, HPSExceptionConstruction) {
    Location loc(10, 2, 5);
    HPSError error(ErrorCode::InvalidToken, "Invalid token found", loc);
    HPSException ex(error);
    
    EXPECT_EQ(ex.code(), ErrorCode::InvalidToken);
    EXPECT_STREQ(ex.what(), "Invalid token found");
    EXPECT_EQ(ex.position(), 10u);
    EXPECT_EQ(ex.line(), 2u);
    EXPECT_EQ(ex.column(), 5u);
}

TEST(ExceptionTest, DirectConstruction) {
    HPSException ex(ErrorCode::UnknownError, "Unknown error", 100);
    
    EXPECT_EQ(ex.code(), ErrorCode::UnknownError);
    EXPECT_EQ(ex.position(), 100u);
    EXPECT_EQ(ex.line(), 1u); // Default
    EXPECT_EQ(ex.column(), 1u); // Default
}

TEST(ExceptionTest, FromPositionCalculatesLineAndColumn) {
    const auto source = std::string_view("ab\ncd\nef");

    const auto start = Location::from_position(source, 0);
    EXPECT_EQ(start.line, 1u);
    EXPECT_EQ(start.column, 1u);

    const auto middle = Location::from_position(source, 5);
    EXPECT_EQ(middle.line, 2u);
    EXPECT_EQ(middle.column, 3u);

    const auto eof = Location::from_position(source, source.size());
    EXPECT_EQ(eof.line, 3u);
    EXPECT_EQ(eof.column, 3u);
}

} // namespace hps::tests
