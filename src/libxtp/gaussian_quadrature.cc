/*
 *            Copyright 2009-2019 The VOTCA Development Team
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

#include <votca/tools/constants.h>
#include <votca/xtp/gauss_hermite_quadrature_constants.h>
#include <votca/xtp/gauss_laguerre_quadrature_constants.h>
#include <votca/xtp/gauss_legendre_quadrature_constants.h>
#include <votca/xtp/gaussian_quadrature.h>
#include <votca/xtp/threecenter.h>

namespace votca {
namespace xtp {

// Constructor
GaussianQuadrature::GaussianQuadrature(const Eigen::VectorXd& energies,
                                       const TCMatrix_gwbse& Mmn)
    : _energies(energies), _Mmn(Mmn) {}

void GaussianQuadrature::configure(options opt, const RPA& rpa) {
  _opt = opt;
  if (_opt.quadrature_scheme == "laguerre") {
    Gauss_Laguerre_Quadrature_Constants glqc;
    _quadpoints = glqc.getPoints(_opt.order);
    _quadadaptedweights = glqc.getAdaptedWeights(_opt.order);
    CalcDielInvVector(rpa);
  } else if (_opt.quadrature_scheme == "modified_legendre") {
    Gauss_Legendre_Quadrature_Constants glqc;
    _quadpoints = glqc.getPoints(_opt.order);
    _quadadaptedweights = glqc.getAdaptedWeights(_opt.order);
    CalcDielInvVector(rpa);
  } else if (_opt.quadrature_scheme == "hermite") {
    Gauss_Hermite_Quadrature_Constants glqc;
    _quadpoints = glqc.getPoints(_opt.order);
    _quadadaptedweights = glqc.getAdaptedWeights(_opt.order);
    CalcDielInvVector(rpa);
  } else {
    std::cout << "There no such a thing as the integration scheme you asked"
              << std::endl;
  }
}

// This function calculates and stores inverses of the microscopic dielectric
// matrix in a matrix vector
void GaussianQuadrature::CalcDielInvVector(const RPA& rpa) {
  _dielinv_matrices_r.resize(_opt.order);

#pragma openmp parallel schedule(guided)
  for (Index j = 0; j < _opt.order; j++) {
    if (_opt.quadrature_scheme == "modified_legendre") {
      double exponent = (1.0 + _quadpoints(j)) / (1.0 - _quadpoints(j));
      double newpoint = std::pow(0.5, exponent);
      Eigen::MatrixXcd eps_inv_j =
        rpa.calculate_epsilon_complex(0.0, newpoint).inverse();
    eps_inv_j.diagonal().array() -= 1.0;
    _dielinv_matrices_r[j] = -eps_inv_j;
    } else {
      double newpoint = _quadpoints(j);
      Eigen::MatrixXcd eps_inv_j =
        rpa.calculate_epsilon_complex(0.0, newpoint).inverse();
    eps_inv_j.diagonal().array() -= 1.0;
    _dielinv_matrices_r[j] = -eps_inv_j;
    }
 }
}

double GaussianQuadrature::SigmaGQDiag(double frequency, Index gw_level,
                                       double eta) const {
  Index homo = _opt.homo - _opt.rpamin;
  Index lumo = homo + 1;

  const Eigen::MatrixXd& Imx = _Mmn[gw_level];
  Eigen::ArrayXcd DeltaE = frequency - _energies.array();
  std::complex<double> result = std::complex<double>(0.0, 0.0);
 

  if (_opt.quadrature_scheme == "modified_legendre") {
    //This work for integration limits a = 0 b = +infty

    for (Index k = 0; k < lumo; ++k) {
      DeltaE(k) += std::complex<double>(0.0, 1.0 * eta);
    }
    for (Index k = lumo; k < _energies.size(); ++k) {
      DeltaE(k) += std::complex<double>(0.0, -1.0 * eta);
    }

    for (Index j = 0; j < _opt.order; ++j) {
      double exponent = (1.0 + _quadpoints(j)) / (1.0 - _quadpoints(j));
      double newpoint = std::pow(0.5, exponent);
      double den =
          (1.0 - _quadadaptedweights(j)) * (1.0 - _quadadaptedweights(j));
      double newweight = (2.0 * _quadadaptedweights(j) * 0.5) / den;
      Eigen::VectorXcd denominator1 =
          (1.0) / (DeltaE + std::complex<double>(0.0, newpoint));
      Eigen::MatrixXcd Amx = denominator1.asDiagonal() * Imx;
      Eigen::MatrixXcd Cmx = Imx * (_dielinv_matrices_r[j]);
      std::complex<double> value1 = (Cmx.cwiseProduct(Amx)).sum();
      Eigen::VectorXcd denominator2 =
          (1.0) / (DeltaE + std::complex<double>(0.0, -newpoint));
      Eigen::MatrixXcd Dmx = denominator2.asDiagonal() * Imx;
      Eigen::MatrixXcd Emx = Imx * (_dielinv_matrices_r[j].conjugate());
      std::complex<double> value2 = (Emx.cwiseProduct(Dmx)).sum();
     
result += newweight * (value1+value2);
    }

result *= 0.5;

  } else if (_opt.quadrature_scheme == "hermite") {
    //This work for integration limits a = -infty b = +infty

    for (Index k = 0; k < lumo; ++k) {
      DeltaE(k) += std::complex<double>(0.0, 1.0 * eta);
    }
    for (Index k = lumo; k < _energies.size(); ++k) {
      DeltaE(k) += std::complex<double>(0.0, -1.0 * eta);
    }

    for (Index j = 0; j < _opt.order; ++j) {

      Eigen::VectorXcd denominator =
          (1.0) / (DeltaE + std::complex<double>(0.0, _quadpoints(j)));
      Eigen::MatrixXcd Amx = denominator.asDiagonal() * Imx;
      Eigen::MatrixXcd Cmx = Imx * (_dielinv_matrices_r[j]);
      std::complex<double> value = (Cmx.cwiseProduct(Amx)).sum();
      result += _quadadaptedweights(j) * value;
    }

    result *= 0.5;
  } else if (_opt.quadrature_scheme == "laguerre") {
    //This work for integration limits a = 0 b = +infty
    for (Index j = 0; j < _opt.order; ++j) {
      Eigen::VectorXcd coeffs1 =
          (DeltaE) / (DeltaE.square() + std::pow(_quadpoints(j), 2));
      Eigen::MatrixXcd Amx = coeffs1.asDiagonal() * Imx;
      Eigen::MatrixXcd Cmx = Imx * (_dielinv_matrices_r[j]);
      std::complex<double> value = (Cmx.cwiseProduct(Amx)).sum();
      result += _quadadaptedweights(j) * value;
    }
  }
  return result.real() / (tools::conv::Pi);
}

}  // namespace xtp

}  // namespace votca
