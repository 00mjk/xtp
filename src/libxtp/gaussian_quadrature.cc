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

#include <c++/5/cmath>
#include <votca/tools/constants.h>
#include <votca/xtp/gauss_laguerre_quadrature_constants.h>
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
  Gauss_Laguerre_Quadrature_Constants glqc;
  _quadpoints = glqc.getPoints(_opt.order);
  _quadadaptedweights = glqc.getAdaptedWeights(_opt.order);
  CalcDielInvVector(rpa);
}

// This function calculates and stores inverses of the microscopic dielectric
// matrix in a matrix vector
void GaussianQuadrature::CalcDielInvVector(const RPA& rpa) {
  Eigen::MatrixXd eps_inv_zero = rpa.calculate_real_epsilon_inverse(0.0, 0.0);
  Eigen::MatrixXd Id =
      Eigen::MatrixXd::Identity(eps_inv_zero.rows(), eps_inv_zero.cols());
  for (int j = 0; j < _opt.order; j++) {
    double exp_alpha = std::exp(-1 * std::pow(_opt.alpha * _quadpoints(j), 2));
    Eigen::MatrixXd eps_inv_j =
        rpa.calculate_real_epsilon_inverse(0.0, _quadpoints(j)) - Id;
    Eigen::MatrixXd eps_inv_zero_alpha = exp_alpha * (eps_inv_zero - Id);
    _dielinv_matrices_r.push_back(-eps_inv_j + eps_inv_zero_alpha);
  }
}
void GaussianQuadrature::CalcDielInvVector_i(const RPA& rpa) {
  Eigen::MatrixXd eps_inv_zero_i = rpa.calculate_imag_epsilon_inverse(0.0, 0.0);
  Eigen::MatrixXd Id =
      Eigen::MatrixXd::Identity(eps_inv_zero_i.rows(), eps_inv_zero_i.cols());
  for (int j = 0; j < _opt.order; j++) {
    double exp_alpha = std::exp(-1 * std::pow(_opt.alpha * _quadpoints(j), 2));
    Eigen::MatrixXd eps_inv_j_i =
        rpa.calculate_imag_epsilon_inverse(0.0, _quadpoints(j)) - Id;
    Eigen::MatrixXd eps_inv_zero_alpha = exp_alpha * (eps_inv_zero_i - Id);
    _dielinv_matrices_i.push_back(-eps_inv_j_i + eps_inv_zero_alpha);
  }
}

Eigen::VectorXd GaussianQuadrature::SigmaGQDiag(
    const Eigen::VectorXd& frequencies, const RPA& rpa) const {
  int homo = _opt.homo - _opt.rpamin;
  int occupied = homo + 1;
  int rpatotal = _energies.size();
  int unoccupied = rpatotal - occupied;
  double middle_of_gap_energy = (_energies(homo + 1) + _energies(homo)) / 2;
  const Eigen::ArrayXd energies = _energies.array();
  const Eigen::ArrayXd frequencies_ar = frequencies.array();
  double eta = rpa.getEta();
  int auxsize = _Mmn.auxsize();

  Eigen::VectorXd result = Eigen::VectorXd::Zero(_opt.qptotal);
#pragma omp parallel for
  for (int m = 0; m < _opt.qptotal; ++m) {
    const Eigen::MatrixXd& Imx = _Mmn[m];
    Eigen::ArrayXd DeltaE = frequencies_ar(m) - energies;
    Eigen::ArrayXd DeltaESq = DeltaE.square();

    for (int j = 0; j < _opt.order; ++j) {
      Eigen::VectorXd coeffs1 = Eigen::VectorXd(rpatotal);

      coeffs1 = (DeltaE) / (DeltaESq + std::pow(_quadpoints(j), 2));

      Eigen::MatrixXd Amx = coeffs1.asDiagonal() * Imx;

      Eigen::MatrixXd Cmx = Imx * (_dielinv_matrices_r[j]);  // C = I * B

      double value = (Cmx.cwiseProduct(Amx)).sum();

      result(m) += _quadadaptedweights(j) * value;
    }
  }
  result /= (tools::conv::Pi);
  return result;
}

Eigen::VectorXd GaussianQuadrature::SigmaGQDiag_i(
    const Eigen::VectorXd& frequencies, const RPA& rpa) const {
  int homo = _opt.homo - _opt.rpamin;
  int occupied = homo + 1;
  int rpatotal = _energies.size();
  int unoccupied = rpatotal - occupied;
  double middle_of_gap_energy = (_energies(homo + 1) + _energies(homo)) / 2;
  const Eigen::ArrayXd energies = _energies.array();
  const Eigen::ArrayXd frequencies_ar = frequencies.array();
  double eta = rpa.getEta();
  int auxsize = _Mmn.auxsize();

  Eigen::VectorXd result = Eigen::VectorXd::Zero(_opt.qptotal);
#pragma omp parallel for
  for (int m = 0; m < _opt.qptotal; ++m) {
    const Eigen::MatrixXd& Imx = _Mmn[m];
    Eigen::ArrayXd DeltaE = frequencies_ar(m) - energies;
    Eigen::ArrayXd DeltaESq = DeltaE.square();

    for (int j = 0; j < _opt.order; ++j) {
      std::cout << "HEY I AM HERE" << std::endl;
      std::cout << _dielinv_matrices_i[j].norm();
      Eigen::VectorXd coeffs1 = Eigen::VectorXd(rpatotal);

      coeffs1 = (DeltaE) / (DeltaESq + std::pow(_quadpoints(j), 2));

      Eigen::MatrixXd Amx = coeffs1.asDiagonal() * Imx;

      Eigen::MatrixXd Cmx = Imx * (_dielinv_matrices_i[j]);  // C = I * B

      double value = (Cmx.cwiseProduct(Amx)).sum();

      result(m) += _quadadaptedweights(j) * value;
    }
  }
  result /= (tools::conv::Pi);
  return result;
}

Eigen::MatrixXd GaussianQuadrature::SigmaGQ(const Eigen::VectorXd& frequencies,
                                            const RPA& rpa) const {
  Eigen::MatrixXd result = Eigen::MatrixXd::Zero(_opt.qptotal, _opt.qptotal);
  int homo = _opt.homo - _opt.rpamin;
  int occupied = homo + 1;
  int rpatotal = _energies.size();
  int unoccupied = rpatotal - occupied;
  double middle_of_gap_energy = (_energies(homo + 1) + _energies(homo)) / 2;
  const Eigen::ArrayXd shiftedenergies =
      _energies.array() - middle_of_gap_energy;
  const Eigen::ArrayXd shiftedfrequencies =
      frequencies.array() - middle_of_gap_energy;
  double eta = rpa.getEta();
  int auxsize = _Mmn.auxsize();
#pragma omp parallel for schedule(dynamic)
  for (int m = 0; m < _opt.qptotal; ++m) {
    const Eigen::MatrixXd& Imxm = _Mmn[m];
    for (int n = m + 1; n < _opt.qptotal; ++n) {
      const Eigen::MatrixXd& Imxn = _Mmn[n];
      Eigen::ArrayXd DeltaEm = shiftedfrequencies(m) - shiftedenergies;
      Eigen::ArrayXd DeltaEn = shiftedfrequencies(n) - shiftedenergies;
      Eigen::ArrayXd DeltaEmSq = DeltaEm.square();
      Eigen::ArrayXd DeltaEnSq = DeltaEn.square();
      double resultmn = 0.0;
      for (int j = 0; j < _opt.order; ++j) {
        double quad_m_eta = _quadpoints(j) - eta;
        double quad_p_eta = _quadpoints(j) + eta;
        Eigen::MatrixXd Pmxn = Imxn * ((_dielinv_matrices_r[j]).real()) - Imxn;
        Eigen::VectorXd coeffs1m = Eigen::VectorXd(rpatotal);
        coeffs1m.segment(0, occupied) =
            DeltaEm.segment(0, occupied) /
            (DeltaEmSq.segment(0, occupied) + std::pow(quad_m_eta, 2));
        coeffs1m.segment(occupied, unoccupied) =
            DeltaEm.segment(occupied, unoccupied) /
            (DeltaEmSq.segment(occupied, unoccupied) + std::pow(quad_p_eta, 2));
        Eigen::MatrixXd Amxm = coeffs1m.asDiagonal() * Imxm;
        double value = (Pmxn.cwiseProduct(Amxm)).sum();
        Eigen::MatrixXd Pmxm = Imxm * ((_dielinv_matrices_r[j]).real()) - Imxm;
        Eigen::VectorXd coeffs1n = Eigen::VectorXd(rpatotal);
        coeffs1n.segment(0, occupied) =
            DeltaEn.segment(0, occupied) /
            (DeltaEnSq.segment(0, occupied) + std::pow(quad_m_eta, 2));
        coeffs1n.segment(occupied, unoccupied) =
            DeltaEn.segment(occupied, unoccupied) /
            (DeltaEnSq.segment(occupied, unoccupied) + std::pow(quad_p_eta, 2));
        Eigen::MatrixXd Amxn = coeffs1n.asDiagonal() * Imxn;
        value += (Pmxm.cwiseProduct(Amxn).sum)();
        Eigen::MatrixXd Qmxn = Imxn * ((_dielinv_matrices_r[j]).imag());
        Eigen::VectorXd coeffs2m = Eigen::VectorXd(rpatotal);
        coeffs2m.segment(0, occupied) =
            quad_m_eta /
            (DeltaEmSq.segment(0, occupied) + std::pow(quad_m_eta, 2));
        coeffs2m.segment(occupied, unoccupied) =
            quad_p_eta /
            (DeltaEmSq.segment(occupied, unoccupied) + std::pow(quad_p_eta, 2));
        Eigen::MatrixXd Bmxm = coeffs2m.asDiagonal() * Imxm;
        value += (Qmxn.cwiseProduct(Bmxm)).sum();
        Eigen::MatrixXd Qmxm = Imxm * ((_dielinv_matrices_r[j]).imag());
        Eigen::VectorXd coeffs2n = Eigen::VectorXd(rpatotal);
        coeffs2n.segment(0, occupied) =
            quad_m_eta /
            (DeltaEnSq.segment(0, occupied) + std::pow(quad_m_eta, 2));
        coeffs2n.segment(occupied, unoccupied) =
            quad_p_eta /
            (DeltaEnSq.segment(occupied, unoccupied) + std::pow(quad_p_eta, 2));
        Eigen::MatrixXd Bmxn = coeffs2n.asDiagonal() * Imxn;
        value += (Qmxm.cwiseProduct(Bmxn)).sum();
        resultmn += _quadadaptedweights(j) * value;
      }
      result(m, n) = resultmn;
      result(n, m) = resultmn;
    }
  }
  result /= (-4 * tools::conv::Pi);
  return result;
}

Eigen::VectorXd GaussianQuadrature::ExactSigmaGQDiag(
    const Eigen::VectorXd& frequencies, const RPA& rpa) const {
  Eigen::VectorXd result = Eigen::VectorXd::Zero(_opt.qptotal, _opt.qptotal);
  Eigen::VectorXcd complexresult =
      Eigen::VectorXcd::Zero(_opt.qptotal, _opt.qptotal);
  Eigen::VectorXd shiftedenergies =
      _energies.array() - (_energies(_opt.homo - _opt.rpamin) +
                           _energies(_opt.homo - _opt.rpamin + 1)) /
                              2;
  Eigen::VectorXd shiftedfrequencies =
      frequencies.array() - (_energies(_opt.homo - _opt.rpamin) +
                             _energies(_opt.homo - _opt.rpamin + 1)) /
                                2;
  double eta = rpa.getEta();
  int rpatotal = _energies.size();
  int auxsize = _Mmn.auxsize();
  Eigen::MatrixXcd Id = Eigen::MatrixXcd::Identity(auxsize, auxsize);
  for (int m = 0; m < _opt.qptotal; ++m) {
    const Eigen::MatrixXd Imxm = _Mmn[m];
    for (int j = 0; j < _opt.order; ++j) {
      Eigen::MatrixXcd DielMxInv = _dielinv_matrices_r[j];
      Eigen::MatrixXcd DielMxInvReal = DielMxInv.real();
      Eigen::MatrixXcd DielMxInvImag = DielMxInv.imag();
      std::complex<double> GHQ(0, 0);
      for (int i = 0; i < _opt.homo - _opt.rpamin + 1; ++i) {
        std::complex<double> omega2(0, _quadpoints(j) - eta);
        for (int mu = 0; mu < auxsize; ++mu) {
          for (int nu = 0; nu < auxsize; ++nu) {
            GHQ += Imxm(i, mu) * Imxm(i, nu) *
                   (1 / (shiftedfrequencies(m) - shiftedenergies(i) + omega2)) *
                   (DielMxInv(mu, nu) - Id(mu, nu));
          }
        }
      }
      for (int i = _opt.homo - _opt.rpamin + 1; i < rpatotal; ++i) {
        std::complex<double> omega2(0, _quadpoints(j) + eta);
        for (int mu = 0; mu < auxsize; ++mu) {
          for (int nu = 0; nu < auxsize; ++nu) {
            GHQ += Imxm(i, mu) * Imxm(i, nu) *
                   (1 / (shiftedfrequencies(m) - shiftedenergies(i) + omega2)) *
                   (DielMxInv(mu, nu) - Id(mu, nu));
          }
        }
      }
      complexresult(m) += _quadadaptedweights(j) * GHQ;
    }
  }
  complexresult /= (-2 * tools::conv::Pi);
  result = complexresult.real();
  return result;
}

Eigen::MatrixXd GaussianQuadrature::ExactSigmaGQOffDiag(
    const Eigen::VectorXd& frequencies, const RPA& rpa) const {
  Eigen::MatrixXd result = Eigen::MatrixXd::Zero(_opt.qptotal, _opt.qptotal);
  Eigen::MatrixXcd complexresult =
      Eigen::MatrixXcd::Zero(_opt.qptotal, _opt.qptotal);
  Eigen::VectorXd shiftedenergies =
      _energies.array() - (_energies(_opt.homo - _opt.rpamin) +
                           _energies(_opt.homo - _opt.rpamin + 1)) /
                              2;
  Eigen::VectorXd shiftedfrequencies =
      frequencies.array() - (_energies(_opt.homo - _opt.rpamin) +
                             _energies(_opt.homo - _opt.rpamin + 1)) /
                                2;
  double eta = rpa.getEta();
  int rpatotal = _energies.size();
  int auxsize = _Mmn.auxsize();
  Eigen::MatrixXcd Id = Eigen::MatrixXcd::Identity(auxsize, auxsize);
  for (int m = 0; m < _opt.qptotal; ++m) {
    const Eigen::MatrixXd Imxm = _Mmn[m];
    for (int n = 0; n < _opt.qptotal; ++n) {
      const Eigen::MatrixXd Imxn = _Mmn[n];
      for (int i = 0; i < _opt.homo - _opt.rpamin + 1; ++i) {
        for (int mu = 0; mu < auxsize; ++mu) {
          for (int nu = 0; nu < auxsize; ++nu) {
            std::complex<double> GHQ(0, 0);
            for (int j = 0; j < _opt.order; ++j) {
              Eigen::MatrixXcd DielMxInv = _dielinv_matrices_r[j];
              std::complex<double> omega2(0, _quadpoints(j) - eta);
              GHQ +=
                  _quadadaptedweights(j) *
                  (1 / (shiftedfrequencies(m) - shiftedenergies(i) + omega2)) *
                  (DielMxInv(mu, nu) - Id(mu, nu));
            }
            complexresult(m, n) -= Imxm(i, mu) * Imxn(i, nu) * GHQ;
          }
        }
      }
      for (int i = _opt.homo - _opt.rpamin + 1; i < rpatotal; ++i) {
        for (int mu = 0; mu < auxsize; ++mu) {
          for (int nu = 0; nu < auxsize; ++nu) {
            std::complex<double> GHQ(0, 0);
            for (int j = 0; j < _opt.order; ++j) {
              Eigen::MatrixXcd DielMxInv = _dielinv_matrices_r[j];
              std::complex<double> omega2(0, _quadpoints(j) + eta);
              GHQ +=
                  _quadadaptedweights(j) *
                  (1 / (shiftedfrequencies(m) - shiftedenergies(i) + omega2)) *
                  (DielMxInv(mu, nu) - Id(mu, nu));
            }
            complexresult(m, n) -= Imxm(i, mu) * Imxn(i, nu) * GHQ;
          }
        }
      }
      for (int i = 0; i < _opt.homo - _opt.rpamin + 1; ++i) {
        for (int mu = 0; mu < auxsize; ++mu) {
          for (int nu = 0; nu < auxsize; ++nu) {
            std::complex<double> GHQ(0, 0);
            for (int j = 0; j < _opt.order; ++j) {
              Eigen::MatrixXcd DielMxInv =
                  Eigen::MatrixXcd::Zero(auxsize, auxsize);
              DielMxInv = _dielinv_matrices_r[j];
              std::complex<double> omega2(0, _quadpoints(j) - eta);
              GHQ +=
                  _quadadaptedweights(j) *
                  (1 / (shiftedfrequencies(n) - shiftedenergies(i) + omega2)) *
                  (DielMxInv(mu, nu) - Id(mu, nu));
            }
            complexresult(m, n) -= Imxm(i, mu) * Imxn(i, nu) * GHQ;
          }
        }
      }
      for (int i = _opt.homo - _opt.rpamin + 1; i < rpatotal; ++i) {
        for (int mu = 0; mu < auxsize; ++mu) {
          for (int nu = 0; nu < auxsize; ++nu) {
            std::complex<double> GHQ(0, 0);
            for (int j = 0; j < _opt.order; ++j) {
              Eigen::MatrixXcd DielMxInv = _dielinv_matrices_r[j];
              std::complex<double> omega2(0, _quadpoints(j) + eta);
              GHQ +=
                  _quadadaptedweights(j) *
                  (1 / (shiftedfrequencies(n) - shiftedenergies(i) + omega2)) *
                  (DielMxInv(mu, nu) - Id(mu, nu));
            }
            complexresult(m, n) -= Imxm(i, mu) * Imxn(i, nu) * GHQ;
          }
        }
      }
    }
  }
  complexresult /= (2 * tools::conv::Pi);
  result = complexresult.real();
  result /= 2;
  return result;
}

}  // namespace xtp
}  // namespace votca
