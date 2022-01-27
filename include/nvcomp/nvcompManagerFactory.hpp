/*
 * Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *  * Neither the name of NVIDIA CORPORATION nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include "nvcompManager.hpp"
#include "lz4.hpp"
#include "snappy.hpp"
#include "bitcomp.hpp"

namespace nvcomp {

/** 
 * @brief Construct a ManagerBase from a buffer
 */ 
std::shared_ptr<nvcompManagerBase> create_manager(const uint8_t* comp_buffer, cudaStream_t stream = 0, const int device_id = 0) {
  // Need to determine the type of manager
  const CommonHeader* common_header = reinterpret_cast<const CommonHeader*>(comp_buffer);
  CommonHeader cpu_common_header;
  CudaUtils::check(cudaMemcpyAsync(&cpu_common_header, common_header, sizeof(CommonHeader), cudaMemcpyDefault, stream));

  std::shared_ptr<nvcompManagerBase> res;

  switch(cpu_common_header.format) {
    case FormatType::LZ4: 
    {
      LZ4FormatSpecHeader format_spec;
      const LZ4FormatSpecHeader* gpu_format_header = reinterpret_cast<const LZ4FormatSpecHeader*>(comp_buffer + sizeof(CommonHeader));
      CudaUtils::check(cudaMemcpyAsync(&format_spec, gpu_format_header, sizeof(LZ4FormatSpecHeader), cudaMemcpyDefault, stream));
      CudaUtils::check(cudaStreamSynchronize(stream));

      res = std::make_shared<LZ4Manager>(cpu_common_header.uncomp_chunk_size, format_spec.data_type, stream, device_id);
      break;
    }
    case FormatType::Snappy: 
    {
      SnappyFormatSpecHeader format_spec;
      const SnappyFormatSpecHeader* gpu_format_header = reinterpret_cast<const SnappyFormatSpecHeader*>(comp_buffer + sizeof(CommonHeader));
      CudaUtils::check(cudaMemcpyAsync(&format_spec, gpu_format_header, sizeof(SnappyFormatSpecHeader), cudaMemcpyDefault, stream));
      CudaUtils::check(cudaStreamSynchronize(stream));
      
      res = std::make_shared<SnappyManager>(cpu_common_header.uncomp_chunk_size, stream, device_id);
      break;
    }
    case FormatType::GDeflate: 
    {
      // TODO
      break;
    }
    case FormatType::Bitcomp: 
    {
#ifdef ENABLE_BITCOMP
      BitcompFormatSpecHeader format_spec;
      const BitcompFormatSpecHeader* gpu_format_header = reinterpret_cast<const BitcompFormatSpecHeader*>(comp_buffer + sizeof(CommonHeader));
      CudaUtils::check(cudaMemcpy(&format_spec, gpu_format_header, sizeof(BitcompFormatSpecHeader), cudaMemcpyDefault));

      res = std::make_shared<BitcompManager>(format_spec.data_type, format_spec.algo, stream, device_id);
#else
      throw NVCompException(nvcompErrorNotSupported, "Bitcomp support not available in this build.");
#endif
      break;
    }
    case FormatType::ANS: 
    {
      // TODO
      break;
    }
    case FormatType::Cascaded: 
    {
      // TODO
      break;
    }
    case FormatType::NotSupportedError:
    {
      assert(false);
    }
  }

  return res;
}

} // namespace nvcomp 
 