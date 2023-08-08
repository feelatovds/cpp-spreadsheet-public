#include "formula.h"
#include "FormulaAST.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <sstream>
#include <iostream>

using namespace std::literals;

std::ostream& operator<<(std::ostream& output, FormulaError fe) {
    return output << fe.ToString();
}

namespace {
class Formula : public FormulaInterface {
public:
    explicit Formula(std::string expression)
    : ast_(ParseFormulaAST(expression)) {
    }

    Value Evaluate(const SheetInterface& sheet) const override {
        auto get_value = [&sheet, this](const Position* cell) {
            this->CheckValidCell(cell);
            if (!sheet.GetCell(*cell)) {
                return 0.0;
            }
            if ( std::holds_alternative<std::string>( sheet.GetCell(*cell)->GetValue()) ) { // std::string or double or FormulaError
                return this->GetStringCell(cell, sheet);
            }
            if ( std::holds_alternative<FormulaError>(sheet.GetCell(*cell)->GetValue()) ) {
                throw std::get<FormulaError>(sheet.GetCell(*cell)->GetValue());
            }

            return std::get<double>(sheet.GetCell(*cell)->GetValue());
        };

        try {
            return ast_.Execute(get_value);
        } catch (const FormulaError& fe) {
            return fe;
        }
    }
    std::string GetExpression() const override {
        std::ostringstream out;
        ast_.PrintFormula(out);
        return out.str();
    }

    std::vector<Position> GetReferencedCells() const override {
        std::vector<Position> ref_cells(ast_.GetCells().begin(), ast_.GetCells().end());
        std::sort(ref_cells.begin(), ref_cells.end());
        auto last = std::unique(ref_cells.begin(), ref_cells.end());
        ref_cells.erase(last, ref_cells.end());

        return ref_cells;
    }

private:
    FormulaAST ast_;
    
    void CheckValidCell(const Position* cell) const {
    	if (!cell->IsValid()) {
            throw FormulaError(FormulaError::Category::Ref);
        }
    }
    double GetStringCell(const Position* cell, const SheetInterface& sheet) const {
    	if ( std::get<std::string>( sheet.GetCell(*cell)->GetValue()) == "" ) {
            return 0.0;
        } else {
            for (auto ch: std::get<std::string>( sheet.GetCell(*cell)->GetValue()) ) {
            	if (ch != '.' && !isdigit(ch)) {
            	    throw FormulaError(FormulaError::Category::Value);
                }
            }

            try {
                return stod( std::get<std::string>( sheet.GetCell(*cell)->GetValue()) );
            } catch (const std::invalid_argument& er) {
            	throw FormulaError(FormulaError::Category::Value);
            }
        }
    }
};

}  // namespace

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) {
    return std::make_unique<Formula>(std::move(expression));
}
