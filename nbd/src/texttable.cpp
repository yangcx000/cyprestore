/*
 * Copyright 2020 JDD authors.
 * @yangbing1
 */

#include "texttable.h"

namespace cyprestore {
namespace nbd {

using std::string;

void TextTable::DefineColumn(
        const string &heading, enum TextTable::Align hd_align,
        enum TextTable::Align col_align) {
    TextTableColumn def(heading, heading.length(), hd_align, col_align);
    col_.push_back(def);
}

void TextTable::Clear() {
    currow_ = 0;
    curcol_ = 0;
    indent_ = 0;
    row_.clear();
    // reset widths to heading widths
    for (unsigned int i = 0; i < col_.size(); i++)
        col_[i].width = col_[i].heading.size();
}

/**
 * Pad s with space to appropriate alignment
 *
 * @param s string to pad
 * @param width width of field to contain padded string
 * @param align desired alignment (LEFT, CENTER, RIGHT)
 *
 * @return padded string
 */
static string pad(string s, int width, TextTable::Align align) {
    int lpad, rpad;
    lpad = 0;
    rpad = 0;
    switch (align) {
        case TextTable::LEFT:
            rpad = width - s.length();
            break;
        case TextTable::CENTER:
            lpad = width / 2 - s.length() / 2;
            rpad = width - lpad - s.length();
            break;
        case TextTable::RIGHT:
            lpad = width - s.length();
            break;
    }

    return string(lpad, ' ') + s + string(rpad, ' ');
}

std::ostream &operator<<(std::ostream &out, const TextTable &t) {
    for (unsigned int i = 0; i < t.col_.size(); i++) {
        TextTable::TextTableColumn col_ = t.col_[i];
        out << string(t.indent_, ' ')
            << pad(col_.heading, col_.width, col_.hd_align) << ' ';
    }
    out << std::endl;

    for (unsigned int i = 0; i < t.row_.size(); i++) {
        for (unsigned int j = 0; j < t.row_[i].size(); j++) {
            TextTable::TextTableColumn col_ = t.col_[j];
            out << string(t.indent_, ' ')
                << pad(t.row_[i][j], col_.width, col_.col_align) << ' ';
        }
        out << std::endl;
    }
    return out;
}

}  // namespace nbd
}  // namespace cyprestore
