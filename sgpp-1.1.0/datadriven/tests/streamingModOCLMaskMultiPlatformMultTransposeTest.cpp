// Copyright (C) 2008-today The SG++ project
// This file is part of the SG++ project. For conditions of distribution and
// use, please see the copyright notice provided with SG++ or at
// sgpp.sparsegrids.org

#if USE_OCL == 1

#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include <zlib.h>

#include <random>
#include <fstream>
#include <iostream>
#include <string>
#include <tuple>
#include <vector>

#include "test_datadrivenCommon.hpp"
#include "sgpp/globaldef.hpp"
#include "sgpp/base/operation/hash/OperationMultipleEval.hpp"
#include "sgpp/datadriven/DatadrivenOpFactory.hpp"
#include "sgpp/base/operation/BaseOpFactory.hpp"
#include "sgpp/datadriven/tools/ARFFTools.hpp"
#include "sgpp/base/grid/generation/functors/SurplusRefinementFunctor.hpp"
#include "sgpp/base/tools/ConfigurationParameters.hpp"

using SGPP::datadriven::OperationMultipleEvalConfiguration;
using SGPP::datadriven::OperationMultipleEvalType;
using SGPP::datadriven::OperationMultipleEvalSubType;

BOOST_AUTO_TEST_SUITE(TestStreamingModOCLMaskMultiPlatformMultTranspose)

BOOST_AUTO_TEST_CASE(Simple) {
  std::vector<std::tuple<std::string, double>> fileNamesError = {
      std::tuple<std::string, double>("datadriven/tests/data/friedman_4d.arff.gz", 1E-12),
      std::tuple<std::string, double>("datadriven/tests/data/friedman_10d.arff.gz", 1E-12)};

  uint32_t level = 5;

  std::shared_ptr<SGPP::base::OCLOperationConfiguration> parameters =
      getConfigurationDefaultsSingleDevice();

  (*parameters)["INTERNAL_PRECISION"].set("double");

  std::vector<std::reference_wrapper<json::Node>> deviceNodes = parameters->getAllDeviceNodes();
  for (json::Node &deviceNode : deviceNodes) {
    // TODO(pfandedd): add setup of blocking for all tests
    deviceNode.replaceIDAttr("KERNEL_USE_LOCAL_MEMORY", false);
    deviceNode.replaceIDAttr("KERNEL_MAX_DIM_UNROLL", 1ul);
  }

  OperationMultipleEvalConfiguration configuration(
      OperationMultipleEvalType::STREAMING, OperationMultipleEvalSubType::OCLMASKMP, *parameters);

  for (std::tuple<std::string, double> fileNameError : fileNamesError) {
    double mse = compareToReferenceTranspose(SGPP::base::GridType::ModLinear,
                                             std::get<0>(fileNameError), level, configuration);
    BOOST_CHECK(mse < std::get<1>(fileNameError));
  }
}

BOOST_AUTO_TEST_CASE(Local) {
  std::vector<std::tuple<std::string, double>> fileNamesError = {
      std::tuple<std::string, double>("datadriven/tests/data/friedman_4d.arff.gz", 1E-12),
      std::tuple<std::string, double>("datadriven/tests/data/friedman_10d.arff.gz", 1E-12)};

  uint32_t level = 5;

  std::shared_ptr<SGPP::base::OCLOperationConfiguration> parameters =
      getConfigurationDefaultsSingleDevice();

  (*parameters)["INTERNAL_PRECISION"].set("double");

  std::vector<std::reference_wrapper<json::Node>> deviceNodes = parameters->getAllDeviceNodes();
  for (json::Node &deviceNode : deviceNodes) {
    deviceNode.replaceIDAttr("KERNEL_USE_LOCAL_MEMORY", true);
    deviceNode.replaceIDAttr("KERNEL_MAX_DIM_UNROLL", 10ul);
  }

  OperationMultipleEvalConfiguration configuration(
      OperationMultipleEvalType::STREAMING, OperationMultipleEvalSubType::OCLMASKMP, *parameters);

  for (std::tuple<std::string, double> fileNameError : fileNamesError) {
    double mse = compareToReferenceTranspose(SGPP::base::GridType::ModLinear,
                                             std::get<0>(fileNameError), level, configuration);
    BOOST_CHECK(mse < std::get<1>(fileNameError));
  }
}

BOOST_AUTO_TEST_CASE(MultiDevice) {
  std::vector<std::tuple<std::string, double>> fileNamesError = {
      std::tuple<std::string, double>("datadriven/tests/data/friedman_4d.arff.gz", 1E-12),
      std::tuple<std::string, double>("datadriven/tests/data/friedman_10d.arff.gz", 1E-12)};

  uint32_t level = 5;

  std::shared_ptr<SGPP::base::OCLOperationConfiguration> parameters =
      getConfigurationDefaultsMultiDevice();

  (*parameters)["INTERNAL_PRECISION"].set("double");

  std::vector<std::reference_wrapper<json::Node>> deviceNodes = parameters->getAllDeviceNodes();
  for (json::Node &deviceNode : deviceNodes) {
    deviceNode.replaceIDAttr("KERNEL_USE_LOCAL_MEMORY", true);
    deviceNode.replaceIDAttr("KERNEL_MAX_DIM_UNROLL", 10ul);
  }

  OperationMultipleEvalConfiguration configuration(
      OperationMultipleEvalType::STREAMING, OperationMultipleEvalSubType::OCLMASKMP, *parameters);

  for (std::tuple<std::string, double> fileNameError : fileNamesError) {
    double mse = compareToReferenceTranspose(SGPP::base::GridType::ModLinear,
                                             std::get<0>(fileNameError), level, configuration);
    BOOST_CHECK(mse < std::get<1>(fileNameError));
  }
}

BOOST_AUTO_TEST_CASE(MultiPlatform) {
  std::vector<std::tuple<std::string, double>> fileNamesError = {
      std::tuple<std::string, double>("datadriven/tests/data/friedman_4d.arff.gz", 1E-12),
      std::tuple<std::string, double>("datadriven/tests/data/friedman_10d.arff.gz", 1E-12)};

  uint32_t level = 5;

  std::shared_ptr<SGPP::base::OCLOperationConfiguration> parameters =
      getConfigurationDefaultsMultiPlatform();

  (*parameters)["INTERNAL_PRECISION"].set("double");

  std::vector<std::reference_wrapper<json::Node>> deviceNodes = parameters->getAllDeviceNodes();
  for (json::Node &deviceNode : deviceNodes) {
    deviceNode.replaceIDAttr("KERNEL_USE_LOCAL_MEMORY", true);
    deviceNode.replaceIDAttr("KERNEL_MAX_DIM_UNROLL", 10ul);
  }

  OperationMultipleEvalConfiguration configuration(
      OperationMultipleEvalType::STREAMING, OperationMultipleEvalSubType::OCLMASKMP, *parameters);

  for (std::tuple<std::string, double> fileNameError : fileNamesError) {
    double mse = compareToReferenceTranspose(SGPP::base::GridType::ModLinear,
                                             std::get<0>(fileNameError), level, configuration);
    BOOST_CHECK(mse < std::get<1>(fileNameError));
  }
}

BOOST_AUTO_TEST_CASE(SimpleSinglePrecision) {
  std::vector<std::tuple<std::string, double>> fileNamesError = {
      std::tuple<std::string, double>("datadriven/tests/data/friedman_4d.arff.gz", 1E+7),
      std::tuple<std::string, double>("datadriven/tests/data/friedman_10d.arff.gz", 1E+6)};

  uint32_t level = 5;

  std::shared_ptr<SGPP::base::OCLOperationConfiguration> parameters =
      getConfigurationDefaultsSingleDevice();

  (*parameters)["INTERNAL_PRECISION"].set("float");

  std::vector<std::reference_wrapper<json::Node>> deviceNodes = parameters->getAllDeviceNodes();
  for (json::Node &deviceNode : deviceNodes) {
    deviceNode.replaceIDAttr("KERNEL_USE_LOCAL_MEMORY", false);
    deviceNode.replaceIDAttr("KERNEL_MAX_DIM_UNROLL", 1ul);
  }

  OperationMultipleEvalConfiguration configuration(
      OperationMultipleEvalType::STREAMING, OperationMultipleEvalSubType::OCLMASKMP, *parameters);

  for (std::tuple<std::string, double> fileNameError : fileNamesError) {
    double mse = compareToReferenceTranspose(SGPP::base::GridType::ModLinear,
                                             std::get<0>(fileNameError), level, configuration);
    BOOST_CHECK(mse < std::get<1>(fileNameError));
  }
}

BOOST_AUTO_TEST_CASE(LocalSinglePrecision) {
  std::vector<std::tuple<std::string, double>> fileNamesError = {
      std::tuple<std::string, double>("datadriven/tests/data/friedman_4d.arff.gz", 1E+7),
      std::tuple<std::string, double>("datadriven/tests/data/friedman_10d.arff.gz", 1E+6)};

  uint32_t level = 5;

  std::shared_ptr<SGPP::base::OCLOperationConfiguration> parameters =
      getConfigurationDefaultsSingleDevice();

  (*parameters)["INTERNAL_PRECISION"].set("float");

  std::vector<std::reference_wrapper<json::Node>> deviceNodes = parameters->getAllDeviceNodes();
  for (json::Node &deviceNode : deviceNodes) {
    deviceNode.replaceIDAttr("KERNEL_USE_LOCAL_MEMORY", true);
    deviceNode.replaceIDAttr("KERNEL_MAX_DIM_UNROLL", 10ul);
  }

  OperationMultipleEvalConfiguration configuration(
      OperationMultipleEvalType::STREAMING, OperationMultipleEvalSubType::OCLMASKMP, *parameters);

  for (std::tuple<std::string, double> fileNameError : fileNamesError) {
    double mse = compareToReferenceTranspose(SGPP::base::GridType::ModLinear,
                                             std::get<0>(fileNameError), level, configuration);
    BOOST_CHECK(mse < std::get<1>(fileNameError));
  }
}

BOOST_AUTO_TEST_CASE(MultiDeviceSinglePrecision) {
  std::vector<std::tuple<std::string, double>> fileNamesError = {
      std::tuple<std::string, double>("datadriven/tests/data/friedman_4d.arff.gz", 1E+7),
      std::tuple<std::string, double>("datadriven/tests/data/friedman_10d.arff.gz", 1E+6)};

  uint32_t level = 5;

  std::shared_ptr<SGPP::base::OCLOperationConfiguration> parameters =
      getConfigurationDefaultsMultiDevice();

  (*parameters)["INTERNAL_PRECISION"].set("float");

  std::vector<std::reference_wrapper<json::Node>> deviceNodes = parameters->getAllDeviceNodes();
  for (json::Node &deviceNode : deviceNodes) {
    deviceNode.replaceIDAttr("KERNEL_USE_LOCAL_MEMORY", true);
    deviceNode.replaceIDAttr("KERNEL_MAX_DIM_UNROLL", 10ul);
  }

  OperationMultipleEvalConfiguration configuration(
      OperationMultipleEvalType::STREAMING, OperationMultipleEvalSubType::OCLMASKMP, *parameters);

  for (std::tuple<std::string, double> fileNameError : fileNamesError) {
    double mse = compareToReferenceTranspose(SGPP::base::GridType::ModLinear,
                                             std::get<0>(fileNameError), level, configuration);
    BOOST_CHECK(mse < std::get<1>(fileNameError));
  }
}

BOOST_AUTO_TEST_CASE(MultiPlatformSinglePrecision) {
  std::vector<std::tuple<std::string, double>> fileNamesError = {
      std::tuple<std::string, double>("datadriven/tests/data/friedman_4d.arff.gz", 1E+7),
      std::tuple<std::string, double>("datadriven/tests/data/friedman_10d.arff.gz", 1E+6)};

  uint32_t level = 5;

  std::shared_ptr<SGPP::base::OCLOperationConfiguration> parameters =
      getConfigurationDefaultsMultiPlatform();

  (*parameters)["INTERNAL_PRECISION"].set("float");

  std::vector<std::reference_wrapper<json::Node>> deviceNodes = parameters->getAllDeviceNodes();
  for (json::Node &deviceNode : deviceNodes) {
    deviceNode.replaceIDAttr("KERNEL_USE_LOCAL_MEMORY", true);
    deviceNode.replaceIDAttr("KERNEL_MAX_DIM_UNROLL", 10ul);
  }

  OperationMultipleEvalConfiguration configuration(
      OperationMultipleEvalType::STREAMING, OperationMultipleEvalSubType::OCLMASKMP, *parameters);

  for (std::tuple<std::string, double> fileNameError : fileNamesError) {
    double mse = compareToReferenceTranspose(SGPP::base::GridType::ModLinear,
                                             std::get<0>(fileNameError), level, configuration);
    BOOST_CHECK(mse < std::get<1>(fileNameError));
  }
}

BOOST_AUTO_TEST_SUITE_END()

#endif
