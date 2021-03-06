// Copyright (C) 2008-today The SG++ project
// This file is part of the SG++ project. For conditions of distribution and
// use, please see the copyright notice provided with SG++ or at
// sgpp.sparsegrids.org

#include <sgpp/datadriven/algorithm/DMSystemMatrixBase.hpp>

#include <sgpp/globaldef.hpp>


namespace SGPP {
namespace datadriven {

DMSystemMatrixBase::DMSystemMatrixBase(SGPP::base::DataMatrix& trainData,
                                       float_t lambda)
  : dataset_(&trainData), lambda_(lambda), completeTimeMult_(0.0),
    computeTimeMult_(0.0),
    completeTimeMultTrans_(0.0), computeTimeMultTrans_(0.0) {
  myTimer_ = new SGPP::base::SGppStopwatch();
}

DMSystemMatrixBase::~DMSystemMatrixBase() {
  delete myTimer_;
}

void DMSystemMatrixBase::prepareGrid() {
}

void DMSystemMatrixBase::resetTimers() {
  completeTimeMult_ = 0.0;
  computeTimeMult_ = 0.0;
  completeTimeMultTrans_ = 0.0;
  computeTimeMultTrans_ = 0.0;
}

void DMSystemMatrixBase::getTimers(float_t& timeMult, float_t& computeMult,
                                   float_t& timeMultTrans, float_t& computeMultTrans) {
  timeMult = completeTimeMult_;
  computeMult = computeTimeMult_;
  timeMultTrans = completeTimeMultTrans_;
  computeMultTrans = computeTimeMultTrans_;
}

}  // namespace datadriven
}  // namespace SGPP

