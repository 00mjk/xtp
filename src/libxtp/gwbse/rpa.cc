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

#include "votca/xtp/threecenter.h"
#include "votca/xtp/vc2index.h"
#include <complex>
#include <votca/xtp/aomatrix.h>
#include <votca/xtp/rpa.h>

using std::flush;

namespace votca {
namespace xtp {

void RPA::UpdateRPAInputEnergies(const Eigen::VectorXd& dftenergies,
                                 const Eigen::VectorXd& gwaenergies,
                                 Index qpmin) {
  Index rpatotal = _rpamax - _rpamin + 1;
  _energies = dftenergies.segment(_rpamin, rpatotal);
  Index gwsize = Index(gwaenergies.size());
  Index lumo = _homo + 1;

  Index qpmax = qpmin + gwsize - 1;
  _energies.segment(qpmin - _rpamin, gwsize) = gwaenergies;
  double DFTgap = dftenergies(lumo) - dftenergies(_homo);
  double QPgap = gwaenergies(lumo - qpmin) - gwaenergies(_homo - qpmin);
  double shift = QPgap - DFTgap;
  Index levelaboveqpmax = _rpamax - qpmax;
  _energies.segment(qpmax + 1 - _rpamin, levelaboveqpmax).array() += shift;
}

template <bool imag>
Eigen::MatrixXd RPA::calculate_epsilon(double frequency) const {
  const Index size = _Mmn.auxsize();
  std::vector<Eigen::MatrixXd> thread_result = std::vector<Eigen::MatrixXd>(
      OPENMP::getMaxThreads(), Eigen::MatrixXd::Zero(size, size));
  const Index lumo = _homo + 1;
  const Index n_occ = lumo - _rpamin;
  const Index n_unocc = _rpamax - lumo + 1;
  const double freq2 = frequency * frequency;
  const double eta2 = _eta * _eta;
#pragma omp parallel for schedule(dynamic)
  for (Index m_level = 0; m_level < n_occ; m_level++) {
    const double qp_energy_m = _energies(m_level);

    const Eigen::MatrixXd Mmn_RPA = _Mmn[m_level].bottomRows(n_unocc);

    const Eigen::ArrayXd deltaE = _energies.tail(n_unocc).array() - qp_energy_m;
    Eigen::VectorXd denom;
    if (imag) {
      denom = 4 * deltaE / (deltaE.square() + freq2);
    } else {
      Eigen::ArrayXd deltEf = deltaE - frequency;
      Eigen::ArrayXd sum = deltEf / (deltEf.square() + eta2);
      deltEf = deltaE + frequency;
      sum += deltEf / (deltEf.square() + eta2);
      denom = 2 * sum;
    }
    thread_result[OPENMP::getThreadId()] +=
        Mmn_RPA.transpose() * denom.asDiagonal() * Mmn_RPA;
  }
  Eigen::MatrixXd result = Eigen::MatrixXd::Identity(size, size);
  for (const auto& mat : thread_result) {
    result += mat;
  }
  return result;
}

Eigen::MatrixXcd RPA::calculate_epsilon_complex(double frequency_r,double frequency_i) const {
  // This is for the case of complex frequencies (general case omega = alpha + i
  // beta)
  const Index size = _Mmn.auxsize();
  std::vector<Eigen::MatrixXcd> thread_result = std::vector<Eigen::MatrixXcd>(
      OPENMP::getMaxThreads(), Eigen::MatrixXcd::Zero(size, size));
  const Index lumo = _homo + 1;
  const Index n_occ = lumo - _rpamin;
  const Index n_unocc = _rpamax - lumo + 1;

  std::complex<double> frequency(frequency_r, frequency_i);
  std::complex<double> eta(0.0, _eta);  
 
#pragma omp parallel for
  for (Index m_level = 0; m_level < n_occ; m_level++) {
    const double qp_energy_m = _energies(m_level);
    const Eigen::MatrixXd Mmn_RPA =
        _Mmn[m_level].block(n_occ, 0, n_unocc, size);
    const Eigen::ArrayXcd deltaE =
        _energies.segment(n_occ, n_unocc).array() - qp_energy_m;

    Eigen::VectorXcd chi;
    Eigen::ArrayXcd deltaEm = frequency - deltaE + eta;
    Eigen::ArrayXcd deltaEp = frequency + deltaE - eta;

    
    chi = deltaEm.cwiseInverse() - deltaEp.cwiseInverse();
    //std::cout << "chi " << chi << std::endl;
    Eigen::MatrixXcd tempresult =  Mmn_RPA.transpose() * chi.asDiagonal()* Mmn_RPA;

  //auto temp = Mmn_RPA.transpose() * chi.asDiagonal();
    //Eigen::MatrixXd tempresult = temp * Mmn_RPA;

    thread_result[OPENMP::getThreadId()] += tempresult;
}   
Eigen::MatrixXcd result = Eigen::MatrixXcd::Identity(size, size);
   
for (const auto& mat : thread_result) {
    result -= mat;
  }
  return result;
}



template <bool imag>
Eigen::MatrixXd RPA::calculate_epsilon_cmplxfreq(double frequency_r,
                                                 double frequency_i) const {
  // This is for the case of complex frequencies (general case omega = alpha + i
  // beta)
  const Index size = _Mmn.auxsize();
  std::vector<Eigen::MatrixXd> thread_result = std::vector<Eigen::MatrixXd>(
      OPENMP::getMaxThreads(), Eigen::MatrixXd::Zero(size, size));
  const Index lumo = _homo + 1;
  const Index n_occ = lumo - _rpamin;
  const Index n_unocc = _rpamax - lumo + 1;

  // Sum and Difference (and relative squares) for the imaginary part of the
  // frequency beta + eta
  const double freq_i_p = frequency_i + _eta;
  const double freq_i_p_2 = std::pow(freq_i_p, 2);
  // beta - eta
  const double freq_i_m = frequency_i -  _eta;
  const double freq_i_m_2 = std::pow(freq_i_m, 2);

#pragma omp parallel for
  for (Index m_level = 0; m_level < n_occ; m_level++) {
    // Occupied level
    const double qp_energy_m = _energies(m_level);

    const Eigen::MatrixXd Mmn_RPA =
        _Mmn[m_level].block(n_occ, 0, n_unocc, size);
    // Energy difference of the unoccupied energies block and the occupied ones
    // (aka transitions from occupied to unoccupied levels)
    const Eigen::ArrayXd deltaE =
        -_energies.segment(n_occ, n_unocc).array() + qp_energy_m;

    Eigen::VectorXd chi;
    Eigen::ArrayXd deltaEm_r = frequency_r - deltaE;
    Eigen::ArrayXd deltaEp_r = frequency_r + deltaE;
    if (imag) {
      chi = ((freq_i_p) / (deltaEm_r.square() + freq_i_p_2)) -
            ((freq_i_m) / (deltaEp_r.square() + freq_i_m_2));
    } else {

      chi = 1.0 - ((deltaEm_r) / (deltaEm_r.square() + freq_i_p_2)) -
           ((deltaEp_r) / (deltaEp_r.square() + freq_i_m_2));
    }

    auto temp = Mmn_RPA.transpose() * chi.asDiagonal();
    Eigen::MatrixXd tempresult = temp * Mmn_RPA;
    thread_result[OPENMP::getThreadId()] += tempresult;
  }
   Eigen::MatrixXd result = Eigen::MatrixXd::Zero(size, size);
  for (const auto& mat : thread_result) {
    result += mat;
  }
  return 2*result;
}

Eigen::MatrixXd RPA::calculate_real_epsilon_inverse(double frequency_r,
                                                    double frequency_i) const {

  Eigen::MatrixXd eps_real = calculate_epsilon_r(frequency_r, frequency_i);
  Eigen::MatrixXd eps_imag = calculate_epsilon_i(frequency_r, frequency_i);
  return (eps_real + eps_imag * eps_real.inverse() * eps_imag).inverse();
}

Eigen::MatrixXd RPA::calculate_imag_epsilon_inverse(double frequency_r,
                                                    double frequency_i) const {

  Eigen::MatrixXd eps_real = calculate_epsilon_r(frequency_r, frequency_i);
  Eigen::MatrixXd eps_imag = calculate_epsilon_i(frequency_r, frequency_i);
  return -(eps_imag + eps_real * eps_imag.inverse() * eps_real).inverse();
}

template Eigen::MatrixXd RPA::calculate_epsilon<true>(double frequency) const;
template Eigen::MatrixXd RPA::calculate_epsilon<false>(double frequency) const;

RPA::rpa_eigensolution RPA::Diagonalize_H2p() const {
  const Index lumo = _homo + 1;
  const Index n_occ = lumo - _rpamin;
  const Index n_unocc = _rpamax - lumo + 1;
  const Index rpasize = n_occ * n_unocc;

  Eigen::VectorXd AmB = Calculate_H2p_AmB();
  Eigen::MatrixXd ApB = Calculate_H2p_ApB();

  // C = AmB^1/2 * ApB * AmB^1/2
  Eigen::MatrixXd& C = ApB;
  C.applyOnTheLeft(AmB.cwiseSqrt().asDiagonal());
  C.applyOnTheRight(AmB.cwiseSqrt().asDiagonal());

  Eigen::SelfAdjointEigenSolver<Eigen::MatrixXd> es = Diagonalize_H2p_C(C);

  RPA::rpa_eigensolution sol;

  // Do not remove this line! It has to be there for MKL to not crash
  sol.omega = Eigen::VectorXd::Zero(es.eigenvalues().size());
  sol.omega = es.eigenvalues().cwiseSqrt();

  XTP_LOG(Log::info, _log) << TimeStamp()
                           << " Lowest neutral excitation energy (eV): "
                           << tools::conv::hrt2ev * sol.omega.minCoeff()
                           << std::flush;

  sol.XpY = Eigen::MatrixXd(rpasize, rpasize);

  Eigen::VectorXd AmB_sqrt = AmB.cwiseSqrt();
  Eigen::VectorXd Omega_sqrt_inv = sol.omega.cwiseSqrt().cwiseInverse();
  for (Index s = 0; s < rpasize; s++) {
    sol.XpY.col(s) =
        Omega_sqrt_inv(s) * AmB_sqrt.cwiseProduct(es.eigenvectors().col(s));
  }

  return sol;
}

Eigen::VectorXd RPA::Calculate_H2p_AmB() const {
  const Index lumo = _homo + 1;
  const Index n_occ = lumo - _rpamin;
  const Index n_unocc = _rpamax - lumo + 1;
  const Index rpasize = n_occ * n_unocc;
  vc2index vc = vc2index(0, 0, n_unocc);
  Eigen::VectorXd AmB = Eigen::VectorXd::Zero(rpasize);
  for (Index v = 0; v < n_occ; v++) {
    Index i = vc.I(v, 0);
    AmB.segment(i, n_unocc) =
        _energies.segment(n_occ, n_unocc).array() - _energies(v);
  }
  return AmB;
}

Eigen::MatrixXd RPA::Calculate_H2p_ApB() const {
  const Index lumo = _homo + 1;
  const Index n_occ = lumo - _rpamin;
  const Index n_unocc = _rpamax - lumo + 1;
  const Index rpasize = n_occ * n_unocc;
  const Index auxsize = _Mmn.auxsize();
  vc2index vc = vc2index(0, 0, n_unocc);
  Eigen::MatrixXd ApB = Eigen::MatrixXd::Zero(rpasize, rpasize);
#pragma omp parallel for schedule(guided)
  for (Index v2 = 0; v2 < n_occ; v2++) {
    Index i2 = vc.I(v2, 0);
    const Eigen::MatrixXd Mmn_v2T =
        _Mmn[v2].block(n_occ, 0, n_unocc, auxsize).transpose();
    for (Index v1 = v2; v1 < n_occ; v1++) {
      Index i1 = vc.I(v1, 0);
      // Multiply with factor 2 to sum over both (identical) spin states
      ApB.block(i1, i2, n_unocc, n_unocc) =
          2 * 2 * _Mmn[v1].block(n_occ, 0, n_unocc, auxsize) * Mmn_v2T;
    }
  }
  ApB.diagonal() += Calculate_H2p_AmB();
  return ApB;
}

Eigen::SelfAdjointEigenSolver<Eigen::MatrixXd> RPA::Diagonalize_H2p_C(
    const Eigen::MatrixXd& C) const {
  XTP_LOG(Log::error, _log)
      << TimeStamp() << " Diagonalizing two-particle Hamiltonian "
      << std::flush;
  Eigen::SelfAdjointEigenSolver<Eigen::MatrixXd> es(C);  // Uses lower triangle
  XTP_LOG(Log::error, _log)
      << TimeStamp() << " Diagonalization done " << std::flush;
  double minCoeff = es.eigenvalues().minCoeff();
  if (minCoeff <= 0.0) {
    XTP_LOG(Log::error, _log)
        << TimeStamp() << " Detected non-positive eigenvalue: " << minCoeff
        << std::flush;
    throw std::runtime_error("Detected non-positive eigenvalue.");
  }
  return es;
}

}  // namespace xtp
}  // namespace votca
