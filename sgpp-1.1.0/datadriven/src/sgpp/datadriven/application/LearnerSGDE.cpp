// Copyright (C) 2008-today The SG++ project
// This file is part of the SG++ project. For conditions of distribution and
// use, please see the copyright notice provided with SG++ or at
// sgpp.sparsegrids.org

#include <sgpp/datadriven/application/LearnerSGDE.hpp>

#include <sgpp/base/exception/application_exception.hpp>
#include <sgpp/base/grid/generation/functors/SurplusRefinementFunctor.hpp>
#include <sgpp/base/grid/generation/GridGenerator.hpp>
#include <sgpp/base/grid/GridStorage.hpp>
#include <sgpp/base/grid/storage/hashmap/HashGridStorage.hpp>
#include <sgpp/base/operation/BaseOpFactory.hpp>
#include <sgpp/base/operation/hash/OperationEval.hpp>
#include <sgpp/base/operation/hash/OperationFirstMoment.hpp>
#include <sgpp/base/operation/hash/OperationMultipleEval.hpp>
#include <sgpp/pde/operation/PdeOpFactory.hpp>
#include <sgpp/solver/sle/ConjugateGradients.hpp>
#include <sgpp/datadriven/algorithm/DensitySystemMatrix.hpp>

#include <stddef.h>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <vector>

// using namespace std;
// using namespace SGPP::base;

using SGPP::base::DataVector;
using SGPP::base::DataMatrix;
using SGPP::base::Grid;
using SGPP::base::GridStorage;
using SGPP::base::OperationMatrix;
using SGPP::base::SurplusRefinementFunctor;
using SGPP::base::GridIndex;
using SGPP::base::OperationEval;
using SGPP::base::GridGenerator;
using SGPP::base::GridType;
using SGPP::base::OperationSecondMoment;
using SGPP::base::OperationFirstMoment;
using SGPP::base::OperationMultipleEval;

using std::vector;
using std::cout;
using std::endl;

namespace SGPP {
namespace datadriven {

LearnerSGDE::LearnerSGDE(SGPP::base::RegularGridConfiguration& gridConfig,
                         SGPP::base::AdpativityConfiguration& adaptivityConfig,
                         SGPP::solver::SLESolverConfiguration& solverConfig,
                         SGPP::datadriven::RegularizationConfiguration& regularizationConfig,
                         LearnerSGDEConfiguration& learnerSGDEConfig) :
  grid(NULL), alpha(1), samples(NULL), gridConfig(gridConfig), adaptivityConfig(
    adaptivityConfig), solverConfig(solverConfig), regularizationConfig(
      regularizationConfig), learnerSGDEConfig(learnerSGDEConfig) {
}

LearnerSGDE::~LearnerSGDE() {
  if (samples != NULL) {
    delete samples;
  }

  if (grid != NULL) {
    delete grid;
  }
}

void LearnerSGDE::initialize(SGPP::base::DataMatrix& psamples) {
  samples = new DataMatrix(psamples);
  size_t ndim = psamples.getNcols();
  createRegularGrid(grid, ndim);
  alpha.resize(grid->getSize());

  // optimize the regularization parameter
  float_t lambdaReg = 0.0;

  if (learnerSGDEConfig.doCrossValidation_) {
    lambdaReg = optimizeLambdaCV();
  } else {
    lambdaReg = learnerSGDEConfig.lambda_;
  }

  // learn the data -> do the density estimation
  train(*grid, alpha, *samples, lambdaReg);
}

// ---------------------------------------------------------------------------

float_t LearnerSGDE::pdf(DataVector& x) {
  OperationEval* opEval = SGPP::op_factory::createOperationEval(*grid);
  float_t ret = opEval->eval(alpha, x);
  delete opEval;
  return ret;
}

void LearnerSGDE::pdf(DataMatrix& points, DataVector& res) {
  OperationMultipleEval* opEvalMulti =
    SGPP::op_factory::createOperationMultipleEval(*grid, points);
  opEvalMulti->eval(alpha, res);
}

float_t LearnerSGDE::mean() {
  OperationFirstMoment* opMoment = op_factory::createOperationFirstMoment(
                                     *grid);
  float_t res = opMoment->doQuadrature(alpha);
  delete opMoment;
  return res;
}

float_t LearnerSGDE::variance() {
  OperationSecondMoment* opMoment = op_factory::createOperationSecondMoment(
                                      *grid);
  float_t secondMoment = opMoment->doQuadrature(alpha);
  delete opMoment;

  // use Steiners translation theorem to compute the variance
  float_t firstMoment = mean();
  float_t res = secondMoment - firstMoment * firstMoment;
  return res;
}

void LearnerSGDE::cov(DataMatrix& cov) {
  return;
}

DataVector* LearnerSGDE::getSamples(size_t dim) {
  DataVector* isamples = new DataVector(getNsamples());
  samples->getColumn(dim, *isamples);
  return isamples;
}

DataMatrix* LearnerSGDE::getSamples() {
  return new DataMatrix(*samples);
}

size_t LearnerSGDE::getDim() {
  return samples->getNcols();
}

size_t LearnerSGDE::getNsamples() {
  return samples->getNrows();
}

DataVector LearnerSGDE::getSurpluses() {
  return alpha;
}

GridStorage* LearnerSGDE::getGridStorage() {
  return grid->getStorage();
}
// ---------------------------------------------------------------------------

void LearnerSGDE::createRegularGrid(Grid*& grid, size_t ndim) {
  // load grid
  if (gridConfig.type_ == GridType::Linear) {
    grid = Grid::createLinearGrid(ndim);
  } else if (gridConfig.type_ == GridType::LinearL0Boundary) {
    grid = Grid::createLinearBoundaryGrid(ndim, 0);
  } else if (gridConfig.type_ == GridType::LinearBoundary) {
    grid = Grid::createLinearBoundaryGrid(ndim);
  } else {
    throw base::application_exception("LeanerSGDE::initialize : grid type is not supported");
  }

  GridGenerator* gridGen = grid->createGridGenerator();
  gridGen->regular(gridConfig.level_);
}

float_t LearnerSGDE::optimizeLambdaCV() {
  Grid* grid = NULL;
  DataVector* alpha = NULL;

  float_t curLambda;
  float_t bestLambda = 0;
  float_t curMean = 0;
  float_t curMeanAcc = 0;
  float_t bestMeanAcc = 0;

  size_t kfold = learnerSGDEConfig.kfold_;

  vector<DataMatrix*> kfold_train(kfold);
  vector<DataMatrix*> kfold_test(kfold);
  splitset(kfold_train, kfold_test);

  float_t lambdaStart = learnerSGDEConfig.lambdaStart_;
  float_t lambdaEnd = learnerSGDEConfig.lambdaEnd_;

  if (learnerSGDEConfig.logScale_) {
    lambdaStart = log(lambdaStart);
    lambdaEnd = log(lambdaEnd);
  }

  for (size_t i = 0; i < learnerSGDEConfig.lambdaSteps_; i++) {
    // compute current lambda
    curLambda = lambdaStart
                + static_cast<float_t>(i) * (lambdaEnd - lambdaStart)
                / static_cast<float_t>(learnerSGDEConfig.lambdaSteps_
                                       - 1);

    if (learnerSGDEConfig.logScale_)
      curLambda = exp(curLambda);

    if (i
        % static_cast<size_t>(std::max(
                                static_cast<float_t>(learnerSGDEConfig.lambdaSteps_)
                                / 10.0f, static_cast<float_t>(1.0f))) == 0) {
      if (!learnerSGDEConfig.silent_) {
        cout << i + 1 << "/" << learnerSGDEConfig.lambdaSteps_
             << " (lambda = " << curLambda << ") " << endl;
        cout.flush();
      }
    }

    // cross-validation
    curMeanAcc = 0.0;
    curMean = 0.0;

    for (size_t j = 0; j < kfold; j++) {
      // initialize standard grid and alpha vector
      createRegularGrid(grid, getDim());
      alpha = new DataVector(getNsamples());
      // compute density
      train(*grid, *alpha, *(kfold_train[j]), curLambda);
      // get L2 norm of residual for test set
      curMean = computeResidual(*grid, *alpha, *(kfold_test[j]), 0.0);
      curMeanAcc += curMean;

      if (!learnerSGDEConfig.silent_) {
        cout << "# " << curLambda << " " << i << " " << j << " "
             << curMeanAcc << " " << curMean << endl;
      }

      // free space
      delete grid;
      delete alpha;
    }

    curMeanAcc /= static_cast<float_t>(kfold);

    if (i == 0 || curMeanAcc < bestMeanAcc) {
      bestMeanAcc = curMeanAcc;
      bestLambda = curLambda;
    }

    if (!learnerSGDEConfig.silent_) {
      cout << "# " << curLambda << " " << bestLambda << " " << i << " "
           << curMeanAcc << endl;
    }
  }

  if (!learnerSGDEConfig.silent_) {
    cout << "# -> best lambda = " << bestLambda << endl;
  }

  // free splitted sets
  for (size_t i = 0; i < kfold; i++) {
    delete kfold_train[i];
    delete kfold_test[i];
  }

  return bestLambda;
}

void LearnerSGDE::train(Grid& grid, DataVector& alpha, DataMatrix& train,
                        float_t lambdaReg) {
  size_t dim = train.getNcols();

  GridStorage* gridStorage = grid.getStorage();
  GridGenerator* gridGen = grid.createGridGenerator();
  DataVector rhs(grid.getStorage()->size());
  alpha.resize(grid.getStorage()->size());
  alpha.setAll(0.0);

  if (!learnerSGDEConfig.silent_) {
    cout << "# LearnerSGDE: grid points " << grid.getSize() << endl;
  }

  for (size_t ref = 0; ref <= adaptivityConfig.numRefinements_; ref++) {
    OperationMatrix* C = computeRegularizationMatrix(grid);

    SGPP::datadriven::DensitySystemMatrix SMatrix(grid, train, *C, lambdaReg);
    SMatrix.generateb(rhs);

    if (!learnerSGDEConfig.silent_) {
      cout << "# LearnerSGDE: Solving " << endl;
    }

    SGPP::solver::ConjugateGradients myCG(solverConfig.maxIterations_,
                                          solverConfig.eps_);
    myCG.solve(SMatrix, alpha, rhs, false, false, solverConfig.threshold_);

    if (ref < adaptivityConfig.numRefinements_) {
      if (!learnerSGDEConfig.silent_) {
        cout << "# LearnerSGDE: Refine grid ... ";
      }

      // Weight surplus with function evaluation at grid points
      OperationEval* opEval = SGPP::op_factory::createOperationEval(grid);
      GridIndex* gp;
      DataVector p(dim);
      DataVector alphaWeight(alpha.getSize());

      for (size_t i = 0; i < gridStorage->size(); i++) {
        gp = gridStorage->get(i);
        gp->getCoords(p);
        alphaWeight[i] = alpha[i] * opEval->eval(alpha, p);
      }

      delete opEval;
      opEval = NULL;

      SurplusRefinementFunctor srf(&alphaWeight,
                                   adaptivityConfig.noPoints_, adaptivityConfig.threshold_);
      gridGen->refine(&srf);

      if (!learnerSGDEConfig.silent_) {
        cout << "# LearnerSGDE: ref " << ref << "/"
             << adaptivityConfig.numRefinements_ - 1 << ": "
             << grid.getStorage()->size() << endl;
      }

      alpha.resize(grid.getStorage()->size());
      rhs.resize(grid.getStorage()->size());
      alpha.setAll(0.0);
      rhs.setAll(0.0);
    }

    delete C;
  }

  return;
}

float_t LearnerSGDE::computeResidual(Grid& grid, DataVector& alpha,
                                     DataMatrix& test, float_t lambdaReg) {
  OperationMatrix* C = computeRegularizationMatrix(grid);

  DataVector rhs(grid.getSize());
  DataVector res(grid.getSize());
  SGPP::datadriven::DensitySystemMatrix SMatrix(grid, test, *C, lambdaReg);
  SMatrix.generateb(rhs);

  SMatrix.mult(alpha, res);

  for (size_t i = 0; i < res.getSize(); i++)
    res[i] = res[i] - rhs[i];

  delete C;
  return res.l2Norm();
}

OperationMatrix* LearnerSGDE::computeRegularizationMatrix(
  SGPP::base::Grid& grid) {
  OperationMatrix* C = NULL;

  if (regularizationConfig.regType_
      == SGPP::datadriven::RegularizationType::Identity) {
    C = SGPP::op_factory::createOperationIdentity(grid);
  } else if (regularizationConfig.regType_
             == SGPP::datadriven::RegularizationType::Laplace) {
    C = SGPP::op_factory::createOperationLaplace(grid);
  } else {
    throw base::application_exception("LearnerSGDE::train : unknown regularization type");
  }

  return C;
}

void LearnerSGDE::splitset(vector<DataMatrix*>& strain,
                           vector<DataMatrix*>& stest) {
  DataMatrix* mydata = new DataMatrix(*samples);
  DataVector p(samples->getNcols());
  DataVector tmp(samples->getNcols());

  size_t kfold = learnerSGDEConfig.kfold_;

  vector<size_t> s(kfold);  // size of partition
  vector<size_t> ind(kfold + 1);  // index of partition
  size_t n = mydata->getNrows();  // size of data

  if (learnerSGDEConfig.shuffle_) {
    if (learnerSGDEConfig.seed_ == -1)
      srand(static_cast<unsigned int>(time(0)));
    else
      srand(learnerSGDEConfig.seed_);

    for (size_t i = 0; i < mydata->getNrows(); i++) {
      size_t r = i
                 + (static_cast<size_t>(rand()) % (mydata->getNrows() - i));
      mydata->getRow(i, p);
      mydata->getRow(r, tmp);
      mydata->setRow(r, p);
      mydata->setRow(i, tmp);
    }
  }

  // set size of partitions
  if (!learnerSGDEConfig.silent_)
    cout << "# kfold: ";

  ind[0] = 0;

  for (size_t i = 0; i < kfold - 1; i++) {
    s[i] = n / kfold;
    ind[i + 1] = ind[i] + s[i];

    if (!learnerSGDEConfig.silent_)
      cout << s[i] << " ";
  }

  ind[kfold] = n;
  s[kfold - 1] = n - (kfold - 1) * (n / kfold);

  if (!learnerSGDEConfig.silent_)
    cout << s[kfold - 1] << endl;

  if (!learnerSGDEConfig.silent_) {
    cout << "# kfold ind: ";

    for (size_t i = 0; i <= kfold; i++)
      cout << ind[i] << " ";

    cout << endl;
  }

  // fill data
  for (size_t i = 0; i < kfold; i++) {
    // allocate memory
    strain[i] = new DataMatrix(mydata->getNrows() - s[i],
                               mydata->getNcols());
    stest[i] = new DataMatrix(s[i], mydata->getNcols());

    size_t local_test = 0;
    size_t local_train = 0;

    for (size_t j = 0; j < mydata->getNrows(); j++) {
      mydata->getRow(j, p);

      if (ind[i] <= j && j < ind[i + 1]) {
        stest[i]->setRow(local_test, p);
        local_test++;
      } else {
        strain[i]->setRow(local_train, p);
        local_train++;
      }
    }
  }

  delete mydata;
}

base::Grid* LearnerSGDE::getGrid() {
  return grid;
}

base::DataVector* LearnerSGDE::getAlpha() {
  return &alpha;
}

}  // namespace datadriven
}  // namespace SGPP

