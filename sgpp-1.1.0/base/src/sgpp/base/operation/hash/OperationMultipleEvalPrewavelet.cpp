// Copyright (C) 2008-today The SG++ project
// This file is part of the SG++ project. For conditions of distribution and
// use, please see the copyright notice provided with SG++ or at
// sgpp.sparsegrids.org

#include <sgpp/base/operation/hash/common/basis/PolyModifiedBasis.hpp>

#include <sgpp/base/operation/hash/OperationMultipleEvalPrewavelet.hpp>

#include <sgpp/base/algorithm/AlgorithmDGEMV.hpp>


#include <sgpp/globaldef.hpp>


namespace SGPP {
namespace base {

void OperationMultipleEvalPrewavelet::mult(DataVector& alpha,
    DataVector& result) {
  AlgorithmDGEMV<SPrewaveletBase> op;
  PrewaveletBasis<unsigned int, unsigned int> base;

  op.mult(storage, base, alpha, this->dataset, result);
}

void OperationMultipleEvalPrewavelet::multTranspose(DataVector& source,
    DataVector& result) {
  AlgorithmDGEMV<SPrewaveletBase> op;
  PrewaveletBasis<unsigned int, unsigned int> base;

  op.mult_transposed(storage, base, source, this->dataset, result);
}

}  // namespace base
}  // namespace SGPP
