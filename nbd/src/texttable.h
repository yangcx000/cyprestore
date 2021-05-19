/*
 * Copyright 2020 JDD authors.
 * @yangbing1
 */

#ifndef NBD_SRC_TEXTTABLE_H_
#define NBD_SRC_TEXTTABLE_H_

#include <sstream>
#include <string>
#include <vector>

/**
 * TextTable:
 * Manage tabular output of data.  Caller defines heading of each column
 * and alignment of heading and column data,
 * then inserts rows of data including tuples of
 * length (ncolumns) terminated by TextTable::endrow.  When all rows
 * are inserted, caller asks for output with ostream <<
 * which sizes/pads/dumps the table to ostream.
 *
 * Columns autosize to largest heading or datum.  One space is printed
 * between columns.
 */

namespace cyprestore {
namespace nbd {

// 用于格式化输出
class TextTable {
public:
    enum Align { LEFT = 1, CENTER, RIGHT };

private:
    struct TextTableColumn {
        std::string heading;
        int width;
        Align hd_align;
        Align col_align;

        TextTableColumn() {}
        TextTableColumn(const std::string &h, int w, Align ha, Align ca)
                : heading(h), width(w), hd_align(ha), col_align(ca) {}
        ~TextTableColumn() {}
    };

    std::vector<TextTableColumn> col_;  // column definitions
    unsigned int curcol_, currow_;       // col_, row_ being inserted into
    unsigned int indent_;               // indent_ width when rendering

protected:
    std::vector<std::vector<std::string>> row_;  // row_ data array

public:
    TextTable() : curcol_(0), currow_(0), indent_(0) {}
    ~TextTable() {}

    /**
     * Define a column in the table.
     *
     * @param heading Column heading string (or "")
     * @param hd_align Alignment for heading in column
     * @param col_align Data alignment
     *
     * @note alignment is of type TextTable::Align; values are
     * TextTable::LEFT, TextTable::CENTER, or TextTable::RIGHT
     *
     */
    void
    DefineColumn(const std::string &heading, Align hd_align, Align col_align);

    /**
     * Set indent_ for table.  Only affects table output.
     *
     * @param i Number of spaces to indent_
     */
    void SetIndent(int i) {
        indent_ = i;
    }

    /**
     * Add item to table, perhaps on new row_.
     * table << val1 << val2 << TextTable::endrow;
     *
     * @param: value to output.
     *
     * @note: Numerics are output in decimal; strings are not truncated.
     * Output formatting choice is limited to alignment in DefineColumn().
     *
     * @return TextTable& for chaining.
     */

    template <typename T> TextTable &operator<<(const T &item) {
        if (row_.size() < currow_ + 1) row_.resize(currow_ + 1);

        /**
         * col_.size() is a good guess for how big row_[currow_] needs to be,
         * so just expand it out now
         */
        if (row_[currow_].size() < col_.size()) {
            row_[currow_].resize(col_.size());
        }

        // inserting more items than defined columns is a coding error
        // assert(curcol_ + 1 <= col_.size());

        // get rendered width of item alone
        std::ostringstream oss;
        oss << item;
        int width = oss.str().length();
        oss.seekp(0);

        // expand column width if necessary
        if (width > col_[curcol_].width) {
            col_[curcol_].width = width;
        }

        // now store the rendered item with its proper width
        row_[currow_][curcol_] = oss.str();

        curcol_++;
        return *this;
    }

    /**
     * Degenerate type/variable here is just to allow selection of the
     * following operator<< for "<< TextTable::endrow"
     */

    struct endrow_t {};
    static endrow_t endrow;

    /**
     * Implements TextTable::endrow
     */

    TextTable &operator<<(endrow_t) {
        curcol_ = 0;
        currow_++;
        return *this;
    }

    /**
     * Render table to ostream (i.e. cout << table)
     */

    friend std::ostream &operator<<(std::ostream &out, const TextTable &t);

    /**
     * Clear: Reset everything in a TextTable except column defs
     * resize cols to heading widths, Clear indent_
     */

    void Clear();
};

}  // namespace nbd
}  // namespace cyprestore

#endif  // NBD_SRC_TEXTTABLE_H_
