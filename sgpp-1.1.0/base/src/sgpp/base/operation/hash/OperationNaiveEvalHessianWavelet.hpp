// Copyright (C) 2008-today The SG++ project
// This file is part of the SG++ project. For conditions of distribution and
// use, please see the copyright notice provided with SG++ or at
// sgpp.sparsegrids.org

#ifndef OPERATIONNAIVEEVALHESSIANWAVELET_HPP
#define OPERATIONNAIVEEVALHESSIANWAVELET_HPP

#include <sgpp/globaldef.hpp>
#include <sgpp/base/operation/hash/OperationNaiveEvalHessian.hpp>
#include <sgpp/base/grid/GridStorage.hpp>
#include <sgpp/base/operation/hash/common/basis/WaveletBasis.hpp>
#include <sgpp/base/datatypes/DataVector.hpp>
#include <sgpp/base/datatypes/DataMatrix.hpp>

namespace SGPP {
namespace base {

/**
 * Operation for evaluating wavelet linear combinations on Noboundary grids, their gradients
 * and their Hessians.
 */
class OperationNaiveEvalHessianWavelet : public OperationNaiveEvalHessian {
 public:
  /**
   * Constructor.
   *
   * @param storage   storage of the sparse grid
   */
  explicit OperationNaiveEvalHessianWavelet(GridStorage* storage) : storage(
      storage) {
  }

  /**
   * Destructor.
   */
  ~OperationNaiveEvalHessianWavelet() override {
  }

  /**
   * @param       alpha       coefficient vector
   * @param       point       evaluation point
   * @param[out]  gradient    gradient vector of linear combination
   * @param[out]  hessian     Hessian matrix of linear combination
   * @return                  value of linear combination
   */
  float_t evalHessian(const DataVector& alpha,
                      const DataVector& point,
                      DataVector& gradient,
                      DataMatrix& hessian) override;

 protected:
  /// storage of the sparse grid
  GridStorage* storage;
  /// 1D wavelet basis
  SWaveletBase base;
};

}  // namespace base
}  // namespace SGPP

#endif /* OPERATIONNAIVEEVALHESSIANWAVELET_HPP */
