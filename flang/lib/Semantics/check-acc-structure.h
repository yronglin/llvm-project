//===-- lib/Semantics/check-acc-structure.h ---------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
// OpenACC 3.3 structure validity check list
//    1. invalid clauses on directive
//    2. invalid repeated clauses on directive
//    3. invalid nesting of regions
//
//===----------------------------------------------------------------------===//

#ifndef FORTRAN_SEMANTICS_CHECK_ACC_STRUCTURE_H_
#define FORTRAN_SEMANTICS_CHECK_ACC_STRUCTURE_H_

#include "check-directive-structure.h"
#include "flang/Common/enum-set.h"
#include "flang/Parser/parse-tree.h"
#include "flang/Semantics/semantics.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/Frontend/OpenACC/ACC.h.inc"

using AccDirectiveSet = Fortran::common::EnumSet<llvm::acc::Directive,
    llvm::acc::Directive_enumSize>;

using AccClauseSet =
    Fortran::common::EnumSet<llvm::acc::Clause, llvm::acc::Clause_enumSize>;

#define GEN_FLANG_DIRECTIVE_CLAUSE_SETS
#include "llvm/Frontend/OpenACC/ACC.inc"

namespace Fortran::semantics {

class AccStructureChecker
    : public DirectiveStructureChecker<llvm::acc::Directive, llvm::acc::Clause,
          parser::AccClause, llvm::acc::Clause_enumSize> {
public:
  AccStructureChecker(SemanticsContext &context)
      : DirectiveStructureChecker(context,
#define GEN_FLANG_DIRECTIVE_CLAUSE_MAP
#include "llvm/Frontend/OpenACC/ACC.inc"
        ) {
  }

  // Construct and directives
  void Enter(const parser::OpenACCBlockConstruct &);
  void Leave(const parser::OpenACCBlockConstruct &);
  void Enter(const parser::OpenACCCombinedConstruct &);
  void Leave(const parser::OpenACCCombinedConstruct &);
  void Enter(const parser::OpenACCLoopConstruct &);
  void Leave(const parser::OpenACCLoopConstruct &);
  void Enter(const parser::OpenACCRoutineConstruct &);
  void Leave(const parser::OpenACCRoutineConstruct &);
  void Enter(const parser::OpenACCStandaloneConstruct &);
  void Leave(const parser::OpenACCStandaloneConstruct &);
  void Enter(const parser::OpenACCStandaloneDeclarativeConstruct &);
  void Leave(const parser::OpenACCStandaloneDeclarativeConstruct &);
  void Enter(const parser::OpenACCWaitConstruct &);
  void Leave(const parser::OpenACCWaitConstruct &);
  void Enter(const parser::OpenACCAtomicConstruct &);
  void Leave(const parser::OpenACCAtomicConstruct &);
  void Enter(const parser::OpenACCCacheConstruct &);
  void Leave(const parser::OpenACCCacheConstruct &);
  void Enter(const parser::AccAtomicUpdate &);
  void Enter(const parser::AccAtomicCapture &);
  void Enter(const parser::AccAtomicWrite &);
  void Enter(const parser::AccAtomicRead &);
  void Enter(const parser::OpenACCEndConstruct &);

  // Clauses
  void Leave(const parser::AccClauseList &);
  void Enter(const parser::AccClause &);

  void Enter(const parser::Module &);
  void Enter(const parser::SubroutineSubprogram &);
  void Enter(const parser::FunctionSubprogram &);
  void Enter(const parser::SeparateModuleSubprogram &);
  void Enter(const parser::DoConstruct &);
  void Leave(const parser::DoConstruct &);

#define GEN_FLANG_CLAUSE_CHECK_ENTER
#include "llvm/Frontend/OpenACC/ACC.inc"

private:
  void CheckAtomicStmt(
      const parser::AssignmentStmt &assign, const std::string &construct);
  void CheckAtomicUpdateStmt(const parser::AssignmentStmt &assign,
      const SomeExpr &updateVar, const SomeExpr *captureVar);
  void CheckAtomicCaptureStmt(const parser::AssignmentStmt &assign,
      const SomeExpr *updateVar, const SomeExpr &captureVar);
  void CheckAtomicWriteStmt(const parser::AssignmentStmt &assign,
      const SomeExpr &updateVar, const SomeExpr *captureVar);
  void CheckAtomicUpdateVariable(
      const parser::Variable &updateVar, const parser::Variable &captureVar);
  void CheckAtomicCaptureVariable(
      const parser::Variable &captureVar, const parser::Variable &updateVar);

  bool CheckAllowedModifier(llvm::acc::Clause clause);
  bool IsComputeConstruct(llvm::acc::Directive directive) const;
  bool IsInsideComputeConstruct() const;
  void CheckNotInComputeConstruct();
  void CheckMultipleOccurrenceInDeclare(
      const parser::AccObjectList &, llvm::acc::Clause);
  void CheckMultipleOccurrenceInDeclare(
      const parser::AccObjectListWithModifier &, llvm::acc::Clause);
  llvm::StringRef getClauseName(llvm::acc::Clause clause) override;
  llvm::StringRef getDirectiveName(llvm::acc::Directive directive) override;

  llvm::SmallDenseMap<Symbol *, llvm::acc::Clause> declareSymbols;
  unsigned loopNestLevel = 0;
};

} // namespace Fortran::semantics

#endif // FORTRAN_SEMANTICS_CHECK_ACC_STRUCTURE_H_
