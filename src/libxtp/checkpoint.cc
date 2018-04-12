/*
 *            Copyright 2009-2017 The VOTCA Development Team
 *                       (http://www.votca.org)
 *
 *      Licensed under the Apache License, Version 2.0 (the "License")
 *
 * You may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *              http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include "gitversion.h"
#include <iostream>
#include <stdexcept>
#include <votca/xtp/checkpoint.h>
namespace votca {
namespace xtp {

namespace hdf5_utils {

H5::DataSpace str_scalar(H5::DataSpace(H5S_SCALAR));

void WriteScalar(const CptLoc& loc, const std::string& value,
                 const std::string& name) {

  hsize_t dims[1] = {1};
  H5::DataSpace dp(1, dims);
  const H5::DataType* strType = InferDataType<std::string>::get();

  H5::Attribute attr = loc.createAttribute(name, *strType, StrScalar());
  attr.write(*strType, &value);
}

void WriteData(const CptLoc& loc, const votca::tools::vec& v,
               const std::string& name) {

    // store tools::vec as n vector of three elements
    std::vector<double> data = {v.getX(), v.getY(), v.getZ()};
    WriteData(loc, data, name);
}

void WriteData(const CptLoc& loc, const std::vector<votca::tools::vec>& v,
               const std::string& name) {

    size_t c = 0;
    std::string r;
    CptLoc parent = loc.createGroup(name);
    for (auto const& x: v){
        r = std::to_string(c);
        WriteData(parent, x, "ind"+r);
        ++c;
    }
}

void ReadScalar(const CptLoc& loc, std::string& var, const std::string& name){
    const H5::DataType* strType = InferDataType<std::string>::get();

    H5::Attribute attr = loc.openAttribute(name);

    H5std_string readbuf("");

    attr.read(*strType, readbuf);

    var = readbuf;
}

void ReadData(const CptLoc& loc, votca::tools::vec& v,
              const std::string& name){

    // read tools::vec as a vector of three elements
    std::vector<double> data = {0,0,0};
    ReadData(loc, data, name);
    v = votca::tools::vec(data[0], data[1], data[2]);
}

void ReadData(const CptLoc& loc, std::vector<votca::tools::vec>& v,
              const std::string& name){

    CptLoc parent = loc.openGroup(name);
    size_t count = parent.getNumObjs();

    v.resize(count);

    size_t c = 0;
    for (auto &vec: v){
        ReadData(parent, vec, "ind"+std::to_string(c));
        ++c;
    }
}

}  // namespace hdf5_utils

using namespace hdf5_utils;

CheckpointFile::CheckpointFile(std::string fN)
    : _fileName(fN), _version(gitversion) {

  try {
      //H5::Exception::dontPrint();
    _fileHandle = H5::H5File(_fileName, H5F_ACC_TRUNC);

    WriteScalar(_fileHandle.openGroup("/"), gitversion, "Version");

  } catch (H5::Exception& error) {
    error.printError();
    throw std::runtime_error(error.getDetailMsg());
  }
};

std::string CheckpointFile::getFileName() { return _fileName; };
std::string CheckpointFile::getVersion() { return _version; };

H5::H5File CheckpointFile::getHandle() { return _fileHandle; };

}  // namespace xtp
}  // namespace votca
