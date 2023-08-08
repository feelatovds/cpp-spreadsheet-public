#include "sheet.h"

#include "cell.h"
#include "common.h"

#include <iostream>
#include <algorithm>
#include <functional>
#include <iostream>
#include <optional>
#include <unordered_map>

using namespace std::literals;

Sheet::~Sheet() {
    cells_.clear();
}

void Sheet::CheckValidPos(Position pos) const {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Incorrect set...");
    }
}
void Sheet::ProcessEmptyCell(Position pos, std::unique_ptr<Cell>& cell) {
    try {
        cell->SetParents(*this);
    } catch (FormulaError& fe) {
        return;
    }
    if (HasCyclic(pos, *cell)) {
        throw CircularDependencyException("");
    }

    cell->SetChildren(*this, pos);
}
void Sheet::ProcessNotEmptyCell(Position pos, std::unique_ptr<Cell>& cell) {
    try {
        cell->SetParents(*this);
    } catch (FormulaError& fe) {
        return;
    }
    cell->SetChildren(*this, pos);
    if (HasCyclic(pos, *cell)) {
        throw CircularDependencyException("");
    }

    Cell* old_cell = dynamic_cast<Cell*>(GetCell(pos));
    for (const auto& parent: old_cell->GetParents()) {
        old_cell->ClearChildren(parent, *this, pos);
    }

    old_cell->InvalidCash(*this);
}

void Sheet::SetCell(Position pos, std::string text) {
    CheckValidPos(pos);

    auto cell = std::make_unique<Cell>(*this);
    cell->Set(std::move(text));

    if (GetCell(pos) == nullptr) { // ячейки не существовало
        ProcessEmptyCell(pos, cell);
    } else {
    	ProcessNotEmptyCell(pos, cell);
    }

    if (pos.row >= size_.rows || pos.col >= size_.cols /* добавить ячейку в "неоткрытую" область */
        || cells_[pos.row][pos.col] == nullptr /* ячейки не существует */) {
        size_t new_rows = std::max(size_.rows, pos.row + 1);
        size_t new_cols = std::max(size_.cols, pos.col + 1);

        cells_.resize(new_rows);
        for (size_t i = 0; i < cells_.size(); ++i) {
            cells_[i].resize(new_cols);
        }

        cells_[pos.row][pos.col] = std::move(cell);
        size_.rows = new_rows;
        size_.cols = new_cols;
    } else {
        cells_[pos.row][pos.col]->Clear();
        cells_[pos.row][pos.col] = std::move(cell);
    }

    fill_positions_.insert(pos);
}

const CellInterface* Sheet::GetCell(Position pos) const {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Incorrect const get...");
    }

    if (pos.row >= size_.rows || pos.col >= size_.cols /* запросить ячейку из "неоткрытой" области */
        || cells_[pos.row][pos.col] == nullptr /* ячейки не существует */) {
        return nullptr;
    } else {
        return cells_[pos.row][pos.col].get();
    }
}
CellInterface* Sheet::GetCell(Position pos) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Incorrect get...");
    }

    if (pos.row >= size_.rows || pos.col >= size_.cols /* запросить ячейку из "неоткрытой" области */
        || cells_[pos.row][pos.col] == nullptr /* ячейки не существует */) {
        return nullptr;
    } else {
        return cells_[pos.row][pos.col].get();
    }
}

void Sheet::ChangeRowSize() {
    for (int i = size_.rows - 1; i >= 0; --i) {
        if ( std::any_of(cells_[i].begin(), cells_[i].end(), [](auto& ptr) {
            return ptr != nullptr;
        }) ) {
            break;
        } else {
            --size_.rows;
        }
    }
}
void Sheet::ChangeColSize() {
    for (int j = size_.cols - 1; j >= 0; --j) {
        bool all_null = true;
        for (int i = 0; i < size_.rows; ++i) {
            if (cells_[i][j] != nullptr) {
                all_null = false;
                break;
            }
        }
        if (all_null) {
            --size_.cols;
        } else {
            break;
        }
    }
}
void Sheet::ClearCell(Position pos) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Incorrect clear...");
    }
    if (GetCell(pos) == nullptr) {
        return;
    }

    cells_[pos.row][pos.col].reset(nullptr);
    if (pos.row == size_.rows - 1) {
    	ChangeRowSize();
    }
    if (pos.col == size_.cols - 1) {
        ChangeColSize();
    }
}

Size Sheet::GetPrintableSize() const {
    return size_;
}

void Sheet::PrintValues(std::ostream& output) const {
    for (int i = 0; i < size_.rows; ++i) {
        bool is_first = true;
        for (int j = 0; j < size_.cols; ++j) {
            if (!is_first) {
                output << '\t';
            }
            is_first = false;

            if (GetCell({i, j}) == nullptr) {
                continue;
            }

            if (std::holds_alternative<double>(cells_[i][j]->GetValue())) {
                output << std::get<double>(cells_[i][j]->GetValue());
            } else if (std::holds_alternative<std::string>(cells_[i][j]->GetValue())) {
                output << std::get<std::string>(cells_[i][j]->GetValue());
            } else if (std::holds_alternative<FormulaError>(cells_[i][j]->GetValue())) {
                output << std::get<FormulaError>(cells_[i][j]->GetValue());
            }

        }
        output << '\n';
    }
}
void Sheet::PrintTexts(std::ostream& output) const {
    for (int i = 0; i < size_.rows; ++i) {
        bool is_first = true;
        for (int j = 0; j < size_.cols; ++j) {
            if (!is_first) {
                output << '\t';
            }
            is_first = false;
            if (GetCell({i, j}) == nullptr) {
                continue;
            }

            output << cells_[i][j]->GetText();
        }
        output << '\n';
    }
}

bool Sheet::HasCyclic(Position pos, const Cell& cell) {
    for (const auto& p: cell.GetParents()) {
        if (pos == p) {
            return true;
        }

        if (GetCell(p) != nullptr) {
            Cell* new_cell = dynamic_cast<Cell*>(const_cast<CellInterface *>(GetCell(p)));
            if (HasCyclic(pos, *new_cell)) {
                return true;
            }
        }
    }

    return false;
}

std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}
