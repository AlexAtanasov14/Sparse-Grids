// Copyright (C) 2008-today The SG++ project
// This file is part of the SG++ project. For conditions of distribution and
// use, please see the copyright notice provided with SG++ or at
// sgpp.sparsegrids.org

#include <sgpp/datadriven/operation/hash/simple/OperationDensityConditionalLinear.hpp>
#include <sgpp/base/exception/operation_exception.hpp>
#include <sgpp/base/operation/BaseOpFactory.hpp>

#include <sgpp/globaldef.hpp>
#include <algorithm>

namespace SGPP {
namespace datadriven {

void OperationDensityConditionalLinear::doConditional(base::DataVector& alpha, base::Grid*& mg,
                                                      base::DataVector& malpha, unsigned int mdim,
                                                      float_t xbar) {
  /**
   * Assume: mdim = 1
   * Compute vector with values
   * (phi_{l1,i1}(xbar) = phi_{l1,i1}(xbar)*phi_{l2,i2}(0.5)
   */
  SGPP::base::GridStorage* gs = this->grid->getStorage();
  SGPP::base::DataVector zeta(alpha.getSize());
  SGPP::base::GridIndex* gp;

  for (size_t seqNr = 0; seqNr < alpha.getSize(); seqNr++) {
    gp = gs->get(seqNr);
    zeta[seqNr] =
        std::max(1. - std::fabs(xbar * std::pow(2.0, static_cast<float_t>(gp->getLevel(mdim))) -
                                static_cast<float_t>(gp->getIndex(mdim))),
                 0.);
  }

  /**
   * Compute
   * theta = theta + alpha_{l,i}*zeta_{l,i}*int{phi_{l2, i_2}}
   */
  float_t theta = 0;
  float_t tmpint = 0;

  for (size_t seqNr = 0; seqNr < gs->size(); seqNr++) {
    gp = gs->get(seqNr);
    tmpint = 1;

    for (unsigned int d = 0; d < gs->dim(); d++) {
      if (d != mdim) tmpint *= pow(2.0, -static_cast<float_t>(gp->getLevel(d)));
    }

    theta += alpha[seqNr] * zeta[seqNr] * tmpint;
  }

  // std::cout << theta << std::endl;

  /**
   * Generate d - 1 dimensional grid, as in marginalize
   */
  /**
   * Note: Because of adaptively refined sparse grids, we cannot simply
   * generate a regular grid. Thus, we need to add point after point
   * to the new grid mg
   */
  // create grid of dimensions d - 1 of the same type
  if (gs->dim() < 2)
    throw SGPP::base::operation_exception(
        "OperationDensityConditional is not possible for less than 2 dimensions");

  mg = base::Grid::createLinearGrid(gs->dim() - 1);
  base::GridStorage* mgs = mg->getStorage();

  // run through grid g and add points to mg
  SGPP::base::GridIndex mgp(mgs->dim());

  for (size_t seqNr = 0; seqNr < gs->size(); seqNr++) {
    gp = gs->get(seqNr);

    for (unsigned int d = 0; d < gs->dim(); d++) {
      // skip direction in which we marginalize
      if (d == mdim) {
        continue;
      } else {
        if (d < mdim) {
          mgp.set(d, gp->getLevel(d), gp->getIndex(d));
        } else {
          mgp.set(d - 1, gp->getLevel(d), gp->getIndex(d));
        }
      }
    }

    if (!mgs->has_key(&mgp)) mgs->insert(mgp);
  }

  mgs->recalcLeafProperty();

  /**
   * Compute coefficients malpha for grid mg
   */
  malpha.resize(mgs->size());
  malpha.setAll(0.0);
  size_t mseqNr;

  for (size_t seqNr = 0; seqNr < gs->size(); seqNr++) {
    gp = gs->get(seqNr);

    for (unsigned int d = 0; d < gs->dim(); d++) {
      if (d < mdim)
        mgp.set(d, gp->getLevel(d), gp->getIndex(d));
      else if (d > mdim)
        mgp.set(d - 1, gp->getLevel(d), gp->getIndex(d));
    }

    if (!mgs->has_key(&mgp))
      throw SGPP::base::operation_exception(
          "Key not found! This should not happen! There is something seriously wrong!");

    // get index in alpha vector for current basis function
    mseqNr = mgs->seq(&mgp);
    // update corresponding coefficient
    malpha[mseqNr] += alpha[seqNr] * zeta[seqNr];
  }

  malpha.mult(1. / theta);
}
}  // namespace datadriven
}  // namespace SGPP
