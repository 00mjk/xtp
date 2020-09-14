/*
 *            Copyright 2009-2020 The VOTCA Development Team
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

#pragma once
#ifndef VOTCA_XTP_QUADRATURE_FACTORY_H
#define VOTCA_XTP_QUADRATURE_FACTORY_H

// Standard includes
#include <map>

// VOTCA includes
#include <votca/tools/objectfactory.h>

// Local VOTCA includes
#include "GaussianQuadratureBase.h"

namespace votca {
namespace xtp {

class QuadratureFactory
    : public tools::ObjectFactory<std::string, GaussianQuadratureBase> {
 private:
  QuadratureFactory() = default;

 public:
  static void RegisterAll(void);

  /**
     Create an instance of the object identified by key.
  *  Overwritten to load calculator defaults
  */
  GaussianQuadratureBase *Create(const std::string &key);

  friend QuadratureFactory &Quadratures();
};

inline QuadratureFactory &Quadratures() {
  static QuadratureFactory _instance;
  return _instance;
}

inline GaussianQuadratureBase *QuadratureFactory::Create(
    const std::string &key) {
  assoc_map::const_iterator it(getObjects().find(key));
  if (it != getObjects().end()) {
    GaussianQuadratureBase *quad = (it->second)();
    return quad;
  } else {
    throw std::runtime_error("factory key " + key + " not found.");
  }
}

}  // namespace xtp
}  // namespace votca

#endif  // VOTCA_XTP_QUADRATURE_FACTORY_H
