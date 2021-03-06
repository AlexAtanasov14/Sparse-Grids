// Copyright (C) 2008-today The SG++ project
// This file is part of the SG++ project. For conditions of distribution and
// use, please see the copyright notice provided with SG++ or at
// sgpp.sparsegrids.org

#include <sgpp/pde/basis/linear/boundary/algorithm_sweep/PhiPhiUpBBLinearBoundary.hpp>

#include <sgpp/globaldef.hpp>

namespace SGPP {
namespace pde {

PhiPhiUpBBLinearBoundary::PhiPhiUpBBLinearBoundary(SGPP::base::GridStorage* storage)
    : PhiPhiUpBBLinear(storage) {}

PhiPhiUpBBLinearBoundary::~PhiPhiUpBBLinearBoundary() {}

void PhiPhiUpBBLinearBoundary::operator()(SGPP::base::DataVector& source,
                                          SGPP::base::DataVector& result, grid_iterator& index,
                                          size_t dim) {
  float_t q = this->boundingBox->getIntervalWidth(dim);
  float_t t = this->boundingBox->getIntervalOffset(dim);

  bool useBB = false;

  if (q != 1.0 || t != 0.0) {
    useBB = true;
  }

  // get boundary values
  float_t fl = 0.0;
  float_t fr = 0.0;

  if (useBB) {
    if (!index.hint()) {
      index.resetToLevelOne(dim);

      if (!this->storage->end(index.seq())) {
        recBB(source, result, index, dim, fl, fr, q, t);
      }

      index.resetToLeftLevelZero(dim);
    }

    size_t seq_left;
    size_t seq_right;

    // left boundary
    seq_left = index.seq();

    // right boundary
    index.resetToRightLevelZero(dim);
    seq_right = index.seq();

    // up
    //////////////////////////////////////
    // Left
    if (this->boundingBox->hasDirichletBoundaryLeft(dim)) {
      result[seq_left] = 0.0;  // source[seq_left];
    } else {
      result[seq_left] = fl;
      result[seq_left] += (((1.0 / 6.0) * source[seq_right]) * q);
    }

    // Right
    if (this->boundingBox->hasDirichletBoundaryRight(dim)) {
      result[seq_right] = 0.0;  // source[seq_right];
    } else {
      result[seq_right] = fr;
    }

    index.resetToLeftLevelZero(dim);
  } else {
    if (!index.hint()) {
      index.resetToLevelOne(dim);

      if (!this->storage->end(index.seq())) {
        rec(source, result, index, dim, fl, fr);
      }

      index.resetToLeftLevelZero(dim);
    }

    size_t seq_left;
    size_t seq_right;

    // left boundary
    seq_left = index.seq();

    // right boundary
    index.resetToRightLevelZero(dim);
    seq_right = index.seq();

    // up
    //////////////////////////////////////
    // Left
    if (this->boundingBox->hasDirichletBoundaryLeft(dim)) {
      result[seq_left] = 0.0;  // source[seq_left];
    } else {
      result[seq_left] = fl;
      result[seq_left] += ((1.0 / 6.0) * source[seq_right]);
    }

    // Right
    if (this->boundingBox->hasDirichletBoundaryRight(dim)) {
      result[seq_right] = 0.0;  // source[seq_right];
    } else {
      result[seq_right] = fr;
    }

    index.resetToLeftLevelZero(dim);
  }
}

}  // namespace pde
}  // namespace SGPP
