// Copyright (C) 2008-today The SG++ project
// This file is part of the SG++ project. For conditions of distribution and
// use, please see the copyright notice provided with SG++ or at
// sgpp.sparsegrids.org

#pragma once

#include <CL/cl.h>

#include <cstdlib>
#include <string>
#include <limits>

#include "sgpp/globaldef.hpp"
#include "sgpp/base/opencl/LinearLoadBalancer.hpp"
#include "sgpp/base/opencl/OCLClonedBuffer.hpp"
#include "sgpp/base/opencl/OCLOperationConfiguration.hpp"
#include "sgpp/base/opencl/OCLManager.hpp"
#include "sgpp/base/opencl/OCLStretchedBuffer.hpp"
#include "sgpp/base/opencl/OCLZeroCopyBuffer.hpp"
#include "AdaptiveOCLKernelSourceBuilder.hpp"

namespace SGPP {
namespace datadriven {

struct levelStruct {
  uint32_t flatLevel;
  uint32_t flatStartIndex;
};

template <typename real_type>
class AdaptiveOCLKernelImpl {
 private:
  size_t dims;

  cl_int err;
  cl_device_id* device_ids;
  cl_uint num_devices;
  //    cl_context context;
  //    cl_command_queue* command_queue;

  base::OCLZeroCopyBuffer deviceData;
  base::OCLZeroCopyBuffer deviceStreamArray;
  base::OCLZeroCopyBuffer deviceSubspaceArray;
  base::OCLZeroCopyBuffer deviceMetaInfo;

  // use pinned memory (on host and device) to speed up data transfers from/to GPU
  base::OCLStretchedBuffer deviceAlpha;
  real_type* hostAlpha;
  base::OCLStretchedBuffer deviceTemp;
  real_type* hostTemp;

  cl_kernel* kernel_multTrans;
  cl_kernel* kernel_mult;

  double* deviceTimingsMult;
  double* deviceTimingsMultTranspose;

  AdaptiveOCLKernelSourceBuilder<real_type> kernelSourceBuilder;
  std::shared_ptr<base::OCLManager> manager;
  std::shared_ptr<base::OCLOperationConfiguration> parameters;

  base::LinearLoadBalancer multLoadBalancer;
  base::LinearLoadBalancer multTransposeLoadBalancer;

 public:
  AdaptiveOCLKernelImpl(size_t dims, std::shared_ptr<base::OCLManager> manager,
                        std::shared_ptr<base::OCLOperationConfiguration> parameters)
      : deviceData(manager),
        deviceStreamArray(manager),
        deviceSubspaceArray(manager),
        deviceMetaInfo(manager),
        deviceAlpha(manager),
        deviceTemp(manager),
        kernelSourceBuilder(parameters),
        manager(manager),
        parameters(parameters),
        multLoadBalancer(manager, this->parameters),
        multTransposeLoadBalancer(manager, this->parameters) {
    this->dims = dims;
    this->num_devices = manager->num_devices;
    this->deviceTimingsMult = new double[this->num_devices];
    this->deviceTimingsMultTranspose = new double[this->num_devices];
    for (size_t i = 0; i < this->num_devices; i++) {
      // initialize with same timing to enforce equal problem sizes in the beginning
      this->deviceTimingsMult[i] = 1.0;
      this->deviceTimingsMultTranspose[i] = 1.0;
    }

    //        this->context = manager->context;
    //        this->command_queue = manager->command_queue;
    this->device_ids = manager->device_ids;
    this->err = CL_SUCCESS;

    this->hostAlpha = nullptr;
    this->hostTemp = nullptr;

    this->kernel_mult = new cl_kernel[num_devices];
    this->kernel_multTrans = new cl_kernel[num_devices];

    // initialize arrays
    for (size_t i = 0; i < num_devices; i++) {
      this->kernel_mult[i] = nullptr;
      this->kernel_multTrans[i] = nullptr;
    }
  }

  ~AdaptiveOCLKernelImpl() {
    releaseDataBuffers();
    releaseGridBuffers();

    for (size_t i = 0; i < num_devices; i++) {
      if (kernel_mult[i]) {
        clReleaseKernel(kernel_mult[i]);
        kernel_mult[i] = nullptr;
      }

      if (kernel_multTrans[i]) {
        clReleaseKernel(kernel_multTrans[i]);
        kernel_multTrans[i] = nullptr;
      }
    }

    //        // release command queue
    //        for (size_t i = 0; i < num_devices; i++) {
    //            if (command_queue[i]) {
    //                clReleaseCommandQueue(command_queue[i]);
    //            }
    //        }
    //
    //        // release context
    //        clReleaseContext(context);

    //        delete[] this->command_queue;

    delete[] this->kernel_mult;
    delete[] this->kernel_multTrans;

    delete[] this->deviceTimingsMult;
    delete[] this->deviceTimingsMultTranspose;

    delete[] this->device_ids;
  }

  void resetKernel() {
    // leads to a reallocation before next kernel execution
    releaseGridBuffers();
  }

  double mult(real_type* streamingArray, real_type* subspaceArray, uint32_t* metaInfo,
              size_t gridSize, real_type* dataset, size_t datasetSize, real_type* alpha,
              real_type* result, const size_t start_index_grid, const size_t end_index_grid,
              const size_t start_index_data, const size_t end_index_data, size_t numSubspaces,
              uint32_t numStreamingSubs, uint32_t numSubSubs) {
    // check if there is something to do at all
    if (!(end_index_grid > start_index_grid && end_index_data > start_index_data)) {
      return 0.0;
    }

    double time = 0.0;

    if (kernel_mult[0] == nullptr) {
      this->createMult(this->dims, (*parameters)["LOCAL_SIZE"].getUInt(), manager->context,
                       num_devices, device_ids, kernel_mult);
    }

    initOCLBuffers(streamingArray, subspaceArray, metaInfo, gridSize, dataset, datasetSize,
                   numSubspaces, numStreamingSubs, numSubSubs);
    initParams(alpha, gridSize, result, datasetSize);

    // determine best fit
    size_t* gpu_start_index_data = new size_t[num_devices];
    size_t* gpu_end_index_data = new size_t[num_devices];

    multLoadBalancer.update(this->deviceTimingsMult);
    multLoadBalancer.getPartitionSegments(start_index_data, end_index_data,
                                          (*parameters)["LOCAL_SIZE"].getUInt(),
                                          gpu_start_index_data, gpu_end_index_data);

    // set kernel arguments
    cl_uint clResultSize = (cl_uint)datasetSize;
    //        cl_uint numDims = (cl_uint) this->dims;
    cl_uint clNumSubspaces = (cl_uint)numSubspaces;
    //    std::cout << "start grid: " << gpu_start_grid << " end grid: " << gpu_end_grid <<
    //    std::endl;

    for (size_t i = 0; i < num_devices; i++) {
      cl_uint gpu_start_data = (cl_uint)gpu_start_index_data[i];
      cl_uint gpu_end_data = (cl_uint)gpu_end_index_data[i];
      //      std::cout << "device: " << i << " start data: " << gpu_start_data << " end data: " <<
      //      gpu_end_data << std::endl;

      if (gpu_end_data > gpu_start_data) {
        if (clSetKernelArg(kernel_mult[i], 0, sizeof(cl_mem),
                           this->deviceStreamArray.getBuffer(i)) ||
            clSetKernelArg(kernel_mult[i], 1, sizeof(cl_mem),
                           this->deviceSubspaceArray.getBuffer(i)) ||
            clSetKernelArg(kernel_mult[i], 2, sizeof(cl_mem), this->deviceMetaInfo.getBuffer(i)) ||
            clSetKernelArg(kernel_mult[i], 3, sizeof(cl_mem), this->deviceData.getBuffer(i)) ||
            clSetKernelArg(kernel_mult[i], 4, sizeof(cl_mem), this->deviceAlpha.getBuffer(i)) ||
            clSetKernelArg(kernel_mult[i], 5, sizeof(cl_mem), this->deviceTemp.getBuffer(i)) ||
            clSetKernelArg(kernel_mult[i], 6, sizeof(cl_uint),
                           &clResultSize) ||  // resultsize == number of entries in dataset
            clSetKernelArg(kernel_mult[i], 7, sizeof(cl_uint), &clNumSubspaces) != CL_SUCCESS) {
          std::stringstream errorString;
          errorString << "OCL Error: Failed to create kernel arguments for kernel " << i
                      << std::endl;
          throw SGPP::base::operation_exception(errorString.str());
        }
      }
    }

    cl_event* clTimings = new cl_event[num_devices];

    // enqueue kernel
    size_t local = (*parameters)["LOCAL_SIZE"].getUInt();
    size_t active_devices = 0;

    for (size_t i = 0; i < num_devices; i++) {
      size_t rangeSize = gpu_end_index_data[i] - gpu_start_index_data[i];

      if (rangeSize > 0) {
        err = clEnqueueNDRangeKernel(manager->command_queue[i], kernel_mult[i], 1,
                                     &gpu_start_index_data[i], &rangeSize, &local, 0, nullptr,
                                     &(clTimings[i]));

        if (active_devices != i) {
          std::stringstream errorString;
          errorString << "OCL Error: Multiple GPUs is erroneous, because only the last chunks may "
                         "handle 0 entries. Synchronization will be not correct." << std::endl;
          throw SGPP::base::operation_exception(errorString.str());
        }

        active_devices++;

        if (err != CL_SUCCESS) {
          std::stringstream errorString;
          errorString << "OCL Error: Failed to enqueue kernel command! Error code: " << err
                      << std::endl;
          throw SGPP::base::operation_exception(errorString.str());
        }
      }
    }

    deviceTemp.readFromBuffer(gpu_start_index_data, gpu_end_index_data);
    for (size_t i = start_index_data; i < end_index_data; i++) {
      result[i] = hostTemp[i];
    }

    // determine kernel execution time
    for (size_t i = 0; i < num_devices; i++) {
      double tmpTime;
      cl_ulong startTime, endTime;
      startTime = endTime = 0;

      if (gpu_end_index_data[i] > gpu_start_index_data[i]) {
        err = clGetEventProfilingInfo(clTimings[i], CL_PROFILING_COMMAND_START, sizeof(cl_ulong),
                                      &startTime, nullptr);

        if (err != CL_SUCCESS) {
          std::stringstream errorString;
          errorString << "OCL Error: Failed to read start-time from command queue (or crash in "
                         "mult)! Error code: " << err << std::endl;
          throw SGPP::base::operation_exception(errorString.str());
        }

        err = clGetEventProfilingInfo(clTimings[i], CL_PROFILING_COMMAND_END, sizeof(cl_ulong),
                                      &endTime, nullptr);

        if (err != CL_SUCCESS) {
          std::stringstream errorString;
          errorString << "OCL Error: Failed to read end-time from command queue! Error code: "
                      << err << std::endl;
          throw SGPP::base::operation_exception(errorString.str());
        }
      }

      tmpTime = static_cast<double>(endTime - startTime);
      tmpTime *= 1e-9;
      this->deviceTimingsMult[i] = tmpTime;

      if (tmpTime > time) {
        time = tmpTime;
      }
    }

    // clean up
    for (size_t i = 0; i < num_devices; i++) {
      if (gpu_end_index_data[i] > gpu_start_index_data[i]) {
        clReleaseEvent(clTimings[i]);
      }
    }

    delete[] gpu_start_index_data;
    delete[] gpu_end_index_data;

    delete[] clTimings;

    return time;
  }

  double multTranspose(real_type* streamingArray, real_type* subspaceArray, uint32_t* metaInfo,
                       size_t gridSize, real_type* dataset, size_t datasetSize, real_type* source,
                       real_type* result, const size_t start_index_grid,
                       const size_t end_index_grid, const size_t start_index_data,
                       const size_t end_index_data, size_t numSubspaces, uint32_t numStreamingSubs,
                       uint32_t numSubSubs) {
    // check if there is something to do at all
    if (!(end_index_grid > start_index_grid && end_index_data > start_index_data)) {
      return 0.0;
    }

    double time = 0.0;

    if (kernel_multTrans[0] == nullptr) {
      this->createMultTrans(this->dims, (*parameters)["LOCAL_SIZE"].getUInt(), manager->context,
                            num_devices, device_ids, kernel_multTrans);
    }

    initOCLBuffers(streamingArray, subspaceArray, metaInfo, gridSize, dataset, datasetSize,
                   numSubspaces, numStreamingSubs, numSubSubs);
    initParams(result, gridSize, source, datasetSize);

    // determine best fit
    size_t* gpu_start_index_grid = new size_t[num_devices];
    size_t* gpu_end_index_grid = new size_t[num_devices];

    multTransposeLoadBalancer.update(this->deviceTimingsMultTranspose);
    multTransposeLoadBalancer.getPartitionSegments(start_index_grid, end_index_grid,
                                                   (*parameters)["LOCAL_SIZE"].getUInt(),
                                                   gpu_start_index_grid, gpu_end_index_grid);

    // determine best fit
    size_t* gpu_start_index_data = new size_t[num_devices];
    size_t* gpu_end_index_data = new size_t[num_devices];

    multLoadBalancer.update(this->deviceTimingsMult);
    multLoadBalancer.getPartitionSegments(start_index_data, end_index_data,
                                          (*parameters)["LOCAL_SIZE"].getUInt(),
                                          gpu_start_index_data, gpu_end_index_data);

    // set kernel arguments
    cl_uint clSourceSize = (cl_uint)datasetSize;
    //        cl_uint gpu_start_data = (cl_uint) start_index_data;
    //        cl_uint gpu_end_data = (cl_uint) end_index_data;
    //        cl_uint numDims = (cl_uint) this->dims;

    //    std::cout << "start data: " << gpu_start_data << " end data: " << gpu_end_data <<
    //    std::endl;

    for (size_t i = 0; i < num_devices; i++) {
      /*cl_uint gpu_start_grid = (cl_uint) gpu_start_index_grid[i];
       cl_uint gpu_end_grid = (cl_uint) gpu_end_index_grid[i];*/
      cl_uint gpu_start_data = (cl_uint)gpu_start_index_data[i];
      cl_uint gpu_end_data = (cl_uint)gpu_end_index_data[i];
      //      std::cout << "device: " << i << " start grid: " << gpu_start_grid << " end grid: " <<
      //      gpu_end_grid << std::endl;

      if (gpu_end_data > gpu_start_data) {
        if (clSetKernelArg(kernel_multTrans[i], 0, sizeof(cl_mem),
                           this->deviceStreamArray.getBuffer(i)) ||
            clSetKernelArg(kernel_multTrans[i], 1, sizeof(cl_mem),
                           this->deviceSubspaceArray.getBuffer(i)) ||
            clSetKernelArg(kernel_multTrans[i], 2, sizeof(cl_mem),
                           this->deviceMetaInfo.getBuffer(i)) ||
            clSetKernelArg(kernel_multTrans[i], 3, sizeof(cl_mem), this->deviceData.getBuffer(i)) ||
            clSetKernelArg(kernel_multTrans[i], 4, sizeof(cl_mem), this->deviceTemp.getBuffer(i)) ||
            clSetKernelArg(kernel_multTrans[i], 5, sizeof(cl_mem),
                           this->deviceAlpha.getBuffer(i)) ||
            clSetKernelArg(kernel_multTrans[i], 6, sizeof(cl_uint),
                           &clSourceSize) ||  // sourceSize == number of entries in dataset
            clSetKernelArg(kernel_multTrans[i], 7, sizeof(cl_uint), &numSubspaces) != CL_SUCCESS) {
          std::stringstream errorString;
          errorString << "OCL Error: Failed to create kernel arguments for kernel " << i
                      << std::endl;
          throw SGPP::base::operation_exception(errorString.str());
        }
      }
    }

    cl_event* clTimings = new cl_event[num_devices];

    // enqueue kernels
    size_t local = (*parameters)["LOCAL_SIZE"].getUInt();
    size_t active_devices = 0;

    for (size_t i = 0; i < num_devices; i++) {
      // size_t rangeSize = gpu_end_index_grid[i] - gpu_start_index_grid[i];
      size_t rangeSize = gpu_end_index_data[i] - gpu_start_index_data[i];

      if (rangeSize > 0) {
        err = clEnqueueNDRangeKernel(manager->command_queue[i], kernel_multTrans[i], 1,
                                     &gpu_start_index_data[i], &rangeSize, &local, 0, nullptr,
                                     &(clTimings[i]));

        if (active_devices != i) {
          std::stringstream errorString;
          errorString << "OCL Error: Multiple GPUs is erroneous, because only the last chunks may "
                         "handle 0 entries. Synchronization will be not correct." << std::endl;
          throw SGPP::base::operation_exception(errorString.str());
        }

        active_devices++;

        if (err != CL_SUCCESS) {
          std::stringstream errorString;
          errorString << "OCL Error: Failed to enqueue kernel command! Error code: " << err
                      << std::endl;
          throw SGPP::base::operation_exception(errorString.str());
        }
      }
    }

    deviceAlpha.readFromBuffer(gpu_start_index_grid, gpu_end_index_grid);

    for (size_t i = start_index_grid; i < end_index_grid; i++) {
      result[i] = hostAlpha[i];
    }

    // determine kernel execution time
    for (size_t i = 0; i < num_devices; i++) {
      double tmpTime;
      cl_ulong startTime, endTime;
      startTime = endTime = 0;

      if (gpu_end_index_data[i] > gpu_start_index_data[i]) {
        err = clGetEventProfilingInfo(clTimings[i], CL_PROFILING_COMMAND_START, sizeof(cl_ulong),
                                      &startTime, nullptr);

        if (err != CL_SUCCESS) {
          std::stringstream errorString;
          errorString << "OCL Error: Failed to read start-time from command queue (or crash in "
                         "multTranspose)! Error code: " << err << std::endl;
          throw SGPP::base::operation_exception(errorString.str());
        }

        err = clGetEventProfilingInfo(clTimings[i], CL_PROFILING_COMMAND_END, sizeof(cl_ulong),
                                      &endTime, nullptr);

        if (err != CL_SUCCESS) {
          std::stringstream errorString;
          errorString << "OCL Error: Failed to read end-time from command queue! Error code: "
                      << err << std::endl;
          throw SGPP::base::operation_exception(errorString.str());
        }
      }

      tmpTime = static_cast<double>(endTime - startTime);
      tmpTime *= 1e-9;
      this->deviceTimingsMultTranspose[i] = tmpTime;

      if (tmpTime > time) {
        time = tmpTime;
      }
    }

    // clean up
    for (size_t i = 0; i < num_devices; i++) {
      if (gpu_end_index_data[i] > gpu_start_index_data[i]) {
        clReleaseEvent(clTimings[i]);
      }
    }

    delete[] gpu_start_index_data;
    delete[] gpu_end_index_data;

    delete[] clTimings;

    return time;
  }

 private:
  void createMultTrans(size_t dims, size_t local_workgroup_size, cl_context context,
                       size_t num_devices, cl_device_id* device_ids, cl_kernel* kernel) {
    std::string program_src = kernelSourceBuilder.generateSourceMultTrans(dims);
    manager->buildKernel(program_src, "multTransOCL", context, num_devices, device_ids, kernel);
  }

  void createMult(size_t dims, size_t local_workgroup_size, cl_context context, size_t num_devices,
                  cl_device_id* device_ids, cl_kernel* kernel) {
    std::string program_src = kernelSourceBuilder.generateSourceMult(dims);
    manager->buildKernel(program_src, "multOCL", context, num_devices, device_ids, kernel);
  }

  void releaseGridBuffers() {
    this->deviceStreamArray.freeBuffer();
    this->deviceSubspaceArray.freeBuffer();
    this->deviceAlpha.freeBuffer();
    this->deviceMetaInfo.freeBuffer();
  }

  void releaseDataBuffers() {
    this->deviceData.freeBuffer();
    this->deviceTemp.freeBuffer();
  }

  void initOCLBuffers(real_type* streamingArray, real_type* subspaceArray, uint32_t* metaInfo,
                      size_t gridSize, real_type* dataset, size_t datasetSize, size_t numSubspaces,
                      uint32_t numStreamSubs, uint32_t numSubSubs) {
    // Hack to prevent oclmanager to initialize a zero-length array
    // TODO(pfandedd): replace hack with proper logic
    if (numStreamSubs == 0 && streamingArray != nullptr &&
        !this->deviceStreamArray.isInitialized()) {
      printf("WARNING! No subspaces evaluated using streaming layout \n");
      real_type dummyArray[5] = {0.0, 0.0, 0.0, 0.0, 0.0};
      this->deviceStreamArray.initializeBuffer(dummyArray, sizeof(real_type), 5, true);
    } else if (streamingArray != nullptr && !this->deviceStreamArray.isInitialized()) {
      // Initialize device memory
      this->deviceStreamArray.initializeBuffer(streamingArray, sizeof(real_type),
                                               numStreamSubs * (this->dims + 1), true);
    }

    if (numSubSubs == 0 && subspaceArray != nullptr && !this->deviceSubspaceArray.isInitialized()) {
      printf("WARNING! No subspaces evaluated using subspace layout \n");
      real_type dummyArray[5] = {0.0, 0.0, 0.0, 0.0, 0.0};
      this->deviceSubspaceArray.initializeBuffer(dummyArray, sizeof(real_type), 5, true);
    } else if (subspaceArray != nullptr && !this->deviceSubspaceArray.isInitialized()) {
      this->deviceSubspaceArray.initializeBuffer(subspaceArray, sizeof(real_type),
                                                 numSubSubs * (this->dims + 1), true);
    }

    if (metaInfo != nullptr && !this->deviceMetaInfo.isInitialized()) {
      this->deviceMetaInfo.initializeBuffer(metaInfo, sizeof(uint32_t),
                                            numSubspaces * (this->dims + 3), true);
    }

    if (!this->deviceData.isInitialized()) {
      this->deviceData.initializeBuffer(dataset, sizeof(real_type), datasetSize * this->dims, true);
    }
  }

  void initParams(real_type* alpha, size_t alphaSize, real_type* tmp, size_t datasetSize) {
    if (!this->deviceAlpha.isInitialized()) {
      this->deviceAlpha.initializeBuffer(sizeof(real_type), alphaSize * this->dims);
      this->hostAlpha = reinterpret_cast<real_type*>(this->deviceAlpha.getMappedHostBuffer());
    }

    for (size_t i = 0; i < alphaSize; i++) {
      this->hostAlpha[i] = alpha[i];
    }
    deviceAlpha.writeToBuffer();

    if (!this->deviceTemp.isInitialized()) {
      this->deviceTemp.initializeBuffer(sizeof(real_type), datasetSize * this->dims);
      this->hostTemp = reinterpret_cast<real_type*>(this->deviceTemp.getMappedHostBuffer());
    }

    for (size_t i = 0; i < datasetSize; i++) {
      this->hostTemp[i] = tmp[i];
    }
    deviceTemp.writeToBuffer();
  }
};
}  // namespace datadriven
}  // namespace SGPP
