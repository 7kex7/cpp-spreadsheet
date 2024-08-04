#include "common.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cassert>
#include <sstream>
#include <vector>

const int LETTERS = 26;
const int MAX_POSITION_LENGTH = 17;
const int MAX_POS_LETTER_COUNT = 3;
const int MAX_ROW_VALUE = 16383;
const int MAX_COL_VALUE = 16383;

const Position Position::NONE = {-1, -1};


int ColumnStringToInteger(std::string_view col) {
    int res = 0;

    for (size_t i = 0; i < col.size(); ++i) {
        int pos_in_alphabet = col[i] - 'A' + 1;
        assert((pos_in_alphabet <= 26 && pos_in_alphabet >= 0)  && "ColumnStringToInteger: wrong pos_in_alphabet var: " + pos_in_alphabet); 
        res += std::pow(26, (col.size() - 1u) - i) * pos_in_alphabet;
    }

    return res - 1;
}

std::string ColumnIntegerToString(int col) {
    if (col < 0)
        return "";

    const int pos_A_ASCII = 65;
    if (col < 26)
        return {(char)(pos_A_ASCII + col)};

    // how many columns we skip
    int columns_to_skip = 0;
    int power = 1;
    for (; (pow(26, power) + columns_to_skip) <= col; ++power)
        columns_to_skip += pow(26, power);
    col -= columns_to_skip;

    const int n_letters = power;
    std::vector<char> letters(n_letters, 'A');

    for (int idx = 0; idx < n_letters; ++idx) {
        // letters[idx] = 'A';
        // segment of changes of letters[idx] letter (as example, 
        // in str with length of 3 first (or left) letter changes every 676 (26*26) times)
        int multiplier = std::pow(26, (n_letters-1) - idx); 
        // pos letter in alphabet minus 1. how far is the letter after 'A'
        int pos_in_alphabet = col / multiplier;
        letters[idx] = (char)(pos_A_ASCII + pos_in_alphabet);
        col -= multiplier * pos_in_alphabet;
    }

    if (col != 0)
        return "";

    return std::string(letters.begin(), letters.end());
}

// returns {"", ""} as result if index format is invalid
std::pair<std::string, std::string> SeparateIndex(std::string_view idx) {
    using namespace std::literals;
    std::pair empty_res = {"", ""};

    std::string col;
    std::string row;
    size_t i = 0;

    while(std::isalpha(idx[i])) {
        if (!std::isupper(idx[i])) {
            return empty_res;
        }
        col += idx[i];
        ++i;
    }
    while(i < idx.size()) {
        // let us know if row is negative.
        if (!std::isdigit(idx[i])) {
            return empty_res;
        }
        row += idx[i];
        ++i;
    }

    return {col, row};
}

// Реализуйте методы:
bool Position::operator==(const Position rhs) const {
    return (row == rhs.row) && (col == rhs.col);
}

bool Position::operator<(const Position rhs) const {
    if (row < rhs.row) {
        return true;
    } else if (row == rhs.row) {
        return col < rhs.col;
    }
    return false;
}

bool Position::IsValid() const {
    if (col > MAX_COL_VALUE || col < 0 || row > MAX_ROW_VALUE || row < 0
        || *this == Position::NONE) {
        return false;
    }
    return true;
}

std::string Position::ToString() const {
    if (!IsValid()) {
        return "";
    }
    return ColumnIntegerToString(col) + std::to_string(row + 1);
}

Position Position::FromString(std::string_view str) {
    if (str.size() > MAX_POSITION_LENGTH) {
        return Position::NONE;
    }

    const auto [column, row] = SeparateIndex(str);
    if (row.empty() || column.empty() || column.size() > MAX_POS_LETTER_COUNT) {
        return Position::NONE;
    }

    Position pos{std::stoi(row)-1, ColumnStringToInteger(column)};
    if (!pos.IsValid()) {
        return Position::NONE;
    }

    return pos;
}

bool Size::operator==(Size rhs) const {
    return cols == rhs.cols && rows == rhs.rows;
}